#include "open_jackson_model.h"

namespace rand_offset{

void OpenJacksonModel::initialize(const string& flow_file, const string& topo_file,
                                const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop,
                                const NodeContainer &node_container){
    readTopology(topo_file);
    readFlows(flow_file, node_container);

    initRoutingMatrix(next_hop);
    initInputRate();
    initServiceRate();

    calcStateProb();
}

void OpenJacksonModel::calcStateProb(){
    // calculate the state probability
    // first calculate the transition matrix
    // then calculate the steady state probability
    // transition matrix is the routing matrix * input_rate_bps / service_rate_bps
    // steady state probability is the eigenvector of the transition matrix
    // corresponding to the eigenvalue 1
    // the steady state probability is the probability of each state
    // the state is the number of packets in each queue

    vector<double> node_drop_prob(node_info.size(), 0);
    // \lambda^t = \gamma * (I - R^t)-1 = (\lambda_1, lambda_2, ..., lambda_n)
    gamma = Vector(input_rate_bps);
    I = Eye(node_info.size());
    R = routing_matrix;
    lambda = gamma * (I - R).Inverse()
    std::cout<<lambda<<std::endl;
}

void OpenJacksonModel::readTopology(const string& topo_file){
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
        uint32_t src, dst, bandwidth_bps;
        string bandwidth_str;
        topo_f >> src >> dst >> bandwidth_str; // bandwidth: "100Gbps" -> 100 * 10^9
        // convert bandwidth to bps
        if(bandwidth_str.find('Gbps') != string::npos){ // unit is Gbps
            bandwidth_str = bandwidth_str.substr(0, bandwidth_str.size() - 4);
            bandwidth_bps = std::stod(bandwidth_str) * 1e9;
        } else {
            throw std::runtime_error("unsupported bandwidth unit");
        }
        Link link = {src, dst, bandwidth_bps};
        topo[src][dst] = link;      
    }
}

void OpenJacksonModel::readFlows(const string& flow_file){
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
            uint32_t trans_time_s = flow_input.size_byte*8 / node_info[flow_input.src].bandwidth_bps;
            node2flowsums[flow_input.src] = {flow_input.src, flow_input.size_byte, 
                                            flow_input.start_time_s, flow_input.start_time_s+trans_time_s};
        } else{
            node2flowsums[flow_input.src].sum_flow_size_byte += flow_input.size_byte;
            node2flowsums[flow_input.src].end_time_s = 
                std::max(node2flowsums[flow_input.src].end_time_s, 
                        flow_input.start_time_s + flow_input.size_byte*8 / node_info[flow_input.src].bandwidth_bps);
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
            flow2routing_matrix[node_id] = routing_matrix;
        }
    }
}

void OpenJacksonModel::initInputRate(){
    for(auto& node2flowsum : node2flowsums){
        input_rate_bps[node2flowsum.second.host_id] = 
            node2flowsum.second.sum_flow_size_byte*8 / (node2flowsum.second.end_time_s - node2flowsum.second.start_time_s);
    }
}

void OpenJacksonModel::initServiceRate(){
    // service rate is the sum of the bandwidth of all links connected to the switch    
    service_rate_bps.fill(0);
    for (auto& src_dst_link : topo){
        uint32_t src = src_dst_link.first;
        for(auto& dst_link : src_dst_link.second){
            uint32_t dst = dst_link.first;
            service_rate_bps[src] += dst_link.second.bandwidth_bps;
        }
    }
}

} // namespace rand_offset