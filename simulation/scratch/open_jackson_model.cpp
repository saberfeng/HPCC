#include "open_jackson_model.h"

namespace rand_offset{

void OpenJacksonModel::initInputRate(std::ifstream &flow_file){

}

void OpenJacksonModel::initialize(const string& flow_file, const string& topo_file){
    std::ifstream topo_f(topo_file), flow_f(flow_file);
    assert(topo_f.is_open() && flow_f.is_open());
    // 1. init node type
    uint32_t num_node, num_switch, num_link;
    topo_f >> num_node >> num_switch >> num_link;
    node_type = vector<uint32_t>(num_node, 0);
    for(uint32_t i = 0; i < num_switch; i++){
        uint32_t switch_id;
        topo_f >> switch_id;
        node_type[switch_id] = 1;
    }
    // 2. init input rate
    initInputRate(flow_f);
    initServiceRateRoutingMatrix(topo_f);
}

void OpenJacksonModel::initServiceRateRoutingMatrix(std::ifstream &flow_f){
    // use flow_file to init input_rate_bps
    uint32_t flow_num;
    flow_f >> flow_num;
    FlowInputEntry flow_input;
    for (uint32_t i = 0; i < flow_num; i++){
        flow_f >> flow_input.src >> flow_input.dst >> flow_input.priority_group 
               >> flow_input.dst_port >> flow_input.size_byte >> flow_input.start_time_s;
        
        if(node2flowsums.find(flow_input.src) == node2flowsums.end()){
            node2flowsums[flow_input.src] = {flow_input.src, 0, flow_input.start_time_s, 0};
        }
    }
}

} // namespace rand_offset