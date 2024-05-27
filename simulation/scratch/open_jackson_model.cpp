#include "open_jackson_model.h"

namespace rand_offset{

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
    vector<long double> rho_vec(node_info.size(), 0);
    vector<long double> node_drop_prob(node_info.size(), 0);
    // \lambda^t = \gamma * (I - R^t)-1 = (\lambda_1, lambda_2, ..., lambda_n)
    gamma = Vector(input_rate_Bps);
    I = Eye(node_info.size());
    lambda = Vector(node_info.size()).Zeros();
    for (auto& flow_routing_matrix : flow2routing_matrix){
        FlowId flow_id = flow_routing_matrix.first;
        Matrix routing_matrix = flow_routing_matrix.second;
        lambda_t = gamma * (I - routing_matrix).Inverse();
        std::cout<<lambda<<std::endl;
        lambda = lambda + lambda_t;
        for (int i = 0; i < lambda.size(); i++){
            rho_vec[i] = lambda[i] / service_rate_Bps[i];
            // p(buffered packets >= queue_size) = (1 - rho) * \sum_n^{queue_size} rho^n
            for (int n = 0; n < node_info[i].queue_size_byte; n++){
                node_drop_prob[i] += (1 - rho) * std::pow(rho, n);
            }
        }
    }
    return <vector, vector>(rho_vec, node_drop_prob); 
}

void OpenJacksonModel::readTopology(ifstream& topo_file){
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
        if(bandwidth_str.find('Gbps') != string::npos){ // unit is Gbps
            bandwidth_str = bandwidth_str.substr(0, bandwidth_str.size() - 4);
            bandwidth_Bps = std::stod(bandwidth_str) * 1e9 / 8;
        } else {
            throw std::runtime_error("unsupported bandwidth unit");
        }
        Link link = {src, dst, bandwidth_Bps};
        topo[src][dst] = link;      
    }
}

void OpenJacksonModel::readFlows(ifstream& flow_file){
    std::ifstream flow_f(flow_file);
    assert(flow_f.is_open());
    uint32_t flow_num;
    flow_f >> flow_num;
    for (uint32_t i = 0; i < flow_num; i++){
        FlowInputEntry flow_input;
        flow_f >> flow_input.src >> flow_input.dst >> flow_input.priority_group 
               >> flow_input.dst_port >> flow_input.size_byte >> flow_input.start_time_s;
        flow_input.flow_idx = i;
        
        node2flows[flow_input.src].push_back(flow_input);
        if(node2flowsums.find(flow_input.src) == node2flowsums.end()){
            uint32_t trans_time_s = flow_input.size_byte*8 / node_info[flow_input.src].bandwidth_Bps;
            node2flowsums[flow_input.src] = {flow_input.src, flow_input.size_byte, 
                                            flow_input.start_time_s, flow_input.start_time_s+trans_time_s};
        } else{
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
        vector<FlowInputEntry> &flows = node2flow.second;

        for(auto& flow : flows){
            Matrix routing_matrix(node_info.size(), node_info.size());
            routing_matrix.fill(0);
            // get flow path
            Node src = node_container.Get(flow.src);
            Node dst = node_container.Get(flow.dst);
            vector<Ptr<Node>> path = next_hop.at(src).at(dst);
            for (uint32_t i = 0; i <= path.size()-2; i++){
                uint32_t src_id = path[i]->GetId();
                uint32_t dst_id = path[i+1]->GetId();
                routing_matrix[src_id][dst_id] = 1;
            }
            flow2routing_matrix[flow.flow_idx] = routing_matrix;
        }
    }
}

void OpenJacksonModel::initInputRate(){
    for(auto& node2flowsum : node2flowsums){
        input_rate_Bps[node2flowsum.second.host_id] = 
            node2flowsum.second.sum_flow_size_byte / (node2flowsum.second.end_time_s - node2flowsum.second.start_time_s);
    }
}

void OpenJacksonModel::initServiceRate(){
    // service rate is the sum of the bandwidth of all links connected to the switch    
    service_rate_Bps.fill(0);
    for (auto& src_dst_link : topo){
        uint32_t src = src_dst_link.first;
        for(auto& dst_link : src_dst_link.second){
            uint32_t dst = dst_link.first;
            service_rate_Bps[src] += dst_link.second.bandwidth_Bps;
        }
    }
}

} // namespace rand_offset