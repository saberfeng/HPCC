import argparse
import sys
import os

config_template="""ENABLE_QCN 0
ENABLE_PFC {enable_pfc}

USE_DYNAMIC_PFC_THRESHOLD {dynamic_thresh} 

PACKET_PAYLOAD_SIZE 1000
TOPOLOGY_FILE simulation/mix/{proj_dir}/{topo}.txt
FLOW_FILE simulation/mix/{proj_dir}/{trace}.txt
TRACE_FILE simulation/mix/{proj_dir}/trace.txt
TRACE_OUTPUT_FILE simulation/mix/{proj_dir}/mix_{topo}_{trace}_{cc}{failure}.tr
FCT_OUTPUT_FILE simulation/mix/{proj_dir}/fct_{topo}_{trace}_{cc}{failure}.txt
PFC_OUTPUT_FILE simulation/mix/{proj_dir}/pfc_{topo}_{trace}_{cc}{failure}.txt

SIMULATOR_STOP_TIME 4.00

CC_MODE {mode}

ERROR_RATE_PER_LINK 0.0000
L2_CHUNK_SIZE 4000
L2_ACK_INTERVAL 1
L2_BACK_TO_ZERO 0

HAS_WIN {has_win}
GLOBAL_T 1
VAR_WIN {vwin}
FAST_REACT {us}

RATE_BOUND 1

ACK_HIGH_PRIO {ack_prio}

LINK_DOWN {link_down}

ENABLE_TRACE {enable_tr}

BUFFER_SIZE {buffer_size}
QLEN_MON_FILE simulation/mix/{proj_dir}/qlen_{topo}_{trace}_{cc}{failure}.txt
QLEN_MON_START 2000000000
QLEN_MON_END 3000000000
"""

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='run simulation')
	parser.add_argument('--cc', dest='cc', action='store', default='rand_offset', help="rand_offset")
	parser.add_argument('--trace', dest='trace', action='store', default='flow', help="the name of the flow file")
	parser.add_argument('--bw', dest="bw", action='store', default='50', help="the NIC bandwidth")
	parser.add_argument('--down', dest='down', action='store', default='0 0 0', help="link down event")
	parser.add_argument('--topo', dest='topo', action='store', default='fat', help="the name of the topology file")
	parser.add_argument('--enable_tr', dest='enable_tr', action = 'store', type=int, default=0, help="enable packet-level events dump")
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

	failure = ''
	if args.down != '0 0 0':
		failure = '_down'

	
	if args.cc == "rand_offset":
		# python run_rand_offset.py --cc rand_offset --trace 2n1f_flow --bw 50 --topo 2n1f_topo --enable_tr 1
		proj_dir = 'rand_offset'
		config_name = "mix/%s/config_%s_%s_%s%s.txt"% \
						(proj_dir, topo, trace, args.cc, failure)

		enable_qcn = 0
		dynamic_thresh = 0
		config = config_template.format(dynamic_thresh=dynamic_thresh, 
						proj_dir=proj_dir, bw=bw, trace=trace, topo=topo, cc=args.cc, 
						mode=12, has_win=0, vwin=0, us=0, ack_prio=1, 
						link_down=args.down, failure=failure, 
						buffer_size=bfsz, enable_tr=enable_tr,
						enable_pfc=enable_pfc)
		print(config)
	else:
		print("unknown cc:", args.cc)
		sys.exit(1)

	with open(config_name, "w") as file:
		file.write(config)
	
	# os.system("./waf --run 'scratch/third %s'"%(config_name))
