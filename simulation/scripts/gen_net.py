import helper
import numpy as np

flow_desc = '''\n
First line: # of flows
src0 dst0 3(priority group) dst_port0 size0(bytes to write) start_time0
src1 dst1 3(priority group) dst_port1 size1(bytes to write) start_time1
...
Please order the flows in ascending start_time!
'''
topo_desc = '''\n
First line: total node #, switch node #, link #
Second line: switch node IDs...
src0 dst0 rate delay error_rate
src1 dst1 rate delay error_rate
...
'''
dir = 'simulation/mix/rand_offset/'

def create_topo_top(all_node_num:int, sw_node_num:int, 
                link_num:int, ) -> str:
    all_node_num = 2 
    sw_node_num = 0
    link_num = 1
    line1 = f"{all_node_num} {sw_node_num} {link_num}\n"
    line2 = ""
    for sw_id in range(sw_node_num):
        line2 += f"{sw_id} "
    line2 += "\n" # TODO: can read this empty line?
    return line1 + line2


def create_2n_topo_conf():
    top = create_topo_top(all_node_num=2, sw_node_num=0,
                        link_num=1)
    rate_Gbps = 100
    delay_ms = 0.001
    error_rate = 0
    links = f'0 1 {rate_Gbps}Gbps {delay_ms}ms {error_rate}\n'   
    return top + links + topo_desc

def create_2n1f_flow_conf():
    flow_num = 1
    top = f'{flow_num}\n'
    src, dst, pri_group = 0, 1, 3
    dst_port, size_byte, start_time_s = 100, 10000000, 2
    flows = f'{src} {dst} {pri_group} {dst_port} {size_byte} '+\
            f'{start_time_s}\n'
    return top + flows + flow_desc 

def create_2n_flow_conf(flow_num:int):
    top = f'{flow_num}\n'
    src, dst, pri_group = 0, 1, 3
    # 80Mb, 100Gbps, transtime 0.0008s
    dst_port, size_byte, start_time_s = 100, 10000000, 2 
    flows = ''
    for i in range(flow_num):
        flows += f'{src} {dst} {pri_group} {dst_port} {size_byte} '+\
                 f'{start_time_s + i*0.001}\n'
    return top + flows + flow_desc

def gen_2n1f():
    topo_name = '2n1f_topo.txt'
    flow_name = '2n1f_flow.txt'
    topo_str = create_2n_topo_conf()
    flow_str = create_2n1f_flow_conf()
    helper.write_file(dir+topo_name, topo_str)
    helper.write_file(dir+flow_name, flow_str)

def gen_2n2f():
    topo_name = '2n2f_topo.txt'
    flow_name = '2n2f_flow.txt'
    topo_str = create_2n_topo_conf()
    flow_str = create_2n_flow_conf(flow_num=2)
    helper.write_file(dir+topo_name, topo_str)
    helper.write_file(dir+flow_name, flow_str)

def main():
    gen_2n1f()
    gen_2n2f()


if __name__ == '__main__':
    main()