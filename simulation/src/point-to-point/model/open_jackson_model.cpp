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

void OpenJacksonModel::initialize(ifstream &flow_file, ifstream &topo_file,
                                const map<Ptr<Node>, 
                                          map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop,
                                const NodeContainer &node_container){
    readTopology(topo_file);
    readFlows(flow_file, node_container);

    initRoutingMatrix(next_hop);
    initInputRate();
    initServiceRate();

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
        FlowId flow_id = flow_routing_matrix.first;
        Matrix routing_matrix = flow_routing_matrix.second;
        // matrix numbers splitted by space
        for (uint32_t i; i < routing_matrix.rows(); i++){
            for (uint32_t j; j < routing_matrix.cols(); j++){
                routing_matrix_ss << routing_matrix[i][j] << " ";
            } 
            routing_matrix_ss << "\n";
        }
    }
    writeFile(routing_matrix_ss.str(), routing_path);

    std::stringstream input_rate_ss;
    for (auto& flow2input_pair : flow2input_Bps){
        FlowId flow_id = flow2input_pair.first;
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
    for (auto& node : node_info){
        queue_size_ss << node.queue_size_byte << " ";
    }
    writeFile(queue_size_ss.str(), queue_size_path);

    stringstream cmd;
    cmd << "python3 simulation/scripts/calc_matrix.py "
        << "-r " << routing_path << " "
        << "-i " << input_rate_path << " "
        << "-s " << service_path << " "
        << "-q " << queue_size_path << " "
        << "-f " << total_flow_num << " "
        << "-o1 " << output_rho_path << " "
        << "-o2 " << output_nodrop_prob_path;
    system(cmd.str());

    // read python script output
    vector<long double> rho_vec = readVector(output_rho_path);
    vector<long double> node_drop_prob = readVector(output_nodrop_prob_path);

    return pair<vector<long double>, vector<long double>>(
        rho_vec, node_drop_prob); 
}

void OpenJacksonModel::readTopology(
        ifstream& topo_file, 
        const NodeContainer &node_container){
    std::ifstream topo_f(topo_file);
    assert(topo_f.is_open());
    uint32_t num_node, num_switch, num_link;
    topo_f >> num_node >> num_switch >> num_link;
    node_info = vector<NodeEntry>(num_node, {0, 0});
    for(uint32_t i = 0; i < num_switch; i++){
        uint32_t switch_id;
        topo_f >> switch_id;
        node_info[switch_id].node_type = 1;
    }
    for(uint32_t i = 0; i < num_link; i++){
        uint32_t src, dst, bandwidth_Bps;
        string bandwidth_str;
        topo_f >> src >> dst >> bandwidth_str; // bandwidth: "100Gbps" -> 100 * 10^9
        // convert bandwidth to bps
        if(bandwidth_str.find(string("Gbps")) != string::npos){ // unit is Gbps
            bandwidth_str = bandwidth_str.substr(0, bandwidth_str.size() - 4);
            bandwidth_Bps = std::stod(bandwidth_str) * 1e9 / 8;
        } else {
            throw std::runtime_error("unsupported bandwidth unit");
        }
        Link link = {src, dst, bandwidth_Bps};
        topo[src][dst] = link;      
    }
}

void OpenJacksonModel::readFlows(ifstream& flow_file, const NodeContainer &node_container){
    std::ifstream flow_f(flow_file);
    assert(flow_f.is_open());
    // conversion? 
    flow_f >> total_flow_num;

    for (uint32_t i = 0; i < total_flow_num; i++){
        FlowInputEntry flow_input;
        flow_f >> flow_input.src >> flow_input.dst >> flow_input.priority_group 
               >> flow_input.dst_port >> flow_input.size_byte >> flow_input.start_time_s;
        flow_input.flow_idx = i;

        node2flows[flow_input.src].push_back(
            std::make_shared<FlowInputEntry>(flow_input));
        if(node2flowsums.find(flow_input.src) == node2flowsums.end()){ 
            // have not created HostFlowSum object at this node
            uint32_t trans_time_s = flow_input.size_byte*8 / node_info[flow_input.src].bandwidth_Bps;
            node2flowsums[flow_input.src] = {flow_input.src, flow_input.size_byte, 
                                            flow_input.start_time_s, flow_input.start_time_s+trans_time_s};
        } else{
            // append new flow to corresponding node 
            node2flowsums[flow_input.src].sum_flow_size_byte += flow_input.size_byte;
            node2flowsums[flow_input.src].end_time_s = 
                std::max(node2flowsums[flow_input.src].end_time_s, 
                        flow_input.start_time_s + flow_input.size_byte*8 / node_info[flow_input.src].bandwidth_Bps);
        }
    }
}

void OpenJacksonModel::initRoutingMatrix(
        const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop,
        const NodeContainer &node_container){
    // first iterate through node2flows to get all flows
    for(auto& node2flow : node2flows){
        uint32_t node_id = node2flow.first;
        vector<shared_ptr<FlowInputEntry>> &flows = node2flow.second;

        for(auto& flow_ptr : flows){
            Matrix routing_matrix(node_info.size(), node_info.size());
            routing_matrix.Fill(0);
            // get flow path
            Ptr<Node> src = node_container.Get(flow_ptr->src);
            Ptr<Node> dst = node_container.Get(flow_ptr->dst);
            vector<Ptr<Node>> path = next_hop.at(src).at(dst);
            for (uint32_t i = 0; i <= path.size()-2; i++){
                uint32_t src_id = path[i]->GetId();
                uint32_t dst_id = path[i+1]->GetId();
                routing_matrix[src_id][dst_id] = 1;
            }
            flow2routing_matrix[flow_ptr] = routing_matrix;
        }
    }
}

void OpenJacksonModel::initInputRate(){
    for(auto& node2flows_pair : node2flows){
        NodeId& node_id = node2flows_pair.first;
        HostFlowSum& nodeFlowSum = node2flowsums.at(node_id);
        long long node_transtime = nodeFlowSum.end_time_s - nodeFlowSum.start_time_s;
        for(auto& flow_ptr: node2flows_pair.second){
            // initialize flow i's input as 0
            flow2input_Bps[flow_ptr] = 
                    vector<double>(node_container.GetN(), 0);
            flow2input_Bps[flow_ptr][node_id] = flow_ptr->size_byte / node_transtime;
        }
    }
    
    for(auto& node2flowsum : node2flowsums){
        input_rate_Bps[node2flowsum.second.host_id] = 
            node2flowsum.second.sum_flow_size_byte / (node2flowsum.second.end_time_s - node2flowsum.second.start_time_s);
    }
}

void OpenJacksonModel::initServiceRate(){
    // service rate is the sum of the bandwidth of all links connected to the switch    
    service_rate_Bps = vector<double>(node_info.size(), 0);
    for (auto& src_dst_link : topology){
        uint32_t src = src_dst_link.first;
        for(auto& dst_link : src_dst_link.second){
            uint32_t dst = dst_link.first;
            service_rate_Bps[src] += dst_link.second.bandwidth_Bps;
        }
    }
}

} // namespace rand_offset