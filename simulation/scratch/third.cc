/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation;
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#undef PGO_TRAINING
#define PATH_TO_PGO_CONFIG "path_to_pgo_config"

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <time.h> 
#include <memory>
#include <unordered_set>
#include "ns3/core-module.h"
#include "ns3/qbb-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/packet.h"
#include "ns3/error-model.h"
#include <ns3/rdma.h>
#include <ns3/rdma-client.h>
#include <ns3/rdma-client-helper.h>
#include <ns3/rdma-driver.h>
#include <ns3/switch-node.h>
#include <ns3/sim-setting.h>
// #include "ns3/netcalc_model.h"
// #include "ns3/rand_offset_injector.h"
#include "ns3/plain_random_model.h"

using namespace ns3;
using namespace std;
using namespace rand_offset;

NS_LOG_COMPONENT_DEFINE("GENERIC_SIMULATION");

uint32_t cc_mode = 1; // 1: DCQCN, 3: HPCC, 7: TIMELY, 8: DCTCP, 10: HPCC-PINT, 11:NONE, 12:RAND-OFFSET
uint32_t enable_randoffset = 0;
bool enable_qcn = true, use_dynamic_pfc_threshold = true;
bool enable_pfc = true;
bool enable_qbb = false;
uint32_t packet_payload_size = 1000, l2_chunk_size = 0, l2_ack_interval = 0;
double pause_time = 5, simulator_stop_time = 20.01;
std::string data_rate, link_delay, topology_file, flow_file, 
	trace_file, monitor_file, trace_output_file, rand_param_file;
std::string fct_output_file = "fct.txt";
std::string pfc_output_file = "pfc.txt";

double alpha_resume_interval = 55, rp_timer, ewma_gain = 1 / 16;
double rate_decrease_interval = 4;
uint32_t fast_recovery_times = 5;
std::string rate_ai, rate_hai, min_rate = "100Mb/s";
std::string dctcp_rate_ai = "1000Mb/s";

bool clamp_target_rate = false, l2_back_to_zero = false;
double error_rate_per_link = 0.0;
uint32_t has_win = 1;
uint32_t global_t = 1;
uint32_t mi_thresh = 5;
bool var_win = false, fast_react = true;
bool multi_rate = true;
bool sample_feedback = false;
double pint_log_base = 1.05;
double pint_prob = 1.0;
double u_target = 0.95;
uint32_t int_multi = 1;
bool rate_bound = true;

uint32_t ack_high_prio = 0;
uint64_t link_down_time = 0;
uint32_t link_down_A = 0, link_down_B = 0;

uint32_t enable_trace = 1;

uint32_t buffer_size_MB = 16;

uint32_t qlen_dump_interval = 100000000, qlen_monitor_interval_ns = 100;
uint64_t qlen_mon_start = 2000000000, qlen_mon_end = 2100000000;
string qlen_mon_file;

unordered_map<uint64_t, uint32_t> rate2kmax, rate2kmin;
unordered_map<uint64_t, double> rate2pmax;
uint32_t offset_upbound_us = 0;

uint32_t display_config = 0;

// random offset params
uint32_t slots_num = 0;
Time slots_interval = Time(0);
/************************************************
 * Runtime varibles
 ***********************************************/
std::ifstream topof, flowf, tracef, qmonitorf;

NodeContainer n;

uint64_t nic_rate;

uint64_t maxRtt, maxBdp;

struct Interface{
	uint32_t idx;
	bool up;
	uint64_t delay;
	uint64_t bw;

	Interface() : idx(0), up(false){}
};
map<Ptr<Node>, map<Ptr<Node>, Interface> > nbr2if;
// Mapping destination to next hop for each node: <node, <dest, <nexthop0, ...> > >
map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node> > > > nextHop;
map<Ptr<Node>, map<Ptr<Node>, uint64_t> > pairDelay; // accumulated propagation delay between two nodes (on shortest path)
map<Ptr<Node>, map<Ptr<Node>, uint64_t> > pairTxDelay; // accumulated transmission delay between two nodes (on shortest path)
map<uint32_t, map<uint32_t, uint64_t> > pairBw; // smallest bandwidth between two nodes (on shortest path)
map<Ptr<Node>, map<Ptr<Node>, uint64_t> > pairBdp;
map<uint32_t, map<uint32_t, uint64_t> > pairRtt;
unordered_set<uint32_t> monitor_nodeids;

std::vector<Ipv4Address> serverAddress;

// maintain port number for each host pair
std::unordered_map<uint32_t, unordered_map<uint32_t, uint16_t> > portNumder;

FlowInputEntry flow_input = {0};
uint32_t flow_num;
shared_ptr<vector<FlowInputEntry>> flows;
// vector<shared_ptr<FlowInputEntry>>::iterator cur_flow_iter;
uint32_t cur_flow_idx = 0;

std::unordered_map<uint32_t, std::vector<uint64_t>> node2params; // node to ROCC parameters

void printVec(const vector<long double>& vec){
    for(const auto& elem : vec){
        cout << elem << " ";
    }
    cout << endl;
}

void readAllFlowInputEntrys(){
	flows = make_shared<vector<FlowInputEntry>>();
	int read_idx = 0;
	while (read_idx < flow_num){
		FlowInputEntry flow;
		double start_time_ns;
		flowf >> flow.src >> flow.dst >> flow.priority_group 
			  >> flow.dst_port >> flow.size_byte 
			  >> start_time_ns;
		flow.start_time = NanoSeconds(start_time_ns);
		flow.flow_idx = read_idx;
		flows->emplace_back(flow);
        read_idx++;
	}
	flowf.close();
}

void sortFlowsByStartTime(){
	std::sort(flows->begin(), flows->end(), 
				[](const FlowInputEntry& a, 
				   const FlowInputEntry& b){
		return a.getOffsetStart() < b.getOffsetStart();
	});
}

void ReadFlowInput(){
	if (flow_input.flow_idx < flow_num){
		double start_time_ns;
		flowf >> flow_input.src >> flow_input.dst >> flow_input.priority_group 
			  >> flow_input.dst_port >> flow_input.size_byte >> start_time_ns;
		flow_input.start_time = NanoSeconds(start_time_ns);
		NS_ASSERT(n.Get(flow_input.src)->GetNodeType() == 0 && n.Get(flow_input.dst)->GetNodeType() == 0);
	}
}

void ScheduleFlowInputs(){
	while (cur_flow_idx < flows->size() && 
		   (*flows)[cur_flow_idx].getOffsetStart() == Simulator::Now()){
		FlowInputEntry& cur_flow = (*flows)[cur_flow_idx];
		uint32_t port = portNumder[cur_flow.src][cur_flow.dst]++; // get a new port number 
		RdmaClientHelper clientHelper(
			cur_flow.priority_group, 
			serverAddress[cur_flow.src], 
			serverAddress[cur_flow.dst], port, 
			cur_flow.dst_port, 
			cur_flow.size_byte, 
			has_win?(global_t==1?
						maxBdp:pairBdp[n.Get(cur_flow.src)][n.Get(cur_flow.dst)]):0, 
			global_t==1?
				maxRtt:pairRtt[cur_flow.src][cur_flow.dst],
			cur_flow.flow_idx);
		ApplicationContainer appCon = clientHelper.Install(n.Get(cur_flow.src));
		appCon.Start(Time(0));

		cur_flow_idx++;
	}

	// schedule the next time to run this function
	if (cur_flow_idx < flows->size()){
		FlowInputEntry& cur_flow = (*flows)[cur_flow_idx];
		NS_LOG_INFO(string("schedule flow:") + cur_flow.get_str());
		Simulator::Schedule(cur_flow.getOffsetStart()-Simulator::Now(), 
							ScheduleFlowInputs);
	}
}

Ipv4Address node_id_to_ip(uint32_t id){
	return Ipv4Address(0x0b000001 + ((id / 256) * 0x00010000) + ((id % 256) * 0x00000100));
}

uint32_t ip_to_node_id(Ipv4Address ip){
	return (ip.Get() >> 8) & 0xffff;
}

void qp_finish(FILE* fout, Ptr<RdmaQueuePair> q){
	uint32_t sid = ip_to_node_id(q->sip), did = ip_to_node_id(q->dip);
	uint64_t base_rtt = pairRtt[sid][did], b = pairBw[sid][did];
	// total payload size plus the header size
	uint32_t total_bytes = q->m_size + ((q->m_size-1) / packet_payload_size + 1) * (CustomHeader::GetStaticWholeHeaderSize() - IntHeader::GetStaticSize()); // translate to the minimum bytes required (with header but no INT)
	// complete fct
	uint64_t complete_fct_ns = (Simulator::Now() - q->startTime).GetNanoSeconds();
	// fct + offset
	FlowInputEntry& cur_flow = (*flows)[q->flow_id];
	uint64_t offsetted_fct_ns = complete_fct_ns + cur_flow.offset.GetNanoSeconds();
	// standalone fct only includes the transmission delay + propagation delay (base_rtt), plus one transmission delay of all data? problem? 
	uint64_t standalone_fct_ns = base_rtt + total_bytes * 8000000000lu / b; // fct in unit of ns
	// sip, dip, src_port, dst_port, size (B), start_time, fct (ns), offseted_fct(ns), standalone_fct (ns), end(ns)
	fprintf(fout, "%08x,%08x,%u,%u,%lu,%lu,%lu,%lu,%lu,%lu\n", 
					q->sip.Get(), q->dip.Get(), q->sport, q->dport, q->m_size, q->startTime.GetNanoSeconds(), 
					complete_fct_ns, offsetted_fct_ns, standalone_fct_ns, Simulator::Now().GetNanoSeconds());
	fflush(fout);

	// remove rxQp from the receiver
	Ptr<Node> dstNode = n.Get(did);
	Ptr<RdmaDriver> rdma = dstNode->GetObject<RdmaDriver> ();
	rdma->m_rdma->DeleteRxQp(q->sip.Get(), q->m_pg, q->sport);
}

void get_pfc(FILE* fout, Ptr<QbbNetDevice> dev, uint32_t type){
	fprintf(fout, "%lu %u %u %u %u\n", Simulator::Now().GetTimeStep(), dev->GetNode()->GetId(), dev->GetNode()->GetNodeType(), dev->GetIfIndex(), type);
}

struct QlenDistribution{
	vector<uint32_t> cnt; // cnt[i] is the number of times that the queue len is i KB

	void add(uint32_t qlen){
		uint32_t kb = qlen / 1000;
		if (cnt.size() < kb+1)
			cnt.resize(kb+1);
		cnt[kb]++;
	}
};

map<uint32_t, map<uint32_t, QlenDistribution> > queue_result;
void monitor_buffer(FILE* qlen_output, NodeContainer *n){
	for (uint32_t i = 0; i < n->GetN(); i++){
		if (n->Get(i)->GetNodeType() == 1 && 
			monitor_nodeids.find(i) != monitor_nodeids.end()){ 
			// is switch and is registed as need to monitor
			Ptr<SwitchNode> sw = DynamicCast<SwitchNode>(n->Get(i));
			if (queue_result.find(i) == queue_result.end())
				queue_result[i];
			for (uint32_t j = 1; j < sw->GetNDevices(); j++){
				uint32_t size = 0;
				for (uint32_t k = 0; k < SwitchMmu::qCnt; k++)
					size += sw->m_mmu->egress_bytes[j][k];
				queue_result[i][j].add(size);
			}
		}
	}

	if (Simulator::Now().GetTimeStep() % qlen_dump_interval == 0){
		fprintf(qlen_output, "Node Port KB:Count\n");
		fprintf(qlen_output, "time: %lu us\n", Simulator::Now().GetMicroSeconds());
		for (auto &it0 : queue_result)
			for (auto &it1 : it0.second){
				fprintf(qlen_output, "%u %u", it0.first, it1.first);
				auto &dist = it1.second.cnt;
				for (uint32_t i = 0; i < dist.size(); i++){
					if(dist[i] == 0){
						continue;
					} else {
						fprintf(qlen_output, " %u:%u", i, dist[i]);
					}
				}
				fprintf(qlen_output, "\n");
			}
		fflush(qlen_output);
	}
	if (Simulator::Now().GetTimeStep() < qlen_mon_end)
		Simulator::Schedule(NanoSeconds(qlen_monitor_interval_ns), &monitor_buffer, qlen_output, n);
}

void CalculateRoute(Ptr<Node> host){
	// queue for the BFS.
	vector<Ptr<Node> > q;
	// Distance from the host to each node.
	map<Ptr<Node>, int> dis;
	map<Ptr<Node>, uint64_t> delay;
	map<Ptr<Node>, uint64_t> txDelay;
	map<Ptr<Node>, uint64_t> bw;
	// init BFS.
	q.push_back(host);
	dis[host] = 0;
	delay[host] = 0;
	txDelay[host] = 0;
	bw[host] = 0xfffffffffffffffflu;
	// BFS.
	for (int i = 0; i < (int)q.size(); i++){
		Ptr<Node> now = q[i];
		int d = dis[now];
		for (auto it = nbr2if[now].begin(); it != nbr2if[now].end(); it++){
			// skip down link
			if (!it->second.up)
				continue;
			Ptr<Node> next = it->first;
			// If 'next' have not been visited.
			if (dis.find(next) == dis.end()){
				dis[next] = d + 1;
				delay[next] = delay[now] + it->second.delay;
				txDelay[next] = txDelay[now] + packet_payload_size * 1000000000lu * 8 / it->second.bw;
				bw[next] = std::min(bw[now], it->second.bw);
				// we only enqueue switch, because we do not want packets to go through host as middle point
				if (next->GetNodeType() == 1)
					q.push_back(next);
			}
			// if 'now' is on the shortest path from 'next' to 'host'.
			if (d + 1 == dis[next]){
				nextHop[next][host].push_back(now);
			}
		}
	}
	for (auto it : delay)
		pairDelay[it.first][host] = it.second;
	for (auto it : txDelay)
		pairTxDelay[it.first][host] = it.second;
	for (auto it : bw)
		pairBw[it.first->GetId()][host->GetId()] = it.second;
}

void print_route(map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node> > > >& nextHop){
	ofstream route_file("route.txt");
	for(const auto& src_dst_path_pair : nextHop){
		for(const auto& dst_path_pair : src_dst_path_pair.second){
			uint32_t src_id = src_dst_path_pair.first->GetId();
			uint32_t dst_id = dst_path_pair.first->GetId();
			route_file << src_id << "->" << dst_id << ": ";	
			for(const auto& path_node : dst_path_pair.second){
				route_file << path_node->GetId() << " "; 
			}
			route_file << endl;
		}
	}
	route_file.close();
}

void CalculateRoutes(NodeContainer &n){
	for (int i = 0; i < (int)n.GetN(); i++){
		Ptr<Node> node = n.Get(i);
		if (node->GetNodeType() == 0)
			CalculateRoute(node);
	}
}

void SetRoutingEntries(){
	// For each node.
	for (auto i = nextHop.begin(); i != nextHop.end(); i++){
		Ptr<Node> node = i->first;
		auto &table = i->second;
		for (auto j = table.begin(); j != table.end(); j++){
			// The destination node.
			Ptr<Node> dst = j->first;
			// The IP address of the dst.
			Ipv4Address dstAddr = dst->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
			// The next hops towards the dst.
			vector<Ptr<Node> > nexts = j->second;
			for (int k = 0; k < (int)nexts.size(); k++){
				Ptr<Node> next = nexts[k];
				uint32_t interface = nbr2if[node][next].idx;
				if (node->GetNodeType() == 1)
					DynamicCast<SwitchNode>(node)->AddTableEntry(dstAddr, interface);
				else{
					node->GetObject<RdmaDriver>()->m_rdma->AddTableEntry(dstAddr, interface);
				}
			}
		}
	}
}

// take down the link between a and b, and redo the routing
void TakeDownLink(NodeContainer n, Ptr<Node> a, Ptr<Node> b){
	if (!nbr2if[a][b].up)
		return;
	// take down link between a and b
	nbr2if[a][b].up = nbr2if[b][a].up = false;
	nextHop.clear();
	CalculateRoutes(n);
	// clear routing tables
	for (uint32_t i = 0; i < n.GetN(); i++){
		if (n.Get(i)->GetNodeType() == 1)
			DynamicCast<SwitchNode>(n.Get(i))->ClearTable();
		else
			n.Get(i)->GetObject<RdmaDriver>()->m_rdma->ClearTable();
	}
	DynamicCast<QbbNetDevice>(a->GetDevice(nbr2if[a][b].idx))->TakeDown();
	DynamicCast<QbbNetDevice>(b->GetDevice(nbr2if[b][a].idx))->TakeDown();
	// reset routing table
	SetRoutingEntries();

	// redistribute qp on each host
	for (uint32_t i = 0; i < n.GetN(); i++){
		if (n.Get(i)->GetNodeType() == 0)
			n.Get(i)->GetObject<RdmaDriver>()->m_rdma->RedistributeQp();
	}
}

uint64_t get_nic_rate(NodeContainer &n){
	for (uint32_t i = 0; i < n.GetN(); i++)
		if (n.Get(i)->GetNodeType() == 0)
			return DynamicCast<QbbNetDevice>(n.Get(i)->GetDevice(1))->GetDataRate().GetBitRate();
}

void log_flow_schedule(Time now, Time sched_after, FlowInputEntry& flow){
	stringstream ss;
	ss << "SimTime:" << now << " After:" << sched_after
		<< " WillInsertFlow:" << flow;
	NS_LOG_INFO(ss.str());
}

void log_cur_time(){
	stringstream ss;
	ss << "Sim Time:" << Simulator::Now();
	NS_LOG_INFO(ss.str());
}

NodeContainer get_node_container_from_file(ifstream& ifs, uint32_t node_num){
	NodeContainer nodes;
	for (uint32_t i = 0; i < node_num; i++)
	{
		uint32_t nid;
		ifs >> nid;
		if (nid >= n.GetN()){
			continue;
		}
		nodes = NodeContainer(nodes, n.Get(nid));
	}
	return nodes;
}

unordered_set<uint32_t> get_nodeid_set_from_file(ifstream& ifs, uint32_t node_num){
	unordered_set<uint32_t> nodeid_set;
	for (uint32_t i = 0; i < node_num; i++)
	{
		uint32_t nid;
		ifs >> nid;
		if (nid >= n.GetN()){
			continue;
		}
		nodeid_set.insert(nid);
	}
	return nodeid_set;
}

// void read_RandOffset_param(ifstream& ifs){
// 	uint32_t slots_num, slot_interval_us;
// 	ifs >> slots_num >> slot_interval_us;
// 	// for(uint32_t i=0; i<node_num; i++){
// 	// 	uint32_t node_id;
// 	// 	ifs >> node_id;
// 	// 	for(uint32_t j=0; j<param_num; j++){
// 	// 		uint64_t param;
// 	// 		ifs >> param;
// 	// 		node2params[node_id].push_back(param);
// 	// 	}
// 	// }
// }


Ptr<QbbNetDevice> getQbbDevFromRdmaDriver(Ptr<RdmaDriver> driver){
	Ptr<QbbNetDevice> dev = NULL;
	for (uint32_t i = 0; i < driver->m_node->GetNDevices(); i++){
		if (driver->m_node->GetDevice(i)->IsQbb()){
			dev = DynamicCast<QbbNetDevice>(driver->m_node->GetDevice(i));
			break;
		}
	}
	return dev;
}	

int main(int argc, char *argv[])
{
	clock_t begint, endt;
	begint = clock();
    cout << "third app. argv[1]:" << argv[1] 
		<< " argc:" << argc << endl;
	std::stringstream ss;

	LogComponentEnable("GENERIC_SIMULATION", LOG_LEVEL_LOGIC);
#ifndef PGO_TRAINING
	if (argc > 1)
#else
	if (true)
#endif
	{
		//Read the configuration file
		std::ifstream conf;
#ifndef PGO_TRAINING
		conf.open(argv[1]);
#else
		conf.open(PATH_TO_PGO_CONFIG);
#endif
		//TODO: will dead loop if file does not exist
		// warn if no file
		while (!conf.eof())
		{
			std::string key;
			conf >> key;

			//std::cout << conf.cur << "\n";

			if (key.compare("ENABLE_QBB") == 0)
			{
				uint32_t v;
				conf >> v;
				enable_qbb = v;
				if (enable_qbb)
					ss << "ENABLE_QBB\t\t\t" << "Yes" << "\n";
				else
					ss << "ENABLE_QBB\t\t\t" << "No" << "\n";
			}
			else if (key.compare("ENABLE_QCN") == 0)
			{
				uint32_t v;
				conf >> v;
				enable_qcn = v;
				if (enable_qcn)
					ss << "ENABLE_QCN\t\t\t" << "Yes" << "\n";
				else
					ss << "ENABLE_QCN\t\t\t" << "No" << "\n";
			}
			else if (key.compare("ENABLE_PFC") == 0)
			{
				uint32_t v;
				conf >> v;
				enable_pfc = v;
				if (enable_pfc)
					ss << "ENABLE_PFC\t\t\t" << "Yes" << "\n";
				else
					ss << "ENABLE_PFC\t\t\t" << "No" << "\n";
			}
			else if (key.compare("USE_DYNAMIC_PFC_THRESHOLD") == 0)
			{
				uint32_t v;
				conf >> v;
				use_dynamic_pfc_threshold = v;
				if (use_dynamic_pfc_threshold)
					ss << "USE_DYNAMIC_PFC_THRESHOLD\t" << "Yes" << "\n";
				else
					ss << "USE_DYNAMIC_PFC_THRESHOLD\t" << "No" << "\n";
			}
			else if (key.compare("CLAMP_TARGET_RATE") == 0)
			{
				uint32_t v;
				conf >> v;
				clamp_target_rate = v;
				if (clamp_target_rate)
					ss << "CLAMP_TARGET_RATE\t\t" << "Yes" << "\n";
				else
					ss << "CLAMP_TARGET_RATE\t\t" << "No" << "\n";
			}
			else if (key.compare("PAUSE_TIME") == 0)
			{
				double v;
				conf >> v;
				pause_time = v;
				ss << "PAUSE_TIME\t\t\t" << pause_time << "\n";
			}
			else if (key.compare("DATA_RATE") == 0)
			{
				std::string v;
				conf >> v;
				data_rate = v;
				ss << "DATA_RATE\t\t\t" << data_rate << "\n";
			}
			else if (key.compare("LINK_DELAY") == 0)
			{
				std::string v;
				conf >> v;
				link_delay = v;
				ss << "LINK_DELAY\t\t\t" << link_delay << "\n";
			}
			else if (key.compare("PACKET_PAYLOAD_SIZE") == 0)
			{
				uint32_t v;
				conf >> v;
				packet_payload_size = v;
				ss << "PACKET_PAYLOAD_SIZE\t\t" << packet_payload_size << "\n";
			}
			else if (key.compare("L2_CHUNK_SIZE") == 0)
			{
				uint32_t v;
				conf >> v;
				l2_chunk_size = v;
				ss << "L2_CHUNK_SIZE\t\t\t" << l2_chunk_size << "\n";
			}
			else if (key.compare("L2_ACK_INTERVAL") == 0)
			{
				uint32_t v;
				conf >> v;
				l2_ack_interval = v;
				ss << "L2_ACK_INTERVAL\t\t\t" << l2_ack_interval << "\n";
			}
			else if (key.compare("L2_BACK_TO_ZERO") == 0)
			{
				uint32_t v;
				conf >> v;
				l2_back_to_zero = v;
				if (l2_back_to_zero)
					ss << "L2_BACK_TO_ZERO\t\t\t" << "Yes" << "\n";
				else
					ss << "L2_BACK_TO_ZERO\t\t\t" << "No" << "\n";
			}
			else if (key.compare("TOPOLOGY_FILE") == 0)
			{
				std::string v;
				conf >> v;
				topology_file = v;
				ss << "TOPOLOGY_FILE\t\t\t" << topology_file << "\n";
			}
			else if (key.compare("FLOW_FILE") == 0)
			{
				std::string v;
				conf >> v;
				flow_file = v;
				ss << "FLOW_FILE\t\t\t" << flow_file << "\n";
			}
			else if (key.compare("TRACE_FILE") == 0)
			{
				std::string v;
				conf >> v;
				trace_file = v;
				ss << "TRACE_FILE\t\t\t" << trace_file << "\n";
			}
			else if (key.compare("QUEUE_MONITOR_FILE") == 0)
			{
				std::string v;
				conf >> v;
				monitor_file = v;
				ss << "QUEUE_MONITOR_FILE\t\t\t" << monitor_file << "\n";
			}
			else if (key.compare("RANDOM_PARAM_FILE") == 0)
			{
				std::string v;
				conf >> v;
				rand_param_file = v;
				ss << "RANDOM_PARAM_FILE\t\t\t" << rand_param_file << "\n";
			}
			
			else if (key.compare("DISPLAY_CONFIG") == 0)
			{
				std::int32_t v;
				conf >> v;
				display_config = v;
				ss << "DISPLAY_CONFIG\t\t\t" << display_config << "\n";
			}
			else if (key.compare("TRACE_OUTPUT_FILE") == 0)
			{
				std::string v;
				conf >> v;
				trace_output_file = v;
				if (argc > 2)
				{
					trace_output_file = trace_output_file + std::string(argv[2]);
				}
				ss << "TRACE_OUTPUT_FILE\t\t" << trace_output_file << "\n";
			}
			else if (key.compare("SIMULATOR_STOP_TIME") == 0)
			{
				double v;
				conf >> v;
				simulator_stop_time = v;
				ss << "SIMULATOR_STOP_TIME\t\t" << simulator_stop_time << "\n";
			}
			else if (key.compare("ALPHA_RESUME_INTERVAL") == 0)
			{
				double v;
				conf >> v;
				alpha_resume_interval = v;
				ss << "ALPHA_RESUME_INTERVAL\t\t" << alpha_resume_interval << "\n";
			}
			else if (key.compare("RP_TIMER") == 0)
			{
				double v;
				conf >> v;
				rp_timer = v;
				ss << "RP_TIMER\t\t\t" << rp_timer << "\n";
			}
			else if (key.compare("EWMA_GAIN") == 0)
			{
				double v;
				conf >> v;
				ewma_gain = v;
				ss << "EWMA_GAIN\t\t\t" << ewma_gain << "\n";
			}
			else if (key.compare("FAST_RECOVERY_TIMES") == 0)
			{
				uint32_t v;
				conf >> v;
				fast_recovery_times = v;
				ss << "FAST_RECOVERY_TIMES\t\t" << fast_recovery_times << "\n";
			}
			else if (key.compare("RATE_AI") == 0)
			{
				std::string v;
				conf >> v;
				rate_ai = v;
				ss << "RATE_AI\t\t\t\t" << rate_ai << "\n";
			}
			else if (key.compare("RATE_HAI") == 0)
			{
				std::string v;
				conf >> v;
				rate_hai = v;
				ss << "RATE_HAI\t\t\t" << rate_hai << "\n";
			}
			else if (key.compare("ERROR_RATE_PER_LINK") == 0)
			{
				double v;
				conf >> v;
				error_rate_per_link = v;
				ss << "ERROR_RATE_PER_LINK\t\t" << error_rate_per_link << "\n";
			}
			else if (key.compare("CC_MODE") == 0){
				conf >> cc_mode;
				ss << "CC_MODE\t\t" << cc_mode << '\n';
			} 
			else if (key.compare("ENABLE_RANDOFFSET") == 0){
				conf >> enable_randoffset;
				ss << "ENABLE_RANDOFFSET\t\t" << enable_randoffset << '\n';
			}else if (key.compare("RATE_DECREASE_INTERVAL") == 0){
				double v;
				conf >> v;
				rate_decrease_interval = v;
				ss << "RATE_DECREASE_INTERVAL\t\t" << rate_decrease_interval << "\n";
			}else if (key.compare("MIN_RATE") == 0){
				conf >> min_rate;
				ss << "MIN_RATE\t\t" << min_rate << "\n";
			}else if (key.compare("FCT_OUTPUT_FILE") == 0){
				conf >> fct_output_file;
				ss << "FCT_OUTPUT_FILE\t\t" << fct_output_file << '\n';
			}else if (key.compare("HAS_WIN") == 0){
				conf >> has_win;
				ss << "HAS_WIN\t\t" << has_win << "\n";
			}else if (key.compare("GLOBAL_T") == 0){
				conf >> global_t;
				ss << "GLOBAL_T\t\t" << global_t << '\n';
			}else if (key.compare("MI_THRESH") == 0){
				conf >> mi_thresh;
				ss << "MI_THRESH\t\t" << mi_thresh << '\n';
			}else if (key.compare("VAR_WIN") == 0){
				uint32_t v;
				conf >> v;
				var_win = v;
				ss << "VAR_WIN\t\t" << v << '\n';
			}else if (key.compare("FAST_REACT") == 0){
				uint32_t v;
				conf >> v;
				fast_react = v;
				ss << "FAST_REACT\t\t" << v << '\n';
			}else if (key.compare("U_TARGET") == 0){
				conf >> u_target;
				ss << "U_TARGET\t\t" << u_target << '\n';
			}else if (key.compare("INT_MULTI") == 0){
				conf >> int_multi;
				ss << "INT_MULTI\t\t\t\t" << int_multi << '\n';
			}else if (key.compare("RATE_BOUND") == 0){
				uint32_t v;
				conf >> v;
				rate_bound = v;
				ss << "RATE_BOUND\t\t" << rate_bound << '\n';
			}else if (key.compare("ACK_HIGH_PRIO") == 0){
				conf >> ack_high_prio;
				ss << "ACK_HIGH_PRIO\t\t" << ack_high_prio << '\n';
			}else if (key.compare("DCTCP_RATE_AI") == 0){
				conf >> dctcp_rate_ai;
				ss << "DCTCP_RATE_AI\t\t\t\t" << dctcp_rate_ai << "\n";
			}else if (key.compare("PFC_OUTPUT_FILE") == 0){
				conf >> pfc_output_file;
				ss << "PFC_OUTPUT_FILE\t\t\t\t" << pfc_output_file << '\n';
			}else if (key.compare("LINK_DOWN") == 0){
				conf >> link_down_time >> link_down_A >> link_down_B;
				ss << "LINK_DOWN\t\t\t\t" << link_down_time << ' '<< link_down_A << ' ' << link_down_B << '\n';
			}else if (key.compare("ENABLE_TRACE") == 0){
				conf >> enable_trace;
				ss << "ENABLE_TRACE\t\t\t\t" << enable_trace << '\n';
			}else if (key.compare("KMAX_MAP") == 0){
				int n_k ;
				conf >> n_k;
				ss << "KMAX_MAP\t\t\t\t";
				for (int i = 0; i < n_k; i++){
					uint64_t rate;
					uint32_t k;
					conf >> rate >> k;
					rate2kmax[rate] = k;
					ss << ' ' << rate << ' ' << k;
				}
				ss<<'\n';
			}else if (key.compare("KMIN_MAP") == 0){
				int n_k ;
				conf >> n_k;
				ss << "KMIN_MAP\t\t\t\t";
				for (int i = 0; i < n_k; i++){
					uint64_t rate;
					uint32_t k;
					conf >> rate >> k;
					rate2kmin[rate] = k;
					ss << ' ' << rate << ' ' << k;
				}
				ss<<'\n';
			}else if (key.compare("PMAX_MAP") == 0){
				int n_k ;
				conf >> n_k;
				ss << "PMAX_MAP\t\t\t\t";
				for (int i = 0; i < n_k; i++){
					uint64_t rate;
					double p;
					conf >> rate >> p;
					rate2pmax[rate] = p;
					ss << ' ' << rate << ' ' << p;
				}
				ss<<'\n';
			}else if (key.compare("BUFFER_SIZE") == 0){
				conf >> buffer_size_MB;
				ss << "BUFFER_SIZE\t\t\t\t" << buffer_size_MB << '\n';
			}else if (key.compare("QLEN_MON_FILE") == 0){
				conf >> qlen_mon_file;
				ss << "QLEN_MON_FILE\t\t\t\t" << qlen_mon_file << '\n';
			}else if (key.compare("QLEN_MON_START") == 0){
				conf >> qlen_mon_start;
				ss << "QLEN_MON_START\t\t\t\t" << qlen_mon_start << '\n';
			}else if (key.compare("QLEN_MON_END") == 0){
				conf >> qlen_mon_end;
				ss << "QLEN_MON_END\t\t\t\t" << qlen_mon_end << '\n';
			}else if (key.compare("QLEN_MON_INTV_NS") == 0){
				uint32_t v;
				conf >> v;
				qlen_monitor_interval_ns = v;
				ss << "QLEN_MON_INTV_NS\t\t\t" << qlen_monitor_interval_ns << "\n";
			}else if (key.compare("QLEN_MON_DUMP_INTV_NS") == 0){
				uint32_t v;
				conf >> v;
				qlen_dump_interval = v;
				ss << "QLEN_MON_DUMP_INTV_NS\t\t\t" << qlen_dump_interval << "\n";
			}else if (key.compare("MULTI_RATE") == 0){
				int v;
				conf >> v;
				multi_rate = v;
				ss << "MULTI_RATE\t\t\t\t" << multi_rate << '\n';
			}else if (key.compare("SAMPLE_FEEDBACK") == 0){
				int v;
				conf >> v;
				sample_feedback = v;
				ss << "SAMPLE_FEEDBACK\t\t\t\t" << sample_feedback << '\n';
			}else if(key.compare("PINT_LOG_BASE") == 0){
				conf >> pint_log_base;
				ss << "PINT_LOG_BASE\t\t\t\t" << pint_log_base << '\n';
			}else if (key.compare("PINT_PROB") == 0){
				conf >> pint_prob;
				ss << "PINT_PROB\t\t\t\t" << pint_prob << '\n';
			}else if (key.compare("RANDOFFSET_PARAMS") == 0){
				double slots_num_d, slots_interval_us_d;
				conf >> slots_num_d >> slots_interval_us_d;
				slots_num = uint32_t(slots_num_d);
				slots_interval = MicroSeconds((slots_interval_us_d));
				ss << "PINT_PROB\t\t\t\t" << pint_prob << '\n';
			}		
			fflush(stdout);
		}
		conf.close();
	}
	else
	{
		std::cout << "Error: require a config file\n";
		fflush(stdout);
		return 1;
	}

	if(display_config){
		std::cout << ss.str() << endl;
	}


	bool dynamicth = use_dynamic_pfc_threshold;

	Config::SetDefault("ns3::QbbNetDevice::PauseTime", UintegerValue(pause_time));
	Config::SetDefault("ns3::QbbNetDevice::QcnEnabled", BooleanValue(enable_qcn));
	Config::SetDefault("ns3::QbbNetDevice::QbbEnabled", BooleanValue(enable_qbb));
	Config::SetDefault("ns3::QbbNetDevice::DynamicThreshold", BooleanValue(dynamicth));

	// set int_multi
	IntHop::multi = int_multi;
	// IntHeader::mode
	if (cc_mode == 7) // timely, use ts
		IntHeader::mode = IntHeader::TS;
	else if (cc_mode == 3) // hpcc, use int
		IntHeader::mode = IntHeader::NORMAL;
	else if (cc_mode == 10) // hpcc-pint
		IntHeader::mode = IntHeader::PINT;
	else if (cc_mode == 12)
		// IntHeader::mode = IntHeader::RAND_OFFSET;
		throw runtime_error("RAND_OFFSET not a cc mode");
	else // others, no extra header
		IntHeader::mode = IntHeader::NONE;

	// Set Pint
	if (cc_mode == 10){
		Pint::set_log_base(pint_log_base);
		IntHeader::pint_bytes = Pint::get_n_bytes();
		printf("PINT bits: %d bytes: %d\n", Pint::get_n_bits(), Pint::get_n_bytes());
	}

	//SeedManager::SetSeed(time(NULL));
	topof.open(topology_file.c_str());
	flowf.open(flow_file.c_str());
	tracef.open(trace_file.c_str());
	qmonitorf.open(monitor_file.c_str());

	uint32_t node_num, switch_num, link_num, trace_num, monitor_num;
	topof >> node_num >> switch_num >> link_num;
	flowf >> flow_num;
	tracef >> trace_num;
	qmonitorf >> monitor_num;


	//n.Create(node_num);
	std::vector<uint32_t> node_type(node_num, 0); // 0: server, 1: switch
	for (uint32_t i = 0; i < switch_num; i++)
	{
		uint32_t sid;
		topof >> sid; // node switch ID
		node_type[sid] = 1; // mark as switch
	}
	for (uint32_t i = 0; i < node_num; i++){ // create servers and switches
		if (node_type[i] == 0)
			n.Add(CreateObject<Node>());
		else{
			Ptr<SwitchNode> sw = CreateObject<SwitchNode>();
			n.Add(sw);
			sw->SetAttribute("EcnEnabled", BooleanValue(enable_qcn));
			sw->SetAttribute("QbbEnabled", BooleanValue(enable_qbb));
		}
	}


	NS_LOG_INFO("Create nodes.");

	InternetStackHelper internet;
	internet.Install(n);

	//
	// Assign IP to each server
	//
	for (uint32_t i = 0; i < node_num; i++){
		if (n.Get(i)->GetNodeType() == 0){ // is server
			serverAddress.resize(i + 1);
			serverAddress[i] = node_id_to_ip(i);
		}
	}

	NS_LOG_INFO("Create channels.");

	//
	// Explicitly create the channels required by the topology.
	//

	Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
	Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
	rem->SetRandomVariable(uv);
	uv->SetStream(50);
	rem->SetAttribute("ErrorRate", DoubleValue(error_rate_per_link));
	rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));

	FILE *pfc_file = fopen(pfc_output_file.c_str(), "w");

	QbbHelper qbb;
	Ipv4AddressHelper ipv4;
	for (uint32_t i = 0; i < link_num; i++)
	{
		uint32_t src, dst;
		std::string data_rate, link_delay;
		double error_rate;
		topof >> src >> dst >> data_rate >> link_delay >> error_rate;

		Ptr<Node> snode = n.Get(src), dnode = n.Get(dst);

		qbb.SetDeviceAttribute("DataRate", StringValue(data_rate));
		qbb.SetChannelAttribute("Delay", StringValue(link_delay));

		if (error_rate > 0)
		{
			Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
			Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
			rem->SetRandomVariable(uv);
			uv->SetStream(50);
			rem->SetAttribute("ErrorRate", DoubleValue(error_rate));
			rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
			qbb.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));
		}
		else
		{
			qbb.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));
		}

		fflush(stdout);

		// Assigne server IP
		// Note: this should be before the automatic assignment below (ipv4.Assign(d)),
		// because we want our IP to be the primary IP (first in the IP address list),
		// so that the global routing is based on our IP
		NetDeviceContainer d = qbb.Install(snode, dnode);
		if (snode->GetNodeType() == 0){
			Ptr<Ipv4> ipv4 = snode->GetObject<Ipv4>();
			ipv4->AddInterface(d.Get(0));
			ipv4->AddAddress(1, Ipv4InterfaceAddress(serverAddress[src], Ipv4Mask(0xff000000)));
		}
		if (dnode->GetNodeType() == 0){
			Ptr<Ipv4> ipv4 = dnode->GetObject<Ipv4>();
			ipv4->AddInterface(d.Get(1));
			ipv4->AddAddress(1, Ipv4InterfaceAddress(serverAddress[dst], Ipv4Mask(0xff000000)));
		}

		// used to create a graph of the topology
		nbr2if[snode][dnode].idx = DynamicCast<QbbNetDevice>(d.Get(0))->GetIfIndex();
		nbr2if[snode][dnode].up = true;
		// propagation delay in ns
		nbr2if[snode][dnode].delay = DynamicCast<QbbChannel>(DynamicCast<QbbNetDevice>(d.Get(0))->GetChannel())->GetDelay().GetTimeStep();
		nbr2if[snode][dnode].bw = DynamicCast<QbbNetDevice>(d.Get(0))->GetDataRate().GetBitRate();
		nbr2if[dnode][snode].idx = DynamicCast<QbbNetDevice>(d.Get(1))->GetIfIndex();
		nbr2if[dnode][snode].up = true;
		nbr2if[dnode][snode].delay = DynamicCast<QbbChannel>(DynamicCast<QbbNetDevice>(d.Get(1))->GetChannel())->GetDelay().GetTimeStep();
		nbr2if[dnode][snode].bw = DynamicCast<QbbNetDevice>(d.Get(1))->GetDataRate().GetBitRate();

		// This is just to set up the connectivity between nodes. The IP addresses are useless
		char ipstring[16];
		sprintf(ipstring, "10.%d.%d.0", i / 254 + 1, i % 254 + 1);
		ipv4.SetBase(ipstring, "255.255.255.0");
		ipv4.Assign(d);

		// setup PFC trace
		DynamicCast<QbbNetDevice>(d.Get(0))->TraceConnectWithoutContext("QbbPfc", MakeBoundCallback (&get_pfc, pfc_file, DynamicCast<QbbNetDevice>(d.Get(0))));
		DynamicCast<QbbNetDevice>(d.Get(1))->TraceConnectWithoutContext("QbbPfc", MakeBoundCallback (&get_pfc, pfc_file, DynamicCast<QbbNetDevice>(d.Get(1))));
	}

	nic_rate = get_nic_rate(n);

	// config switch
	for (uint32_t i = 0; i < node_num; i++){
		if (n.Get(i)->GetNodeType() == 1){ // is switch
			Ptr<SwitchNode> sw = DynamicCast<SwitchNode>(n.Get(i));
			uint32_t sw_port_num = sw->GetNDevices();
			uint32_t shift = 4; // by default 1/16; larger shift, smaller 1/2^shift, smaller pfc threshold, more aggressive pfc
			// if (sw_port_num >= 16){
			// 	shift = 7; // 1/128
			// } else {
			// 	shift = 8; // 1/256
			// }
			for (uint32_t j = 1; j < sw->GetNDevices(); j++){
				Ptr<QbbNetDevice> dev = DynamicCast<QbbNetDevice>(sw->GetDevice(j));
				uint64_t rate = dev->GetDataRate().GetBitRate();
				// set ecn
				NS_ASSERT_MSG(rate2kmin.find(rate) != rate2kmin.end(), "must set kmin for each link speed");
				NS_ASSERT_MSG(rate2kmax.find(rate) != rate2kmax.end(), "must set kmax for each link speed");
				NS_ASSERT_MSG(rate2pmax.find(rate) != rate2pmax.end(), "must set pmax for each link speed");
				sw->m_mmu->ConfigEcn(j, rate2kmin[rate], rate2kmax[rate], rate2pmax[rate]);
				// // set pfc
				// uint64_t delay = DynamicCast<QbbChannel>(dev->GetChannel())->GetDelay().GetTimeStep();
				// uint32_t headroom = rate * delay / 8 / 1000000000 * 3; 
				// sw->m_mmu->ConfigHdrm(j, headroom);

				// port pfc hdrm = buffer * 10%
				// uint32_t hdrm_bytes = buffer_size_MB * 1024 * 1024 * 0.1;
				uint32_t hdrm_bytes = 1024 * 1024; // 1MB
				sw->m_mmu->ConfigHdrm(j, hdrm_bytes);
				// port pfc 
				// set pfc alpha, proportional to link bw
				sw->m_mmu->pfc_a_shift[j] = shift;
				while (rate > nic_rate && sw->m_mmu->pfc_a_shift[j] > 0){
					sw->m_mmu->pfc_a_shift[j]--;
					rate /= 2;
				}
			}
			sw->m_mmu->ConfigNPort(sw->GetNDevices()-1);
			sw->m_mmu->ConfigBufferSizeByte(buffer_size_MB* 1024 * 1024);
			sw->m_mmu->node_id = sw->GetId();
			sw->m_mmu->ConfigEnablePFC(enable_pfc);
		}
	}

	#if ENABLE_QP
	FILE *fct_output = fopen(fct_output_file.c_str(), "w");
	fprintf(fct_output, "src_ip,dst_ip,sport,dport,m_size(bytes),start(ns),complete_fct(ns),offseted_fct(ns),standalone_fct(ns),end(ns)\n");
	//
	// install RDMA driver
	//
	for (uint32_t i = 0; i < node_num; i++){
		if (n.Get(i)->GetNodeType() == 0){ // is server
			// create RdmaHw
			Ptr<RdmaHw> rdmaHw = CreateObject<RdmaHw>();
			rdmaHw->SetAttribute("ClampTargetRate", BooleanValue(clamp_target_rate));
			rdmaHw->SetAttribute("AlphaResumInterval", DoubleValue(alpha_resume_interval));
			rdmaHw->SetAttribute("RPTimer", DoubleValue(rp_timer));
			rdmaHw->SetAttribute("FastRecoveryTimes", UintegerValue(fast_recovery_times));
			rdmaHw->SetAttribute("EwmaGain", DoubleValue(ewma_gain));
			rdmaHw->SetAttribute("RateAI", DataRateValue(DataRate(rate_ai)));
			rdmaHw->SetAttribute("RateHAI", DataRateValue(DataRate(rate_hai)));
			rdmaHw->SetAttribute("L2BackToZero", BooleanValue(l2_back_to_zero));
			rdmaHw->SetAttribute("L2ChunkSize", UintegerValue(l2_chunk_size));
			rdmaHw->SetAttribute("L2AckInterval", UintegerValue(l2_ack_interval));
			rdmaHw->SetAttribute("CcMode", UintegerValue(cc_mode));
			rdmaHw->SetAttribute("RateDecreaseInterval", DoubleValue(rate_decrease_interval));
			rdmaHw->SetAttribute("MinRate", DataRateValue(DataRate(min_rate)));
			rdmaHw->SetAttribute("Mtu", UintegerValue(packet_payload_size));
			rdmaHw->SetAttribute("MiThresh", UintegerValue(mi_thresh));
			rdmaHw->SetAttribute("VarWin", BooleanValue(var_win));
			rdmaHw->SetAttribute("FastReact", BooleanValue(fast_react));
			rdmaHw->SetAttribute("MultiRate", BooleanValue(multi_rate));
			rdmaHw->SetAttribute("SampleFeedback", BooleanValue(sample_feedback));
			rdmaHw->SetAttribute("TargetUtil", DoubleValue(u_target));
			rdmaHw->SetAttribute("RateBound", BooleanValue(rate_bound));
			rdmaHw->SetAttribute("DctcpRateAI", DataRateValue(DataRate(dctcp_rate_ai)));
			rdmaHw->SetPintSmplThresh(pint_prob);
			// create and install RdmaDriver
			Ptr<RdmaDriver> rdma_driver = CreateObject<RdmaDriver>();
			Ptr<Node> node = n.Get(i);
			rdma_driver->SetNode(node);
			rdma_driver->SetRdmaHw(rdmaHw);

			node->AggregateObject (rdma_driver);
			rdma_driver->Init();
			rdma_driver->TraceConnectWithoutContext("QpComplete", 
				MakeBoundCallback (qp_finish, fct_output)); // fct output
			// if(node2params.find(i) != node2params.end()){
			// 	Time pkt_trans_time = NanoSeconds(packet_payload_size * 8 * 1e9/ nic_rate);
			// 	Ptr<QbbNetDevice> qbb_net_dev = getQbbDevFromRdmaDriver(rdma_driver);
			// 	qbb_net_dev->SetRocc(1, 0, node2params[i], pkt_trans_time); // set rocc parameters
			// }
		}
	}
	#endif

	// set ACK priority on hosts
	if (ack_high_prio)
		RdmaEgressQueue::ack_q_idx = 0;
	else
		RdmaEgressQueue::ack_q_idx = 3;

	// setup routing
	CalculateRoutes(n);
	SetRoutingEntries();

	readAllFlowInputEntrys();
	// ******************** start ROCC logic *********************
	if(enable_randoffset){
		// RandOffsetInjector rand_offset_injector = rand_offset::RandOffsetInjector();
		ifstream topo_file_randoffset(topology_file);// topology file for Random Offset Injector (ROI)
		PlainRandomModel plain_rand_model(flows, topo_file_randoffset, nextHop, n, nic_rate, slots_num, slots_interval);
		plain_rand_model.insert_offsets(flows, nextHop, n, packet_payload_size);
		sortFlowsByStartTime();
	}
	// // ******************** ROCC end *****************************

	//
	// get BDP and delay
	//
	maxRtt = maxBdp = 0;
	for (uint32_t i = 0; i < node_num; i++){
		if (n.Get(i)->GetNodeType() != 0)
			continue;
		for (uint32_t j = 0; j < node_num; j++){
			if (n.Get(j)->GetNodeType() != 0)
				continue;
			uint64_t delay = pairDelay[n.Get(i)][n.Get(j)];
			uint64_t txDelay = pairTxDelay[n.Get(i)][n.Get(j)];
			uint64_t rtt = delay * 2 + txDelay;
			uint64_t bw = pairBw[i][j];
			uint64_t bdp = rtt * bw / 1000000000/8; 
			pairBdp[n.Get(i)][n.Get(j)] = bdp;
			pairRtt[i][j] = rtt;
			if (bdp > maxBdp)
				maxBdp = bdp;
			if (rtt > maxRtt)
				maxRtt = rtt;
		}
	}
	printf("maxRtt=%lu maxBdp=%lu\n", maxRtt, maxBdp);

	//
	// setup switch CC
	//
	for (uint32_t i = 0; i < node_num; i++){
		if (n.Get(i)->GetNodeType() == 1){ // switch
			Ptr<SwitchNode> sw = DynamicCast<SwitchNode>(n.Get(i));
			sw->SetAttribute("CcMode", UintegerValue(cc_mode));
			sw->SetAttribute("MaxRtt", UintegerValue(maxRtt));
		}
	}

	//
	// add trace
	//

	NodeContainer trace_nodes = get_node_container_from_file(tracef, trace_num);
	monitor_nodeids = get_nodeid_set_from_file(qmonitorf, monitor_num);

	FILE *trace_output;
	if (enable_trace){
		trace_output = fopen(trace_output_file.c_str(), "w");
		qbb.EnableTracing(trace_output, trace_nodes);
	}

	// dump link speed to trace file
	{
		SimSetting sim_setting;
		for (auto i: nbr2if){
			for (auto j : i.second){
				uint16_t node = i.first->GetId();
				uint8_t intf = j.second.idx;
				uint64_t bps = DynamicCast<QbbNetDevice>(i.first->GetDevice(j.second.idx))->GetDataRate().GetBitRate();
				sim_setting.port_speed[node][intf] = bps;
			}
		}
		sim_setting.win = maxBdp;
		if(enable_trace){
			sim_setting.Serialize(trace_output);
		}
	}

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	NS_LOG_INFO("Create Applications.");

	Time interPacketInterval = Seconds(0.0000005 / 2);

	// maintain port number for each host
	for (uint32_t i = 0; i < node_num; i++){
		if (n.Get(i)->GetNodeType() == 0)
			for (uint32_t j = 0; j < node_num; j++){
				if (n.Get(j)->GetNodeType() == 0)
					portNumder[i][j] = 10000; // each host pair use port number from 10000
			}
	}

	// schedule flows
	if (flow_num > 0){
		FlowInputEntry& cur_flow = (*flows)[cur_flow_idx];
		Time sched_after = cur_flow.getOffsetStart() - 
								Simulator::Now();
		log_flow_schedule(Simulator::Now(), sched_after, cur_flow);
		Simulator::Schedule(sched_after, ScheduleFlowInputs);
	}

	topof.close();
	tracef.close();

	// schedule link down
	if (link_down_time > 0){
		Simulator::Schedule(Seconds(2) + MicroSeconds(link_down_time), &TakeDownLink, n, n.Get(link_down_A), n.Get(link_down_B));
	}

	// // schedule buffer monitor
	FILE* qlen_output = fopen(qlen_mon_file.c_str(), "w");
	Simulator::Schedule(NanoSeconds(qlen_mon_start), &monitor_buffer, qlen_output, &n);

	//
	// Now, do the actual simulation.
	//
	// std::cout << "Running Simulation.\n";
	fflush(stdout);
	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(simulator_stop_time));
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO("Done.");
	log_cur_time();
	if(enable_trace){
		fclose(trace_output);
	}

	endt = clock();
	std::cout << (double)(endt - begint) / CLOCKS_PER_SEC << "\n";

}
