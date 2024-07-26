import sys
import random
from optparse import OptionParser
from traffic_gen_base import Flow, translate_bandwidth
from enum import Enum, auto


class CollecComm(Enum):
	ALLREDUCE = auto()
	REDUCE = auto()
	BROADCAST = auto()
	REDUCESCATTER = auto()
	ALLGATHER = auto()

class AppScenario(Enum):
	GPT1_data_parallel = 1
	GPT2_pipline_parallel = 2
	GPT3_tensor_parallel = 3
	GPT3_hybrid_parallel = 4

# def AppScenarioType(arg):
# 	try:
# 		return AppScenario[arg]
# 	except KeyError:


class FlowPattern(Enum):
	Random = 1
	Ring = 2
	Tree = 3
	DoubleTree = 4
	Incast = 5

def ns_to_s(time_ns:int):
	return time_ns/1e9

def gen_llm_traffic(nhost, end_time, output, bandwidth, flow_size_bit, interval_ns, flow_pattern:FlowPattern):
	#TODO run GPT1 GPT2 to get real traffic
	#TODO call NCCL to verify allreduce pipelined traffic
	# generate following types of traffic:
	# 1. total random src and dst
	# 2. ring allreduce
	# 3. tree allreduce
	# 4. double tree allreduce
	# 5. incast
	flows = ''
	if flow_pattern == FlowPattern.Incast:
		# pick a host to be the dst
		dst = random.randint(0, nhost-1)
		flow_start_time = 0
		flow_count = 0
		while flow_start_time <= end_time:
			for host_idx in range(nhost):
				# start a burst period
				if host_idx == dst:
					continue
				# add flow from host_idx to dst
				flow_size_byte = int(flow_size_bit/8)
				flows += f'{host_idx} {dst} 3 100 {flow_size_byte} {ns_to_s(flow_start_time)}\n'
				flow_count += 1
			flow_start_time += interval_ns
	return flow_count, flows 

if __name__ == "__main__":
	parser = OptionParser()
	parser.add_option("-c", "--cdf", dest = "cdf_file", 
				   help = "the file of the traffic size cdf", 
				   default = "uniform_distribution.txt")
	parser.add_option("-n", "--nhost", dest = "nhost", 
				   help = "number of hosts")
	parser.add_option("-l", "--load", dest = "load", 
				   help = "the percentage of the traffic load to the network capacity, by default 0.3", default = "0.3")
	parser.add_option("-b", "--bandwidth", dest = "bandwidth", 
				   help = "the bandwidth of host link (G/M/K), by default 10G", 
				   default = "10G")
	parser.add_option("-t", "--time", dest = "time", 
				   help = "the total run time (s), by default 10", default = "10")
	parser.add_option("-o", "--output", dest = "output", 
				   help = "the output file", default = "tmp_traffic.txt")
	# parser.add_option("-cc", "--cc", dest='collec_comm',
	# 			   help = 'type of collective communication, by default 1=allreduce',
	# 			   default=CollecComm.AllReduce)
	parser.add_option('-s', '--scenario',  dest = 'scenario',
				   help='fixed scenarios: GPT1_data_parallel, GPT2_pipline_parallel...;'\
						'Once set, ignore bandwidth and load settting',)
	options,args = parser.parse_args()

	if not options.nhost:
		print("please use -n to enter number of hosts")
		sys.exit(0)
	nhost = int(options.nhost)
	time = float(options.time)*1e9 # translates to ns
	output = options.output

	if options.scenario:
		if AppScenario[options.scenario] == AppScenario.GPT1_data_parallel:
			bandwidth = '50G'
			flow_size_bit = 3540983606  # 48Gbps * 73.77ms =~ 3.54Gbit
			interval_ns = 106557000 # ns ~106ms
	else:
		load = float(options.load)
		bandwidth = translate_bandwidth(options.bandwidth)
		if bandwidth == None:
			print("bandwidth format incorrect") 
			sys.exit(0)

	flow_count, flows =	gen_llm_traffic(nhost, time, output, bandwidth, flow_size_bit, interval_ns, FlowPattern.Incast)
	with open(output, mode='w') as f:
		f.write(str(flow_count)+'\n')
		f.write(flows)
