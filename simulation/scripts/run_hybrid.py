import argparse
import sys
import os

config_template="""ENABLE_QCN 1
USE_DYNAMIC_PFC_THRESHOLD 1

PACKET_PAYLOAD_SIZE 1000

TOPOLOGY_FILE {TOPOLOGY_FILE}
FLOW_FILE {FLOW_FILE}
TRACE_FILE {TRACE_FILE}
QUEUE_MONITOR_FILE {QUEUE_MONITOR_FILE}
RANDOM_PARAM_FILE {RANDOM_PARAM_FILE}

DISPLAY_CONFIG 0
TRACE_OUTPUT_FILE {TRACE_OUTPUT_FILE}
FCT_OUTPUT_FILE {FCT_OUTPUT_FILE}
PFC_OUTPUT_FILE {PFC_OUTPUT_FILE}

SIMULATOR_STOP_TIME {sim_time_s}

CC_MODE {mode}
ALPHA_RESUME_INTERVAL {t_alpha}
RATE_DECREASE_INTERVAL {t_dec}
CLAMP_TARGET_RATE 0
RP_TIMER {t_inc}
EWMA_GAIN {g}
FAST_RECOVERY_TIMES 1
RATE_AI {ai}Mb/s
RATE_HAI {hai}Mb/s
MIN_RATE 1000Mb/s
DCTCP_RATE_AI {dctcp_ai}Mb/s

ENABLE_RANDOFFSET {enable_randoffset}

ERROR_RATE_PER_LINK 0.0000
L2_CHUNK_SIZE 4000
L2_ACK_INTERVAL 1
L2_BACK_TO_ZERO 0

HAS_WIN {has_win}
GLOBAL_T 1
VAR_WIN {vwin}
FAST_REACT {us}
U_TARGET {u_tgt}
MI_THRESH {mi}
INT_MULTI {int_multi}
MULTI_RATE 0
SAMPLE_FEEDBACK 0
PINT_LOG_BASE {pint_log_base}
PINT_PROB {pint_prob}

RATE_BOUND 1

ACK_HIGH_PRIO {ack_prio}

LINK_DOWN {link_down}

ENABLE_TRACE {enable_tr}

KMAX_MAP {kmax_map}
KMIN_MAP {kmin_map}
PMAX_MAP {pmax_map}
BUFFER_SIZE {buffer_size}
QLEN_MON_FILE simulation/mix/rand_offset/qlen_{topo}_{flow}_{cc}{failure}.txt
QLEN_MON_START {qlen_mon_start_ns}
QLEN_MON_END {qlen_mon_end_ns}
QLEN_MON_INTV_NS {qlen_mon_intv_ns}
QLEN_MON_DUMP_INTV_NS {qlen_mon_dump_intv_ns}

RANDOFFSET_PARAMS {slots_num} {slots_interval_us}
"""


def get_input_file_paths(topo, flow, proj_dir):
	TOPOLOGY_FILE = "{proj_dir}/{topo}.txt"
	FLOW_FILE = "{proj_dir}/{flow}.txt"
	TRACE_FILE = "{proj_dir}/trace.txt"
	QUEUE_MONITOR_FILE = "{proj_dir}/qmonitor.txt"
	RANDOM_PARAM_FILE = "{proj_dir}/plain_rand_model.txt"
	return TOPOLOGY_FILE.format(proj_dir=proj_dir, topo=topo), \
			FLOW_FILE.format(proj_dir=proj_dir, flow=flow), \
			TRACE_FILE.format(proj_dir=proj_dir), \
			QUEUE_MONITOR_FILE.format(proj_dir=proj_dir), \
			RANDOM_PARAM_FILE.format(proj_dir=proj_dir), \

def get_output_file_paths(topo, flow_num, cc, proj_dir, enable_randoffset,slots,multi_factor,seed):
	failure = ''
	# TRACE_OUTPUT_FILE = f"{proj_dir}/"\
	# 				f"mix_{topo}_s{seed}_f{flow_num}_{cc}{failure}_{enable_randoffset}_sl{slots}_fc{multi_factor}.tr"
	TRACE_OUTPUT_FILE = f"{proj_dir}/"\
					f"mix.tr"
	FCT_OUTPUT_FILE = f"{proj_dir}/"\
					f"fct_{topo}_s{seed}_f{flow_num}_{cc}{failure}_{enable_randoffset}_sl{slots}_fc{multi_factor}.csv"
	PFC_OUTPUT_FILE = f"{proj_dir}/"\
					f"pfc_{topo}_s{seed}_f{flow_num}_{cc}{failure}_{enable_randoffset}_sl{slots}_fc{multi_factor}.txt"
	return TRACE_OUTPUT_FILE, FCT_OUTPUT_FILE, PFC_OUTPUT_FILE

def update_param(slots, multi_factor, topo_file, flow_file):
	slots = slots
	multi_factor = multi_factor

	def multiple_flow_trans(multi_factor):
		flow_num, flow_size_bytes = read_flow_file(flow_file)
		nic_rate_Gbps = read_topo_file(topo_file) 
		flow_trans_time_us = (flow_size_bytes * 8 / nic_rate_Gbps) / 1e3
		slots_interval_us = int(flow_num * flow_trans_time_us * multi_factor)
		return slots_interval_us

	slots_interval_us = multiple_flow_trans(multi_factor)
	param_line = f"{slots:.0f} {slots_interval_us:.0f}\n\n"
	return slots, slots_interval_us

def gen_conf(args):
	topo=args['topo']
	bw = int(args['bw'])
	flow = args['flow']
	#bfsz = 16 if bw==50 else 32
	bfsz = 16 * bw / 50
	u_tgt=args['utgt']/100.
	mi=args['mi']
	pint_log_base=args['pint_log_base']
	pint_prob = args['pint_prob']
	enable_tr = args['enable_tr']
	qlen_mon_intv_ns = 100000 # 0.1 ms
	qlen_mon_dump_intv_ns = 1000000 # 1ms
	qlen_mon_start_ns = 300000 # 0.3ms
	qlen_mon_end_ns = 37000000 # 37ms
	cc = args['cc']
	proj_dir = args['proj_dir']
	enable_randoffset = args['enable_randoffset']
	slots = args['slots']
	multi_factor = args['multi_factor']
	seed = args['seed']
	blueprint_name = args['blueprint_name']
	row_id = args ['row_id']

	failure = ''
	# if args.down != '0 0 0':
	# 	failure = '_down'

	kmax_map = "2 %d %d %d %d"%(bw*1000000000, 400*bw/25, bw*4*1000000000, 400*bw*4/25)
	kmin_map = "2 %d %d %d %d"%(bw*1000000000, 100*bw/25, bw*4*1000000000, 100*bw*4/25)
	pmax_map = "2 %d %.2f %d %.2f"%(bw*1000000000, 0.2, bw*4*1000000000, 0.2)

	TOPOLOGY_FILE, FLOW_FILE, TRACE_FILE, QUEUE_MONITOR_FILE, RANDOM_PARAM_FILE = \
		get_input_file_paths(topo, flow, proj_dir)
	flow_num, _ = read_flow_file(FLOW_FILE)	
	TRACE_OUTPUT_FILE, FCT_OUTPUT_FILE, PFC_OUTPUT_FILE = \
		get_output_file_paths(topo, flow_num, cc, proj_dir, enable_randoffset, slots, multi_factor, seed)
	
	slots_num, slots_interval_us = update_param(slots, multi_factor, TOPOLOGY_FILE, FLOW_FILE)
	
	common_temp_args = {
		"proj_dir": proj_dir,
		"bw": bw,
		"flow": flow,
		"topo": topo,
		"u_tgt": u_tgt,
		"mi": mi,
		"pint_log_base": pint_log_base,
		"pint_prob": pint_prob,
		"link_down": args['down'],
		"failure": failure,
		"buffer_size": bfsz,
		"enable_tr": enable_tr,

		"TOPOLOGY_FILE": TOPOLOGY_FILE,
		"FLOW_FILE": FLOW_FILE,
		"TRACE_FILE": TRACE_FILE,
		"QUEUE_MONITOR_FILE": QUEUE_MONITOR_FILE,
		"RANDOM_PARAM_FILE": RANDOM_PARAM_FILE,

		"TRACE_OUTPUT_FILE": TRACE_OUTPUT_FILE,
		"FCT_OUTPUT_FILE": FCT_OUTPUT_FILE,
		"PFC_OUTPUT_FILE": PFC_OUTPUT_FILE,

		"qlen_mon_intv_ns": qlen_mon_intv_ns,
		"qlen_mon_dump_intv_ns": qlen_mon_dump_intv_ns,
		"qlen_mon_start_ns": qlen_mon_start_ns,
		"qlen_mon_end_ns": qlen_mon_end_ns,
		"enable_randoffset": enable_randoffset,
		"sim_time_s":args['sim_time_s'],

		"slots_num": int(slots_num),
		"slots_interval_us": int(slots_interval_us),
	}

	if (args['cc'].startswith("dcqcn")):
		ai = 5 * bw / 25
		hai = 50 * bw /25

		if args['cc'] == "dcqcn":
			config = config_template.format(cc=args['cc'], mode=1, t_alpha=50, t_dec=50, t_inc=55, g=0.00390625, ai=ai, hai=hai, dctcp_ai=1000, has_win=0, vwin=0, us=0, int_multi=1, ack_prio=1, kmax_map=kmax_map, kmin_map=kmin_map, pmax_map=pmax_map, **common_temp_args)
		elif args['cc'] == "dcqcn_paper":
			config = config_template.format(cc=args['cc'], mode=1, t_alpha=50, t_dec=50, t_inc=55, g=0.00390625, ai=ai, hai=hai, dctcp_ai=1000, has_win=0, vwin=0, us=0, int_multi=1, ack_prio=1, kmax_map=kmax_map, kmin_map=kmin_map, pmax_map=pmax_map, **common_temp_args)
		elif args['cc'] == "dcqcn_vwin":
			config = config_template.format(cc=args['cc'], mode=1, t_alpha=50, t_dec=50, t_inc=55, g=0.00390625, ai=ai, hai=hai, dctcp_ai=1000, has_win=0, vwin=0, us=0, int_multi=1, ack_prio=1, kmax_map=kmax_map, kmin_map=kmin_map, pmax_map=pmax_map, **common_temp_args)
		elif args['cc'] == "dcqcn_paper_vwin":
			config = config_template.format(cc=args['cc'], mode=1, t_alpha=50, t_dec=50, t_inc=55, g=0.00390625, ai=ai, hai=hai, dctcp_ai=1000, has_win=0, vwin=0, us=0, int_multi=1, ack_prio=1, kmax_map=kmax_map, kmin_map=kmin_map, pmax_map=pmax_map, **common_temp_args)
	elif args['cc'] == "hp":
		ai = 10 * bw / 25
		if args['hpai'] > 0:
			ai = args['hpai']
		hai = ai # useless
		int_multi = bw / 25
		cc = "%s%d"%(args['cc'], u_tgt)
		if (mi > 0):
			cc += "mi%d"%mi
		if args['hpai'] > 0:
			cc += "ai%d"%ai
		config = config_template.format(cc=args['cc'], mode=1, t_alpha=50, t_dec=50, t_inc=55, g=0.00390625, ai=ai, hai=hai, dctcp_ai=1000, has_win=0, vwin=0, us=0, int_multi=1, ack_prio=1, kmax_map=kmax_map, kmin_map=kmin_map, pmax_map=pmax_map, **common_temp_args)
	elif args['cc'] == "dctcp":
		ai = 10 # ai is useless for dctcp
		hai = ai  # also useless
		dctcp_ai=615 # calculated from RTT=13us and MTU=1KB, because DCTCP add 1 MTU per RTT.
		kmax_map = "2 %d %d %d %d"%(bw*1000000000, 30*bw/10, bw*4*1000000000, 30*bw*4/10)
		kmin_map = "2 %d %d %d %d"%(bw*1000000000, 30*bw/10, bw*4*1000000000, 30*bw*4/10)
		pmax_map = "2 %d %.2f %d %.2f"%(bw*1000000000, 1.0, bw*4*1000000000, 1.0)
		config = config_template.format(cc=args['cc'], mode=1, t_alpha=50, t_dec=50, t_inc=55, g=0.00390625, ai=ai, hai=hai, dctcp_ai=1000, has_win=0, vwin=0, us=0, int_multi=1, ack_prio=1, kmax_map=kmax_map, kmin_map=kmin_map, pmax_map=pmax_map, **common_temp_args)
	elif args['cc'] == "timely":
		ai = 10 * bw / 10
		hai = 50 * bw / 10
		config = config_template.format(cc=args['cc'], mode=1, t_alpha=50, t_dec=50, t_inc=55, g=0.00390625, ai=ai, hai=hai, dctcp_ai=1000, has_win=0, vwin=0, us=0, int_multi=1, ack_prio=1, kmax_map=kmax_map, kmin_map=kmin_map, pmax_map=pmax_map, **common_temp_args)
	elif args['cc'] == "timely_vwin":
		ai = 10 * bw / 10
		hai = 50 * bw / 10
		config = config_template.format(cc=args['cc'], mode=1, t_alpha=50, t_dec=50, t_inc=55, g=0.00390625, ai=ai, hai=hai, dctcp_ai=1000, has_win=0, vwin=0, us=0, int_multi=1, ack_prio=1, kmax_map=kmax_map, kmin_map=kmin_map, pmax_map=pmax_map, **common_temp_args)
	elif args['cc'] == "hpccPint":
		ai = 10 * bw / 25
		if args['hpai'] > 0:
			ai = args['hpai']
		hai = ai # useless
		int_multi = bw / 25
		cc = "%s%d"%(args['cc'], args['utgt'])
		if (mi > 0):
			cc += "mi%d"%mi
		if args['hpai'] > 0:
			cc += "ai%d"%ai
		cc += "log%.3f"%pint_log_base
		cc += "p%.3f"%pint_prob
		config = config_template.format(cc=cc, mode=10, t_alpha=1, t_dec=4, t_inc=300, g=0.00390625, ai=ai, hai=hai, dctcp_ai=1000, has_win=1, vwin=1, us=1, int_multi=int_multi, ack_prio=0, kmax_map=kmax_map, kmin_map=kmin_map, pmax_map=pmax_map, **common_temp_args)
	else:
		print("unknown cc:", args['cc'])
		sys.exit(1)

	conf_path = get_conf_path(topo, flow, cc, enable_randoffset, proj_dir, seed, slots, multi_factor, blueprint_name, row_id) 

	print(f'generated:{conf_path}')
	with open(conf_path, "w") as file:
		file.write(config)
	return conf_path, TRACE_OUTPUT_FILE, FCT_OUTPUT_FILE, PFC_OUTPUT_FILE
	
def get_conf_path(topo, flow, cc, enable_randoffset, proj_dir, seed, slots, multi_factor, blueprint_name, row_id):
	return f"{proj_dir}/conf_{blueprint_name}_r{row_id}_{topo}_s{seed}_f{flow}_{cc}_{enable_randoffset}_sl{slots}_fc{multi_factor}.txt"

def read_flow_file(flow_file):
	with open(flow_file, "r") as file:
		lines = file.readlines()
		flow_num = int(lines[0])
		flow_size_bytes = int(lines[1].split(" ")[4])
	return flow_num, flow_size_bytes

def read_topo_file(topo_file):
	with open(topo_file, "r") as file:
		lines = file.readlines()
		node_num, switch_num, link_num = [int(x) for x in lines[0].split(" ")]
		nic_rate = lines[2].split(" ")[2] # Gbps
	return int(nic_rate[0:-4])




if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='run simulation')
	parser.add_argument('--update_model_param', dest='update_model_param', action='store', default=0, help="whether to update model param")
	parser.add_argument('--enable_randoffset', dest='enable_randoffset', action='store', default=1, help="enable rand_offset")
	parser.add_argument('--cc', dest='cc', action='store', default='hp', help="hp/dcqcn/timely/dctcp/hpccPint")
	parser.add_argument('--flow', dest='flow', action='store', default='flow', help="the name of the flow file")
	parser.add_argument('--bw', dest="bw", action='store', default='100', help="the NIC bandwidth")
	parser.add_argument('--down', dest='down', action='store', default='0 0 0', help="link down event")
	parser.add_argument('--topo', dest='topo', action='store', default='fat', help="the name of the topology file")
	parser.add_argument('--utgt', dest='utgt', action='store', type=int, default=95, help="eta of HPCC")
	parser.add_argument('--mi', dest='mi', action='store', type=int, default=0, help="MI_THRESH")
	parser.add_argument('--hpai', dest='hpai', action='store', type=int, default=50, help="AI for HPCC")
	parser.add_argument('--pint_log_base', dest='pint_log_base', action = 'store', type=float, default=1.01, help="PINT's log_base")
	parser.add_argument('--pint_prob', dest='pint_prob', action = 'store', type=float, default=1.0, help="PINT's sampling probability")
	parser.add_argument('--enable_tr', dest='enable_tr', action = 'store', type=int, default=0, help="enable packet-level events dump")
	parser.add_argument('--offset_upbound_us', dest='offset_upbound_us', action = 'store', type=int, default=1000, help="offset upperbound")
	parser.add_argument('--sim_time_s', dest='sim_time_s', action='store', default=10, help="simulation time(s)")
	parser.add_argument('--proj_dir', dest='proj_dir', action='store', default='rand_offset', help="project dir to store files")
	parser.add_argument('--slots', dest='slots', action='store', default=1000, help="number of slots in the random range")
	parser.add_argument('--multi_factor', dest='multi_factor', action='store', default=0.5, help="factor when calculating slots_interval_us")
	args = parser.parse_args()
	
	gen_conf(vars(args))
	if int(args['update_model_param']) == 1:
		update_param(vars(args))
