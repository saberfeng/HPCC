#include "open_jackson_model.h"

namespace rand_offset{

void writeFile(const string& text, const string& path){
    ofstream file;
    file.open(path);
    file << text;
    file.close();
}

vector<long double> readVector(const string& filename){
    std::ifstream file(filename);
    assert(file.is_open());
    vector<long double> vec;
    while(!file.eof()){
        long double value;
        file >> value;
        vec.push_back(value);
    }
    return vec;
}

void OpenJacksonModel::initialize(const vector<shared_ptr<FlowInputEntry>>& flows, 
                                ifstream &topo_file,
                                const map<Ptr<Node>, 
                                          map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop,
                                const NodeContainer &node_container){
    num_flows = flows.size();
    input_rate_Bps = vector<double>(node_info.size(), 0);
    service_rate_Bps = vector<double>(node_info.size(), 0);

    readTopology(topo_file);
    buildNode2Flows(flows);

    updateRoutingMatrix(next_hop, node_container);
    updateNode2FlowSums(node_container, next_hop);
    updateInputRate(node_container);
    updateServiceRate();
}

void appendFlowOffsets(const unordered_map<shared_ptr<FlowInputEntry>, 
                                           long double>& flow2offsets){
    for(auto& flow_offset : flow2offsets){
        shared_ptr<FlowInputEntry> flow_ptr = flow_offset.first;
        long double offset = flow_offset.second;
        flow_ptr->start_time_s += offset;
    }
}

pair<vector<long double>, vector<long double>> 
OpenJacksonModel::calcStateProb(){
    // the steady state probability is the probability of each state
    // the state is the number of packets in each queue
    // gamma (vector): mean arrival rate from outside network
    // lambda (vector): the sum of the arrival rate of all flows at each node
    // lambda^t (vector): the sum of the arrival rate of flow t at each node 
    // rho (vector): the utilization of each node
    string write_path("simulation/mix/rand_offset/");
    string routing_path = write_path + string("routing.txt");
    string input_rate_path = write_path + string("input_rate.txt");
    string service_path = write_path + string("service_rate.txt");
    string queue_size_path = write_path + string("queue_size.txt");
    string output_rho_path = write_path + string("rho.txt");
    string output_nodrop_prob_path = write_path + string("node_drop_prob.txt");

    // prepare routing matrix, stack matrices vertically
    std::stringstream routing_matrix_ss;
    for (auto& flow_routing_matrix : flow2routing_matrix){
        // FlowId flow_id = flow_routing_matrix.first;
        Matrix routing_matrix = flow_routing_matrix.second;
        // matrix numbers splitted by space
        for (uint32_t i; i < routing_matrix.size(); i++){
            for (uint32_t j; j < routing_matrix.size(); j++){
                routing_matrix_ss << routing_matrix[i][j] << " ";
            } 
            routing_matrix_ss << "\n";
        }
    }
    writeFile(routing_matrix_ss.str(), routing_path);

    std::stringstream input_rate_ss;
    for (auto& flow2input_pair : flow2input_Bps){
        // FlowId flow_id = flow2input_pair.first;
        for(auto& node_input_rate : flow2input_pair.second){
            input_rate_ss << node_input_rate << " ";
        }
        input_rate_ss << "\n"; // stack flows vertically
    }
    writeFile(input_rate_ss.str(), input_rate_path);

    std::stringstream service_rate_ss;
    for (auto& service_rate : service_rate_Bps){
        service_rate_ss << service_rate << " ";
    }
    writeFile(service_rate_ss.str(), service_path);

    std::stringstream queue_size_ss;
    for(auto& node_links : topology){
        uint32_t src_id = node_links.first;
        for(auto& dst_link : node_links.second){
            uint32_t dst_id = dst_link.first;
            Link& link = dst_link.second;
            queue_size_ss << src_id << " "
                          << dst_id << " "
                          << link.queue_size_byte << "\n";
        }
    }
    writeFile(queue_size_ss.str(), queue_size_path);

    stringstream cmd;
    cmd << "python3 simulation/scripts/calc_matrix.py "
        << "-r " << routing_path << " "
        << "-i " << input_rate_path << " "
        << "-s " << service_path << " "
        << "-q " << queue_size_path << " "
        << "-f " << num_flows << " "
        << "-o1 " << output_rho_path << " "
        << "-o2 " << output_nodrop_prob_path;
    system(cmd.str().c_str());

    // read python script output
    vector<long double> rho_vec = readVector(output_rho_path);
    vector<long double> node_drop_prob = readVector(output_nodrop_prob_path);

    return pair<vector<long double>, vector<long double>>(
        rho_vec, node_drop_prob); 
}

uint64_t getQueSzByteByBw(long double bandwidth_Bps){
    // queue size usually should provide 50ms line rate transmission
    return bandwidth_Bps*50/1000;
}

void OpenJacksonModel::readTopology(ifstream& topo_f){
    assert(topo_f.is_open());
    uint32_t num_node, num_switch, num_link;
    topo_f >> num_node >> num_switch >> num_link;
    node_info = vector<NodeEntry>(num_node, {0});
    for(uint32_t i = 0; i < num_switch; i++){
        uint32_t switch_id;
        topo_f >> switch_id;
        node_info[switch_id].node_type = 1;
    }
    for(uint32_t i = 0; i < num_link; i++){
        uint32_t src, dst;
        long double bandwidth_Bps;
        string bandwidth_str, link_delay, error_rate;
        topo_f >> src >> dst >> bandwidth_str >> link_delay >> error_rate; // bandwidth: "100Gbps" -> 100 * 10^9
        // convert bandwidth to bps
        if(bandwidth_str.find(string("Gbps")) != string::npos){ // unit is Gbps
            bandwidth_str = bandwidth_str.substr(0, bandwidth_str.size() - 4);
            bandwidth_Bps = std::stod(bandwidth_str) * 1e9 / 8;
        } else {
            throw std::runtime_error("unsupported bandwidth unit");
        }
        Link link = {src, dst, bandwidth_Bps, getQueSzByteByBw(bandwidth_Bps)};
        topology[src][dst] = link;      
        topology[dst][src] = link;
    }
}

void OpenJacksonModel::buildNode2Flows(const vector<shared_ptr<FlowInputEntry>>& flows){
    for(const auto& flow_ptr : flows){
        node2flows[flow_ptr->src].push_back(flow_ptr);
    }
}

uint64_t OpenJacksonModel::getBwByLinkNodeId(
        uint32_t link_src_id, uint32_t link_dst_id,
        const NodeContainer &node_container,
        const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop){
    // uint64_t bw = topology.at(node_id)
    Ptr<Node> src_node = node_container.Get(link_src_id);
    Ptr<Node> dst_node = node_container.Get(link_dst_id);
    vector<Ptr<Node>> path = next_hop.at(src_node).at(dst_node);
    Ptr<Node> first_path_sw = path[0];
    const Link& first_link = topology.at(src_node->GetId()).at(first_path_sw->GetId());
    long double bandwidth_Bps = first_link.bandwidth_Bps;
    return bandwidth_Bps;
    
}

void OpenJacksonModel::updateNode2FlowSums(
    const NodeContainer& node_container,
    const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop){
    for(const auto& node2flows_pair : node2flows){
        NodeId node_id = node2flows_pair.first;
        for(const auto& flow_ptr : node2flows_pair.second){
            long double bandwidth_Bps = getBwByLinkNodeId(
                flow_ptr->src, flow_ptr->dst, node_container, next_hop);
            long double trans_time_s = flow_ptr->size_byte*8 / bandwidth_Bps;

            if(node2flowsums.find(node_id) == node2flowsums.end()){ 
                // have not created HostFlowSum object at this node
                node2flowsums[node_id] = {node_id, 0, 
                                                flow_ptr->start_time_s, flow_ptr->start_time_s+trans_time_s};
            }
            // append new flow to corresponding node 
            node2flowsums[node_id].sum_flow_size_byte += flow_ptr->size_byte;
            node2flowsums[node_id].end_time_s = 
                std::max(node2flowsums[node_id].end_time_s, 
                        flow_ptr->start_time_s + trans_time_s);

            cout << "node2flowsums:[" << node_id << "].sum_flow_size_byte: " 
                 << node2flowsums[node_id].sum_flow_size_byte << endl;
            cout << "node2flowsums:[" << node_id << "].end_time_s: " 
                 << node2flowsums[node_id].end_time_s << endl;
        }
    }
}


void OpenJacksonModel::updateRoutingMatrix(
        const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop,
        const NodeContainer &node_container){
    // first iterate through node2flows to get all flows
    for(auto& node2flow : node2flows){
        uint32_t node_id = node2flow.first;
        vector<shared_ptr<FlowInputEntry>> &flows = node2flow.second;

        for(auto& flow_ptr : flows){
            for(uint32_t i = 0; i < node_container.GetN(); i++){
                routing_matrix.push_back(vector<uint32_t>(node_container.GetN(), 0));
            }
            // get flow path
            Ptr<Node> src = node_container.Get(flow_ptr->src);
            Ptr<Node> dst = node_container.Get(flow_ptr->dst);
            vector<Ptr<Node>> path = next_hop.at(src).at(dst); // path only contains intermediate nodes
            uint32_t last_id = src->GetId();
            uint32_t cur_id = src->GetId();
            for (uint32_t i = 0; i < path.size(); i++){
                cur_id = path[i]->GetId();
                routing_matrix[last_id][cur_id] = 1;
                last_id = cur_id;
            }
            routing_matrix[cur_id][dst->GetId()] = 1;
            flow2routing_matrix[flow_ptr] = routing_matrix;
        }
    }
}

// relies on node2flows and node2flowsums, should be 
// updated after reading flows and updating node2flowsums
void OpenJacksonModel::updateInputRate(const NodeContainer &node_container){
    input_rate_Bps = vector<double>(node_container.GetN(), 0);
    for(auto& node2flows_pair : node2flows){
        const NodeId& node_id = node2flows_pair.first;
        HostFlowSum& nodeFlowSum = node2flowsums.at(node_id);
        long double node_transtime = nodeFlowSum.end_time_s - nodeFlowSum.start_time_s;
        for(auto& flow_ptr: node2flows_pair.second){
            // initialize flow i's input as 0
            flow2input_Bps[flow_ptr] = 
                    vector<double>(node_container.GetN(), 0);
            flow2input_Bps[flow_ptr][node_id] = flow_ptr->size_byte / node_transtime;
        }
    }
    
    for(auto& node2flowsum : node2flowsums){
        long double node_trans_time = node2flowsum.second.end_time_s - 
                                        node2flowsum.second.start_time_s; 
        input_rate_Bps[node2flowsum.first] = 
            node2flowsum.second.sum_flow_size_byte / node_trans_time;
    }
}

void OpenJacksonModel::updateServiceRate(){
    // service rate is the sum of the bandwidth of all links connected to the switch    
    for (auto& src_dst_link : topology){
        uint32_t src = src_dst_link.first;
        for(auto& dst_link : src_dst_link.second){
            uint32_t dst = dst_link.first;
            service_rate_Bps[src] += dst_link.second.bandwidth_Bps;
        }
    }
}

} // namespace rand_offset