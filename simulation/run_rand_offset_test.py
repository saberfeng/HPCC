import argparse
import sys
import os

config_template="""ENABLE_QCN 0
USE_DYNAMIC_PFC_THRESHOLD {dynamic_thresh} 

PACKET_PAYLOAD_SIZE 1000
TOPOLOGY_FILE mix/{proj_dir}/{topo}.txt
FLOW_FILE mix/{proj_dir}/{trace}.txt
TRACE_FILE mix/{proj_dir}/trace.txt
TRACE_OUTPUT_FILE mix/{proj_dir}/mix_{topo}_{trace}_{cc}{failure}.tr
FCT_OUTPUT_FILE mix/{proj_dir}/fct_{topo}_{trace}_{cc}{failure}.txt
PFC_OUTPUT_FILE mix/{proj_dir}/pfc_{topo}_{trace}_{cc}{failure}.txt

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
QLEN_MON_FILE mix/{proj_dir}/qlen_{topo}_{trace}_{cc}{failure}.txt
QLEN_MON_START 2000000000
QLEN_MON_END 3000000000
"""

