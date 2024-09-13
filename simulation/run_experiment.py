import argparse
import run_hybrid
import sys

# to run experiments:
# 1. generate traffic: gen_llm_flows.gen_llm_traffic()
# 2. set random param file
# 3. determine fct, pfc output

#TODO: remove trace
#TODO: monitor link utilization

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

def run_conf(conf_path):
    app_path = "simulation/build/scratch/third"
    cmd = f'{app_path} {conf_path}'
    sys.cmd(cmd)

def gen_exp_conf(topo, seed, flow_num, cc, enable_randoffset, proj_dir, slots, multi_factor):
    conf_path = f'config_{topo}_flow_{flow_num}_{cc}_{seed}.txt'
    line = f'{topo},{seed},{flow_num},{cc},{enable_randoffset},{conf_path},'\
            f'-1,-1,-1,-1,-1\n'

    conf_args = {
        'topo': 'fat',
        'update_model_param': 0, # TODO
        'enable_randoffset': enable_randoffset,
        'cc': cc,
        'flow':'llmFlows',
        'enable_tr': 0,
        'sim_time_s': 100,
        'proj_dir': proj_dir,
    }
    conf_args_str = ' '.join([f'--{k} {v}' for k, v in conf_args.items()])
    gen_config_script_path = f'{proj_dir}/run_hybrid.py'
    cmd = f'python3 run_hybrid.py {conf_args_str}'
    print(cmd)
    sys.cmd(cmd)
    return conf_path

def update_flownum_in_flowfile(flow_file:str, flow_num:int):
    with open(flow_file, 'rw', encoding='utf-8') as f:
        lines = f.readlines()
        lines[0] = f'{flow_num}\n'
        f.writelines(lines)

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