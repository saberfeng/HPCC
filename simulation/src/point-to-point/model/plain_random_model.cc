#include "plain_random_model.h"

namespace rand_offset{

void PlainRandomModel::shift_arr_curve_algo(shared_ptr<vector<FlowInputEntry>>& flows,
                            const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                            const NodeContainer &nodes,
                            uint32_t mtu_byte){
    // Assuming flows are periodic, 
    // only schedule one period
    const auto flow_range = get_first_period_flow_range(flows);

    // assign offset order the same as flows vector (time tabling)
    unordered_map<uint32_t, vector<Window>> sw2wins;
    for(uint32_t flow_id = 0; flow_id < flows->size(); flow_id++){
        auto flow = (*flows)[flow_id];
        Ptr<Node> src_ptr = nodes.Get(flow.src);
        Ptr<Node> dst_ptr = nodes.Get(flow.dst);

        const vector<Ptr<Node>>& path_nodes = nextHop.at(src_ptr).at(dst_ptr);
        bool alloc_succ = false;
        while(!alloc_succ){
            for(uint32_t i = 0; i < path_nodes.size(); i++){
                uint32_t sw_id = path_nodes[i]->GetId();
                // get last hop id
                uint32_t prev_id;
                if(i == 0){
                    prev_id = flow.src; 
                } else {
                    prev_id = path_nodes[i-1]->GetId();
                }

                // get next hop id
                uint32_t next_id;
                if(i == path_nodes.size()-1){
                    next_id = flow.dst;
                } else {
                    next_id = path_nodes[i+1]->GetId();
                }

                if(sw2wins.find(sw_id) == sw2wins.end()){
                    sw2wins[sw_id] = vector<Window>();
                }
                Link ingress_link = topology.at(prev_id).at(sw_id);
                Link out_link = topology.at(sw_id).at(next_id);
                Time flow_trans_time_in = transTime(ingress_link.bandwidth_Bps, flow.size_byte);
                Time packet_trans_time_in = transTime(ingress_link.bandwidth_Bps, mtu_byte);
                Time flow_trans_time_out = transTime(out_link.bandwidth_Bps, flow.size_byte);
                Time packet_trans_time_out = transTime(out_link.bandwidth_Bps, mtu_byte);

                // get last hop trans start time
                Time last_trans_start;
                if(i == 0){
                    last_trans_start = flow.getOffsetStart();
                } else {
                    last_trans_start = sw2wins.at(prev_id).back().start;
                }

                // this hop receive time 
                Time recv_time = last_trans_start + packet_trans_time_in + ns3::MicroSeconds(1);
                // this hop start transmit time
                Time start_time = recv_time;
                Window cur_win({start_time, start_time+flow_trans_time_out, flow.flow_idx});
                vector<Window>& cur_sw_wins = sw2wins.at(sw_id);
                Time overlap_size = cur_sw_wins.back().overlapSize(cur_win);
                if(overlap_size == Time(0)){
                    cur_sw_wins.push_back(cur_win);
                    if(i == path_nodes.size()-1){
                        alloc_succ = true;
                    }
                } else {
                    flow.offset += overlap_size;
                    break;
                }
            }
        }
        
    }  
}

void PlainRandomModel::insert_offsets(shared_ptr<vector<FlowInputEntry>> flows_ptr, uint32_t offset_upbound_us){
    for(auto& flow : *flows_ptr){
        if(flow.flow_idx == 1){

        }
        flow.offset = ns3::MicroSeconds(get_rand(0, offset_upbound_us));
    }
}

}