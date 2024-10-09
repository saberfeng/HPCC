import argparse
import scripts.run_hybrid as run_hybrid
import scripts.helper as helper
from scripts.run_exp_base import *
import sys
import pandas as pd
import time
import os
from multiprocessing import Process, Lock, cpu_count, Pool, Manager, Queue

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
            'randOffset':int, 'slots':int, 'multiFactor':float,
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
            conf_path, runtime_s = self.execute(unrun_row, row_id)
            # parse results
            hpcc_parser = HPCCResultParser(conf_path)
            results = hpcc_parser.parse_fct()
            mgr.update_blueprint(row_id, results, runtime_s)
            unrun_row, row_id = self._find_unrun_row()
        print("all experiments finished") 

    def run_by_blueprint_proc_pool_que_msg(self):
        # find all unrun rows
        unrun_row_li, row_id_li = self._find_unrun_row_list(-1)        
        with Manager() as manager: # use manager to share lock among processes
            queue = manager.Queue()
            # input for each process: (unrun_row, row_id, lock)
            args_li = [(unrun_row_li[i], row_id_li[i], queue) for i in range(len(unrun_row_li))]
            with Pool(self.proc_num + 1) as pool: # +1 for writer process
                writer_result = pool.apply_async(self.blueprint_writer_process, (queue,))
                pool.map(self.run_row_que_msg, args_li)
                queue.put(None) # terminate the writing process
                writer_result.wait()

    # using multiprocessing.Queue to communicate between processes
    def run_row_que_msg(self, args_li:tuple):
        unrun_row, row_id, queue = args_li
        queue.put((self.act_set_blueprint_running, {'row_id':row_id}))
        # run experiment
        conf_path, runtime_s = self.execute(unrun_row, row_id)
        # parse results
        hpcc_parser = HPCCResultParser(conf_path)
        results = hpcc_parser.parse_fct()
            # debugging
            # results = {'maxFctNs':-1, 'avgFctNs':-1, 'makespanNs':-1}
            # runtime_s = 0
            # time.sleep(1)
        queue.put((self.act_update_blueprint, {'row_id':row_id, 'results':results, 'runtime_s':runtime_s}))   


    def blueprint_writer_process(self, queue:Queue):
        mgr = BlueprintWriter(status_col_name='state', path=self.blueprint_path)
        while True:
            request = queue.get()
            if request is None:
                break
            action, data = request
            if action == self.act_set_blueprint_running:
                mgr.set_blueprint_running(**data)
            elif action == self.act_update_blueprint:
                mgr.update_blueprint(**data)
    
    # extract blueprint name from blueprint_path
    def get_blueprint_name(self):
        return self.blueprint_path.split('/')[-1]

    def execute(self, row, row_id):
        conf_path = gen_exp_conf(topo=row.get('topo'), seed=row.get('seed'),
                                 flow_num=row.get('flowNum'), cc=row.get('cc'),
                                 enable_randoffset=row.get('randOffset'),
                                 proj_dir=self.proj_dir, slots=row.get('slots'),
                                 multi_factor=row.get('multiFactor'),
                                 blueprint_name=self.get_blueprint_name(),
                                 row_id=row_id)
        runtime_s = self.run_conf(conf_path)
        return conf_path, runtime_s


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
        blueprint.loc[row_id, 'makespanNs'] = results['makespanNs']
        blueprint.loc[row_id, 'runtimeS'] = f'{runtime_s:.2f}'
        self.save_blueprint(blueprint) 

class HPCCResultParser:
    def __init__(self, conf_path):
        self.output_paths = self.__read_output_paths_from_conf(conf_path)

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
        path = self.output_paths['FCT_OUTPUT_FILE']
        fct_df = pd.read_csv(path)
        start_ns = fct_df['start(ns)']
        complete_fct_ns = fct_df['complete_fct(ns)']
        end_ns = fct_df['end(ns)']
        result = {
            'maxFctNs': complete_fct_ns.max(),
            'avgFctNs': complete_fct_ns.mean(),
            'makespanNs': end_ns.max() - start_ns.min(),
        }
        return result

    
    def parse_pfc(self):
        path = self.output_paths['PFC_OUTPUT_FILE']
    
    def parse_link_util(self):
        path = self.output_paths['LINK_UTIL_OUTPUT_FILE']
    
    def parse_trace(self):
        path = self.output_paths['TRACE_OUTPUT_FILE']



def gen_exp_conf(topo, seed, flow_num, cc, enable_randoffset, proj_dir, 
                 slots, multi_factor, blueprint_name, row_id):
    #TODO: unfinished
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
        'slots':slots,
        'multi_factor':multi_factor,
        'seed':seed,
        'blueprint_name':blueprint_name,
        'row_id':row_id,
    }
    flow = conf_args['flow']
    # conf_path = run_hybrid.get_conf_path(topo, flow, cc, enable_randoffset, proj_dir, seed, slots, multi_factor) 
    # update flow input file
    _1, flow_file, _2, _3, _4 = run_hybrid.get_input_file_paths(
        topo, conf_args['flow'], proj_dir)
    update_flownum_in_flowfile(flow_file, flow_num)
    # generate config file
    conf_path, TRACE_OUTPUT_FILE, FCT_OUTPUT_FILE, PFC_OUTPUT_FILE = run_hybrid.gen_conf(conf_args)
    return conf_path

def update_flownum_in_flowfile(flow_file:str, flow_num:int):
    with open(flow_file, 'r') as f:
        lines = f.readlines()
        lines[0] = f'{flow_num}\n'
    with open(flow_file, 'w') as f:
        f.writelines(lines)


# ----------v--------  gen blueprint --------------------------------
class BlueprintGenerator:
    def __init__(self):
        pass

    def add_param_combinations(self, repetition, slots_li, multi_factors_li, **kwargs):
        lines = []
        combination = [(slots, multi_factor) 
                            for slots in slots_li 
                                for multi_factor in multi_factors_li]
        for slots, multi_factor in combination:
            for i in range(repetition):
                # conf_path = gen_exp_conf(**kwargs, slots=slots, multi_factor=multi_factor)
                lines.append(self.get_blueprint_line(slots=slots, multi_factor=multi_factor, **kwargs))
        return lines

    def get_blueprint_line(self, topo, seed, flow_num, cc, enable_randoffset, slots, multi_factor):
        state_default = -1
        line = f'{state_default},{topo},{seed},{flow_num},{cc},{enable_randoffset},'\
                f'{slots},{multi_factor},'\
                f'-1,-1,-1,-1,-1,-1\n'
        return line
    
    def gen_example_blueprint(self, blueprint_path):
        topos = ['fat',]
        seed = 100
        repetition = 1
        flow_num_range = [10,16] + list(range(128, 321, 16))
        cc_li = ['dcqcn', 'hp', 'dctcp', 'timely', 'hpccPint']
        rand_offset = [0, 1]
        inflow_filename = 'llmFlows'
        proj_dir = 'rand_offset/preliminary'
        # rand offset params to try
        slots_li = [100, 1000, 1e6]
        multi_factors_li = [0.2, 0.5, 0.7, 1, 1.5]
        self.gen_blueprint(blueprint_path, topos, seed, repetition, flow_num_range, cc_li, rand_offset, inflow_filename, proj_dir, slots_li, multi_factors_li)
    
    def gen_test_blueprint(self, blueprint_path):
        topos = ['fat',]
        seed = 100
        repetition = 1
        flow_num_range = [10]
        cc_li = ['dcqcn', 'hp', 'dctcp', 'timely', 'hpccPint']
        rand_offset = [0, 1]
        inflow_filename = 'llmFlows'
        proj_dir = 'rand_offset/preliminary'
        # rand offset params to try
        slots_li = [3]
        multi_factors_li = [0.7, 1]
        self.gen_blueprint(blueprint_path, topos, seed, repetition, flow_num_range, cc_li, rand_offset, inflow_filename, proj_dir, slots_li, multi_factors_li)

    def gen_blueprint(self, blueprint_path, topos, seed, repetition, flow_num_range, cc_li, rand_offset, inflow_filename, proj_dir, slots_li, multi_factors_li):
        headerline_input = 'state,topo,seed,flowNum,cc,randOffset,'
        headerline_adjust = 'slots,multiFactor,'
        headerline_output = 'maxFctNs,avgFctNs,makespanNs,dropPkts,linkUtil,runtimeS\n'
        headerline = headerline_input + headerline_adjust + headerline_output
        lines = []
        for topo in topos:
            for flow_num in flow_num_range:
                # flow_input_file = run_hybrid.get_input_file_paths(
                #     topo=topo, flow=inflow_filename, proj_dir=proj_dir)[1]
                # update_flownum_in_flowfile(flow_input_file, flow_num)
                for cc in cc_li:
                    for enable_randoffset in rand_offset:
                        kwargs = {
                            'topo': topo,
                            'seed': seed,
                            'flow_num': flow_num,
                            'cc': cc,
                            'enable_randoffset': enable_randoffset,
                        }
                        if enable_randoffset:
                            lines += self.add_param_combinations(repetition, slots_li, multi_factors_li, **kwargs) 
                        else:
                            kwargs['slots'] = -1
                            kwargs['multi_factor'] = -1
                            lines.append(self.get_blueprint_line(**kwargs))
                    seed += 1
        with open(blueprint_path, 'w') as f:
            f.write(headerline)
            f.writelines(lines)
    

def main():
    # gen_exp_blueprint()
    pass


if __name__ == '__main__':
    main()