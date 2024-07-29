#include "net_model.h"

namespace rand_offset{

uint64_t getQueSzByteByBw(long double bandwidth_Bps){
    // queue size usually should provide 50ms line rate transmission
    return bandwidth_Bps*50/1000;
}

void NetModel::readTopology(ifstream& topo_f){
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

Time transTime(long double bandwidth_Bps, uint64_t size_byte){
    long double trans_s = (long double)size_byte/bandwidth_Bps;
    uint64_t trans_ns = trans_s * 1e9;
    return ns3::NanoSeconds(trans_ns);
}

}
