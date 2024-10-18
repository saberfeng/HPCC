

class BlueprintGenerator:
    def __init__(self):
        self.seed = 0

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
        line = f'{state_default},{topo},{self.seed},{flow_num},{cc},{enable_randoffset},'\
                f'{slots},{multi_factor},'\
                f'-1,-1,-1,-1,-1,-1,-1\n'
        self.seed += 1
        return line
    
    def gen_example_blueprint(self, blueprint_path):
        topos = ['fat',]
        self.seed = 100
        repetition = 1
        flow_num_range = [10,16] + list(range(128, 321, 16))
        cc_li = ['dcqcn', 'hp', 'dctcp', 'timely', 'hpccPint']
        rand_offset = [0, 1]
        inflow_filename = 'llmFlows'
        proj_dir = 'rand_offset/preliminary'
        # rand offset params to try
        slots_li = [100, 1000, 1e6]
        multi_factors_li = [0.2, 0.5, 0.7, 1, 1.5]
        self.gen_blueprint(blueprint_path, topos, self.seed, repetition, flow_num_range, cc_li, rand_offset, inflow_filename, proj_dir, slots_li, multi_factors_li)
    
    def gen_test_blueprint(self, blueprint_path):
        topos = ['fat',]
        self.seed = 300
        repetition = 3
        flow_num_range = [10, 16, 32, 64, 128, 256,319]
        cc_li = ['dcqcn', 'hp', 'dctcp', 'timely', 'hpccPint']
        rand_offset = [0]
        inflow_filename = 'llmFlows'
        proj_dir = 'rand_offset/preliminary'
        # rand offset params to try
        slots_li = [3]
        multi_factors_li = [0.7, 1]
        self.gen_blueprint(blueprint_path, topos, self.seed, repetition, flow_num_range, cc_li, rand_offset, inflow_filename, proj_dir, slots_li, multi_factors_li)

    def gen_blueprint(
            self, blueprint_path, topos, seed, repetition, 
            flow_num_range, cc_li, rand_offset, inflow_filename, 
            proj_dir, slots_li, multi_factors_li):
        headerline_input = 'state,topo,seed,flowNum,cc,randOffset,'
        headerline_adjust = 'slots,multiFactor,'
        headerline_output = 'maxFctNs,avgFctNs,mkspanJobNs,mkspanAllNs,dropPkts,linkUtil,runtimeS\n'
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
                            for i in range(repetition):
                                lines.append(self.get_blueprint_line(**kwargs))
        with open(blueprint_path, 'w') as f:
            f.write(headerline)
            f.writelines(lines)
 