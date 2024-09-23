import argparse
import scripts.run_hybrid as run_hybrid
import scripts.helper as helper
from scripts.run_exp_base import *
import sys
import pandas as pd
import time
import os
from multiprocessing import Process

# to run experiments:
# 1. generate traffic: gen_llm_flows.gen_llm_traffic()
# 2. set random param file
# 3. determine fct, pfc output

#TODO: remove trace
#TODO: monitor link utilization


# ----------v--------- run experiment ------------------------
class HPCCExperiment(ExperimentRunnerBase):
    def __init__(self, blueprint_path, status_col_name, proj_dir, app_path, proc_num):
        super().__init__(blueprint_path, status_col_name)
        self.proj_dir = proj_dir
        self.app_path = app_path # "simulation/build/scratch/third"
        self.proc_num = proc_num

    def run_by_blueprint(self):
        unrun_row, row_id = self._find_unrun_row()
        while unrun_row is not False:
            self.run_row(unrun_row, row_id)
            unrun_row, row_id = self._find_unrun_row()
        print("all experiments finished") 

    def run_by_blueprint_parallel(self):
        unrun_row_li, row_id_li = self._find_unrun_row_list(self.proc_num)
        while unrun_row_li != [] and row_id_li != []:
            procs = []
            for i in range(len(unrun_row_li)):
                p = Process(target=self.run_row, args=(unrun_row_li[i], row_id_li[i]))
                procs.append(p)
                p.start()
            for p in procs:
                p.join()
            unrun_row_li, row_id_li = self._find_unrun_row_list(self.proc_num)   

    def run_row(self, unrun_row, row_id):
        self.set_blueprint_running(row_id)
        # run experiment
        conf_path, runtime_s = self.execute(unrun_row)
        # parse results
        hpcc_parser = HPCCResultParser(conf_path)
        results = hpcc_parser.parse_fct()
        self.update_blueprint(row_id, results, runtime_s)
    

    def set_blueprint_running(self, row_id):
        blueprint = self.read_blueprint()
        blueprint.loc[row_id, self.status_col_name] = self.EXP_RUNNING_STATUS
        self.save_blueprint(blueprint)
    
    def execute(self, row):
        conf_path = gen_exp_conf(topo=row.get('topo'), seed=row.get('seed'),
                                 flow_num=row.get('flowNum'), cc=row.get('cc'),
                                 enable_randoffset=row.get('randOffset'),
                                 proj_dir=self.proj_dir, slots=row.get('slots'),
                                 multi_factor=row.get('multiFactor'))
        runtime_s = self.run_conf(conf_path)
        return conf_path, runtime_s


    def update_blueprint(self, row_id, results, runtime_s):
        blueprint = self.read_blueprint()
        blueprint.loc[row_id, self.status_col_name] = self.EXP_DONE_STATUS
        blueprint.loc[row_id, 'maxFctNs'] = results['maxFctNs']
        blueprint.loc[row_id, 'avgFctNs'] = results['avgFctNs']
        blueprint.loc[row_id, 'makespanNs'] = results['makespanNs']
        blueprint.loc[row_id, 'runtimeS'] = f'{runtime_s:.2f}'
        self.save_blueprint(blueprint)

    def run_conf(self, conf_path):
        cmd = f'{self.app_path} {conf_path}'
        start_time_s = time.time()
        print(f"running:{cmd}")
        os.system(cmd)
        end_time_s = time.time()
        # logging.debug(f"running time: {(end_time_s-start_time_s)*1000}ms")
        runtime_s = end_time_s - start_time_s
        print(f"running time: {runtime_s}s")
        return runtime_s

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
                 slots, multi_factor):
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
    }
    flow = conf_args['flow']
    # conf_path = run_hybrid.get_conf_path(topo, flow, cc, enable_randoffset, proj_dir, seed, slots, multi_factor) 
    # update flow input file
    _1, flow_file, _2, _3, _4 = run_hybrid.get_input_file_paths(
        topo, conf_args['flow'], proj_dir)
    update_flownum_in_flowfile(flow_file, flow_num)
    # generate config file
    conf_path, TRACE_OUTPUT_FILE, FCT_OUTPUT_FILE, PFC_OUTPUT_FILE = run_hybrid.gen_conf(conf_args)
    run_hybrid.update_param(conf_args)
    return conf_path

def update_flownum_in_flowfile(flow_file:str, flow_num:int):
    with open(flow_file, 'r') as f:
        lines = f.readlines()
        lines[0] = f'{flow_num}\n'
    with open(flow_file, 'w') as f:
        f.writelines(lines)

# ----------v--------  gen blueprint --------------------------------
def add_param_combinations(repetition, slots_li, multi_factors_li, **kwargs):
    lines = []
    combination = [(slots, multi_factor) 
                        for slots in slots_li 
                            for multi_factor in multi_factors_li]
    for slots, multi_factor in combination:
        for i in range(repetition):
            # conf_path = gen_exp_conf(**kwargs, slots=slots, multi_factor=multi_factor)
            lines.append(get_blueprint_line(slots=slots, multi_factor=multi_factor, **kwargs))
    return lines

def get_blueprint_line(topo, seed, flow_num, cc, enable_randoffset, slots, multi_factor):
    line = f'{topo},{seed},{flow_num},{cc},{enable_randoffset},'\
            f'{slots},{multi_factor},-1,'\
            f'-1,-1,-1,-1,-1\n'
    return line

def gen_exp_blueprint():
    headerline_input = 'topo,seed,flowNum,cc,randOffset,'
    headerline_adjust = 'slots,multiFactor,state,'
    headerline_output = 'maxFctNs,avgFctNs,makespanNs,dropPkts,linkUtil\n'
    headerline = headerline_input + headerline_adjust + headerline_output
    topos = ['fat',]
    seed = 100
    repetition = 1
    flow_num_range = [10,16] + list(range(128, 321, 16))
    cc = ['dcqcn', 'hp', 'dctcp', 'timely', 'hpccPint']
    rand_offset = [0, 1]
    inflow_filename = 'llmFlows'
    proj_dir = 'rand_offset/preliminary'

    # rand offset params to try
    slots_li = [100, 1000, 1e6]
    multi_factors_li = [0.2, 0.5, 0.7, 1, 1.5]

    lines = []
    for topo in topos:
        for flow_num in flow_num_range:
            # flow_input_file = run_hybrid.get_input_file_paths(
            #     topo=topo, flow=inflow_filename, proj_dir=proj_dir)[1]
            # update_flownum_in_flowfile(flow_input_file, flow_num)
            for cc in cc:
                for enable_randoffset in rand_offset:
                    kwargs = {
                        'topo': topo,
                        'seed': seed,
                        'flow_num': flow_num,
                        'cc': cc,
                        'enable_randoffset': enable_randoffset,
                    }
                    if enable_randoffset:
                        lines += add_param_combinations(repetition, slots_li, multi_factors_li, **kwargs) 
                    else:
                        kwargs['slots'] = -1
                        kwargs['multi_factor'] = -1
                        lines.append(get_blueprint_line(**kwargs))
                seed += 1

    with open('exp_blueprint.csv', 'w') as f:
        f.write(headerline)
        f.writelines(lines)
    

def main():
    gen_exp_blueprint()


if __name__ == '__main__':
    main()