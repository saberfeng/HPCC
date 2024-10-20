import argparse
import scripts.run_hybrid as run_hybrid
import scripts.helper as helper
from scripts.run_exp_base import *
import sys
import pandas as pd
import time
import os
from multiprocessing import Process, Lock, cpu_count, Pool, Manager, Queue
from queue import Empty
import random

# to run experiments:
# 1. generate traffic: gen_llm_flows.gen_llm_traffic()
# 2. set random param file
# 3. determine fct, pfc output

#TODO: add row id to config name
#TODO: monitor link utilization


# ----------v--------- run experiment ------------------------
class HPCCExperiment(ExperimentRunnerBase):
    def __init__(self, blueprint_path, status_col_name, proj_dir, app_path, proc_num):
        super().__init__(blueprint_path, status_col_name, dtype={
            'state':int, 'topo':str, 'seed':int, 'flowNum':int, 'cc':str,
            'randOffset':int, 'algo':str, 'params':str,
            'maxFctNs':int, 'avgFctNs':float, 'makespanNs':int, 
            'dropPkts':int, 'linkUtil':float, 'runtimeS':int,
        })
        self.proj_dir = proj_dir
        self.app_path = app_path # "simulation/build/scratch/third"
        self.proc_num = proc_num

        self.act_set_blueprint_running = 'action_set_blueprint_running'
        self.act_update_blueprint = 'action_update_blueprint'

    def run_by_blueprint(self):
        unrun_row, row_id = self._find_unrun_row()
        mgr = BlueprintWriter(status_col_name='state', path=self.blueprint_path)
        while unrun_row is not False:
            mgr.set_blueprint_running(row_id)
            # run experiment
            conf_path, runtime_s, trace_output_file, fct_output_file,\
                pfc_output_file = self.execute(unrun_row, row_id)
            # parse results
            hpcc_parser = HPCCResultParser(trace_output_file, fct_output_file, pfc_output_file)
            results = hpcc_parser.parse_fct_multi_job()
            
            # # debug
            # time.sleep(1)
            # runtime_s = 0
            # results = {
            #     'maxFctNs':-1, 'avgFctNs':-1, 'mkspanJobNs':-1, 'mkspanAllNs':-1, 'runtimeS':-1
            # }

            mgr.update_blueprint(row_id, results, runtime_s)
            unrun_row, row_id = self._find_unrun_row()
        print("all experiments finished") 

    # job assignment order not gauranteed
    def run_by_blueprint_proc_pool_que_msg(self):
        # find all unrun rows
        unrun_row_li, row_id_li = self._find_unrun_row_list(-1)        
        with Manager() as manager: # use manager to share lock among processes
            queue_msg = manager.Queue()
            queue_task = manager.Queue()
            # input for each process: (unrun_row, row_id, lock)
            args_li = [(unrun_row_li[i], row_id_li[i], queue_msg) for i in range(len(unrun_row_li))]
            with Pool(self.proc_num + 1) as pool: # +1 for writer process
                writer_result = pool.apply_async(self.proc_blueprint_writer, (queue_msg,))
                pool.map(self.run_row_que_msg, args_li)
                queue_msg.put(None) # terminate the writing process
                writer_result.wait()

    # fixed job assignment order
    def run_by_blueprint_que_tsk_que_msg(self):
        # find all unrun rows
        unrun_row_li, row_id_li = self._find_unrun_row_list(-1)        
        with Manager() as manager: # use manager to share lock among processes
            queue_msg = manager.Queue()
            queue_task = manager.Queue()
            # input for each process: (unrun_row, row_id, lock)
            for task_item in [(unrun_row_li[i], row_id_li[i]) for i in range(len(unrun_row_li))]:
                queue_task.put(task_item)

            # start writer process
            proc_state_writer = Process(target=self.proc_blueprint_writer, args=(queue_msg,))
            proc_state_writer.start()

            proc_worker_li = []
            for i in range(self.proc_num):
                proc_worker = Process(target=self.proc_run_row_qmsg_qtsk, args=(queue_msg, queue_task))
                proc_worker.start()
                proc_worker_li.append(proc_worker)
            for proc in proc_worker_li:
                proc.join()
            queue_msg.put(None) # terminate the writing process
            proc_state_writer.join()
            
    def proc_run_row_qmsg_qtsk(self, queue_msg:Queue, queue_task:Queue):
        while True:
            try:
                task = queue_task.get(timeout=1)
            except Empty:
                break # no more task
            unrun_row, row_id = task
            queue_msg.put((self.act_set_blueprint_running, {'row_id':row_id}))
            # run experiment
            conf_path, runtime_s, trace_output_file, fct_output_file,\
                    pfc_output_file = self.execute(unrun_row, row_id)
            # parse results
            hpcc_parser = HPCCResultParser(trace_output_file, fct_output_file, pfc_output_file)
            results = hpcc_parser.parse_fct_multi_job()

            # # debugging
            # time.sleep(1)
            # runtime_s = 0
            # results = {
            #     'maxFctNs':-1, 'avgFctNs':-1, 'mkspanJobNs':-1, 'mkspanAllNs':-1, 'runtimeS':-1
            # }
            queue_msg.put((self.act_update_blueprint, {'row_id':row_id, 'results':results, 'runtime_s':runtime_s}))

    # using multiprocessing.Queue to communicate between processes
    def run_row_que_msg(self, args_li:tuple):
        unrun_row, row_id, queue = args_li
        queue.put((self.act_set_blueprint_running, {'row_id':row_id}))
        # run experiment
        conf_path, runtime_s, trace_output_file, fct_output_file,\
                pfc_output_file = self.execute(unrun_row, row_id)
        # parse results
        hpcc_parser = HPCCResultParser(trace_output_file, fct_output_file, pfc_output_file)
        results = hpcc_parser.parse_fct_multi_job()
            # debugging
            # results = {'maxFctNs':-1, 'avgFctNs':-1, 'makespanNs':-1}
            # runtime_s = 0
            # time.sleep(1)
        queue.put((self.act_update_blueprint, {'row_id':row_id, 'results':results, 'runtime_s':runtime_s}))   


    def proc_blueprint_writer(self, queue_msg:Queue):
        mgr = BlueprintWriter(status_col_name='state', path=self.blueprint_path)
        while True:
            request = queue_msg.get()
            if request is None:
                break
            action, data = request
            if action == self.act_set_blueprint_running:
                mgr.set_blueprint_running(**data)
            elif action == self.act_update_blueprint:
                mgr.update_blueprint(**data)
    
    # extract blueprint name from blueprint_path, discard the path and extension
    def get_blueprint_name(self):
        return self.blueprint_path.split('/')[-1].split('.')[-2]

    def execute(self, row, row_id):
        meta_flow_file = f'{self.proj_dir}/llmFlows2.txt'
        conf_path, trace_output_file, fct_output_file, \
        pfc_output_file, flow_input_file  = gen_conf_wrapper(
                                    topo=row.get('topo'), seed=row.get('seed'),
                                    flow_num=row.get('flowNum'), cc=row.get('cc'),
                                    enable_randoffset=row.get('randOffset'),
                                    proj_dir=self.proj_dir, algo=row.get('algo'),
                                    params=row.get('params'),
                                    blueprint_name=self.get_blueprint_name(),
                                    row_id=row_id, meta_flow_file=meta_flow_file)
        gen_flow_file(meta_flow_file=meta_flow_file, out_flow_file=flow_input_file, flow_num=row.get('flowNum'))
        runtime_s = self.run_conf(conf_path)
        return conf_path, runtime_s, trace_output_file, fct_output_file, pfc_output_file


    def run_conf(self, conf_path):
        cmd = f'{self.app_path} {conf_path}'
        start_time_s = time.time()
        print(f"running:{cmd}")
        os.system(cmd)
        end_time_s = time.time()
        # logging.debug(f"running time: {(end_time_s-start_time_s)*1000}ms")
        runtime_s = end_time_s - start_time_s
        print(f"runtime: {runtime_s}s")
        return runtime_s

class BlueprintWriter(helper.BlueprintManagerBase):
    def set_blueprint_running(self, row_id):
        blueprint = self.read_blueprint()
        blueprint.loc[row_id, self.status_col_name] = self.EXP_RUNNING_STATUS
        # print(f"row_id{row_id}---writing blueprint:{blueprint.loc[row_id]}")
        self.save_blueprint(blueprint)
   
    def update_blueprint(self, row_id, results, runtime_s):
        blueprint = self.read_blueprint()
        blueprint.loc[row_id, self.status_col_name] = self.EXP_DONE_STATUS
        blueprint.loc[row_id, 'maxFctNs'] = results['maxFctNs']
        blueprint.loc[row_id, 'avgFctNs'] = results['avgFctNs']
        blueprint.loc[row_id, 'mkspanJobNs'] = results['mkspanJobNs']
        blueprint.loc[row_id, 'mkspanAllNs'] = results['mkspanAllNs']
        blueprint.loc[row_id, 'runtimeS'] = f'{runtime_s:.2f}'
        self.save_blueprint(blueprint) 

class HPCCResultParser:
    def __init__(self, trace_out_path, fct_out_path, pfc_out_path):
        self.output_paths = {
            'TRACE_OUTPUT_FILE': trace_out_path,
            'FCT_OUTPUT_FILE': fct_out_path,
            'PFC_OUTPUT_FILE': pfc_out_path,
        }
        self.trace_output_path = trace_out_path
        self.fct_output_path = fct_out_path
        self.pfc_output_path = pfc_out_path

    def __read_output_paths_from_conf(self, conf_path):
        conf_lines = helper.read_file_lines(conf_path)
        output_paths = {}
        for line in conf_lines:
            line = line.strip('\n')
            if line == '':
                continue
            kv_pair = line.split(' ')
            key, value = kv_pair[0], kv_pair[1]
            if key[-11:] == 'OUTPUT_FILE':
                output_paths[key] = value
        return output_paths
    
    def parse_fct(self):
        path = self.fct_output_path
        fct_df = pd.read_csv(path)
        start_ns = fct_df['start(ns)']
        complete_fct_ns = fct_df['complete_fct(ns)']
        end_ns = fct_df['end(ns)']
        result = {
            'maxFctNs': complete_fct_ns.max(),
            'avgFctNs': complete_fct_ns.mean(),
            'mkspanAllNs': end_ns.max() - start_ns.min(),
        }
        return result
    
    def parse_fct_multi_job(self, num_job:int=0):
        path = self.fct_output_path
        fct_df = pd.read_csv(path)
        num_flow = fct_df.shape[0]

        # default 4 flows per job
        if num_job <= 0:
            remainder = num_flow % 4
            num_job = num_flow // 4 + (1 if remainder > 0 else 0)

        shuffled_row_num_li = list(range(num_flow))
        random.shuffle(shuffled_row_num_li)
        num_flow_per_job = num_flow // num_job
        job_id_colname = 'job_id'

        job_id_to_flow_ids = {}
        for job_id in range(num_job):
            job_id_to_flow_ids[job_id] = []
            for j in range(num_flow_per_job):
                shuffled_row_id = shuffled_row_num_li.pop()
                job_id_to_flow_ids[job_id].append(shuffled_row_id)
        for i in range(len(shuffled_row_num_li)):
            job_id_to_flow_ids[i].append(shuffled_row_num_li[i])

        # assign job id to each flow
        for job_id, flow_ids in job_id_to_flow_ids.items():
            fct_df.loc[flow_ids, job_id_colname] = job_id
        # print(fct_df)
        # count rows by job id
        # job_id_to_row_num = fct_df.groupby(job_id_colname).size()
        # print(job_id_to_row_num)
        start_colname = "start(ns)" 
        # complete_fct_colname = "complete_fct(ns)" # use offseted_fct_colname instead
        end_colname = "end(ns)"
        offseted_fct_colname = "offseted_fct(ns)" # offseted_fct = complete_fct + offset

        groupped_df = fct_df.groupby('job_id')
        job_makespan_df = groupped_df[end_colname].max() - groupped_df[start_colname].min()
        result = {
            'maxFctNs': fct_df[offseted_fct_colname].max(),
            'avgFctNs': fct_df[offseted_fct_colname].mean(),
            'mkspanJobNs': self.serialize_job_makespan(job_makespan_df.array),
            'mkspanAllNs': fct_df[end_colname].max() - fct_df[start_colname].min(),
        }
        return result
    
    def serialize_job_makespan(self, makespan_li):
        return ' '.join([f'{makespan}' for makespan in makespan_li])

    def deserialize_job_makespan(self, job_makespan_str:str):
        return [int(makespan) for makespan in job_makespan_str.split(' ')]
    
    def parse_pfc(self):
        path = self.pfc_output_path
    
    def parse_link_util(self):
        pass
    
    def parse_trace(self):
        path = self.trace_output_path



def gen_conf_wrapper(topo, seed, flow_num, cc, enable_randoffset, proj_dir, 
                 algo, params, blueprint_name, row_id, meta_flow_file):
    conf_args = {
        'topo': topo,
        'update_model_param': 0, # TODO
        'enable_randoffset': enable_randoffset,
        'cc': cc,
        'flow':'llmFlows',
        'enable_tr': 0,
        'sim_time_s': 100,
        'proj_dir': proj_dir,
        'bw':100,
        'down':'0 0 0',
        'utgt':95,
        'mi':0,
        'hpai':50,
        'pint_log_base':1.01,
        'pint_prob':1.0,
        'algo':algo,
        'params':params,
        'seed':seed,
        'blueprint_name':blueprint_name,
        'row_id':row_id,
        'flow_num':flow_num,
        'meta_flow_file':meta_flow_file,
    }
    flow = conf_args['flow']
    # conf_path = run_hybrid.get_conf_path(topo, flow, cc, enable_randoffset, proj_dir, seed, algo, params) 
    # update flow input file
    # generate config file
    conf_path, TRACE_OUTPUT_FILE, FCT_OUTPUT_FILE, PFC_OUTPUT_FILE, flow_input_file = run_hybrid.gen_conf(conf_args)
    return conf_path, TRACE_OUTPUT_FILE, FCT_OUTPUT_FILE, PFC_OUTPUT_FILE, flow_input_file

def gen_flow_file(meta_flow_file:str, out_flow_file:str, flow_num:int):
    with open(meta_flow_file, 'r') as f:
        lines = f.readlines()
        lines[0] = f'{flow_num}\n'
        lines = lines[:flow_num+1]
        ordered_start_time_li = []
        flow_size_li = []
        for i in range(1, len(lines)):
            # int(random.uniform(0, 100))
            line_components = lines[i].split(' ')
            start_time_ns = int(line_components[-1])
            start_time_fluctuation_ns = random.uniform(0, 100)
            ordered_start_time_li.append(int(start_time_ns + start_time_fluctuation_ns))
            flow_size_li.append(int(line_components[-2]))
        ordered_start_time_li = sorted(ordered_start_time_li)
        random.shuffle(flow_size_li)

        for i in range(1, len(lines)):
            line_components = lines[i].split(' ')
            line_components[-2] = f'{flow_size_li[i-1]}'
            line_components[-1] = f'{ordered_start_time_li[i-1]}\n'
            lines[i] = ' '.join(line_components)
    with open(out_flow_file, 'w') as f:
        f.writelines(lines)

def update_flownum_in_flowfile(flow_file:str, flow_num:int):
    with open(flow_file, 'r') as f:
        lines = f.readlines()
        lines[0] = f'{flow_num}\n'
    with open(flow_file, 'w') as f:
        f.writelines(lines)


def main():
    # gen_exp_blueprint()
    pass


if __name__ == '__main__':
    main()