import argparse
import sys
import os

config_template="""ENABLE_QCN 1
ENABLE_PFC 1
USE_DYNAMIC_PFC_THRESHOLD {dynamic_thresh} 

PACKET_PAYLOAD_SIZE 1000

TOPOLOGY_FILE simulation/mix/{proj_dir}/{topo}.txt
FLOW_FILE simulation/mix/{proj_dir}/{trace}.txt
TRACE_FILE simulation/mix/{proj_dir}/trace.txt
QUEUE_MONITOR_FILE simulation/mix/{proj_dir}/qmonitor.txt
RANDOM_PARAM_FILE simulation/mix/{proj_dir}/plain_rand_model.txt

TRACE_OUTPUT_FILE simulation/mix/{proj_dir}/mix_{topo}_{trace}_{cc}{failure}.tr
FCT_OUTPUT_FILE simulation/mix/{proj_dir}/fct_{topo}_{trace}_{cc}{failure}.csv
PFC_OUTPUT_FILE simulation/mix/{proj_dir}/pfc_{topo}_{trace}_{cc}{failure}.txt

SIMULATOR_STOP_TIME 4.00

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

KMAX_MAP 2 100000000000 1600 400000000000 6400
KMIN_MAP 2 100000000000 400 400000000000 1600
PMAX_MAP 2 100000000000 0.20 400000000000 0.20
BUFFER_SIZE {buffer_size}
QLEN_MON_FILE simulation/mix/{proj_dir}/qlen_{topo}_{trace}_{cc}{failure}.txt
QLEN_MON_START {qlen_mon_start_ns}
QLEN_MON_END {qlen_mon_end_ns}
QLEN_MON_INTV_NS {qlen_mon_intv_ns}
QLEN_MON_DUMP_INTV_NS {qlen_mon_dump_intv_ns}

OFFSET_UPBOUND_US {offset_upbound_us}
"""

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='run simulation')
	parser.add_argument('--enable_randoffset', dest='enable_randoffset', action='store', default=1, help="enable rand_offset")
	#cc_mode 1: DCQCN, 3: HPCC, 7: TIMELY, 8: DCTCP, 10: HPCC-PINT, 11:NONE,
	parser.add_argument('--cc', dest='cc', action='store', default=1, help="the name of the congestion control algorithm")
	parser.add_argument('--trace', dest='trace', action='store', default='flow', help="the name of the flow file")
	parser.add_argument('--bw', dest="bw", action='store', default='100', help="the NIC bandwidth (Gbps)")
	parser.add_argument('--down', dest='down', action='store', default='0 0 0', help="link down event")
	parser.add_argument('--topo', dest='topo', action='store', default='fat', help="the name of the topology file")
	parser.add_argument('--enable_tr', dest='enable_tr', action = 'store', type=int, default=0, help="enable packet-level events dump")
	parser.add_argument('--offset_upbound_us', dest='offset_upbound_us', action = 'store', type=int, default=1000, help="offset upperbound")
	args = parser.parse_args()

	proj_dir = ''
	enable_qcn = 0
	dynamic_thresh = 1
	topo=args.topo
	bw = int(args.bw)
	trace = args.trace
	#bfsz = 16 if bw==50 else 32
	bfsz = 16 * bw / 50
	enable_tr = args.enable_tr
	enable_pfc = 0
	offset_upbound_us = args.offset_upbound_us
	qlen_mon_intv_ns = 100000 # 0.1 ms
	qlen_mon_dump_intv_ns = 1000000 # 1ms
	qlen_mon_start_ns = 300000 # 0.3ms
	qlen_mon_end_ns = 37000000 # 37ms

	failure = ''
	if args.down != '0 0 0':
		failure = '_down'

	
	# python run_rand_offset.py --cc rand_offset --trace 2n1f_flow --bw 50 --topo 2n1f_topo --enable_tr 1
	proj_dir = 'rand_offset'
	cc_code_to_name_map = {1: 'dcqcn', 3: 'hpcc', 7: 'timely', 8: 'dctcp', 10: 'hpcc-pint', 11: 'none'}
	config_name = "mix/%s/config_%s_%s_%s%s.txt"% \
					(proj_dir, topo, trace, cc_code_to_name_map[int(args.cc)], failure)

	enable_qcn = 0
	dynamic_thresh = 0
	config = config_template.format(dynamic_thresh=dynamic_thresh, 
					proj_dir=proj_dir, bw=bw, trace=trace, topo=topo, cc=args.cc, 
					mode=args.cc, has_win=0, vwin=0, us=0, ack_prio=1, 
					link_down=args.down, failure=failure, 
					buffer_size=bfsz, enable_tr=enable_tr,
					enable_pfc=enable_pfc, offset_upbound_us=offset_upbound_us,
					qlen_mon_intv_ns=qlen_mon_intv_ns, 
					qlen_mon_dump_intv_ns=qlen_mon_dump_intv_ns,
					qlen_mon_start_ns=qlen_mon_start_ns,
					qlen_mon_end_ns=qlen_mon_end_ns,
					enable_randoffset=args.enable_randoffset)
	print(config)


	print(config_name)
	with open(config_name, "w") as file:
		file.write(config)
	
	# os.system("./waf --run 'scratch/third %s'"%(config_name))
