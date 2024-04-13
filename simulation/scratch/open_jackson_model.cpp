#include "open_jackson_model.h"

namespace rand_offset{

void OpenJacksonModel::initInputRate(std::ifstream &flow_file){

}

void OpenJacksonModel::initialize(const string& flow_file, const string& topo_file,
                                const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop){
    readTopology(topo_file);
    readFlows(flow_file);

    initRoutingMatrix(next_hop);
    initInputRate();
    initServiceRate();
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
        node_info[src].bandwidth_bps = bandwidth_bps;
        node_info[dst].bandwidth_bps = bandwidth_bps;
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

void OpenJacksonModel::initRoutingMatrix(const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop){
    // first iterate through node2flows to get all flows
    for(auto& node2flow : node2flows){
        uint32_t node_id = node2flow.first;
        vector<FlowInputEntry> &flows = node2flow.second;

        for(auto& flow : flows){
            Matrix routing_matrix(node_info.size(), node_info.size());
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
    
}

} // namespace rand_offset