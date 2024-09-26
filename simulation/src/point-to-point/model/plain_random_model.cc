#include "plain_random_model.h"
#include <iostream>

namespace rand_offset{

PlainRandomModel::PlainRandomModel(shared_ptr<vector<FlowInputEntry>>& flows, 
                    ifstream& topo_file, 
                    const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                    const NodeContainer &nodes,
                    uint64_t nic_rate_bps,
                    uint32_t slots_num,
                    Time slots_interval){
    readTopology(topo_file);
    this->slots_num = slots_num;
    this->slots_interval = slots_interval;
    uint64_t flow_size = (*flows)[0].size_byte;
    this->flow_trans_time = transTime(nic_rate_bps/8, flow_size);
}

// void PlainRandomModel::print_sw2wins(unordered_map<uint32_t, vector<Window>> sw2wins){
//     ofstream sw2wins_file("simulation/mix/rand_offset/sw2wins.txt");
//     for(const auto& sw_wins_pair : sw2wins){
//         sw2wins_file << "sw:" << sw_wins_pair.first << " ";
//         for(const auto& win : sw_wins_pair.second){
//             sw2wins_file << win << " ";
//         }
//         sw2wins_file << endl;
//     }
//     sw2wins_file.close();
// }

// void PlainRandomModel::read_param_file(string& rand_param_file){
//     ifstream ifs(rand_param_file);
//     uint32_t slots_interval_us;
//     double slots_num_d, slots_interval_us_d;
// 	ifs >> slots_num_d >> slots_interval_us_d;
//     this->slots_num = uint32_t(slots_num_d);
//     slots_interval_us = uint32_t(slots_interval_us_d);
//     ifs.close();
//     this->slots_interval = ns3::MicroSeconds(slots_interval_us);
// }

void PlainRandomModel::shift_arr_curve_algo(shared_ptr<vector<FlowInputEntry>>& flows,
                            const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                            const NodeContainer &nodes,
                            uint32_t mtu_byte){
    // Assuming flows are periodic, 
    // only schedule one period
    const auto flow_range = get_first_period_flow_range(flows);

    // assign offset order the same as flows vector (time tabling)
    unordered_map<uint32_t, vector<Window>> sw2wins;
    unordered_map<uint32_t, Window> sw2merged_win;
    for(uint32_t flow_id = 0; flow_id < flows->size(); flow_id++){
        FlowInputEntry& flow = (*flows)[flow_id];
        Ptr<Node> src_ptr = nodes.Get(flow.src);
        Ptr<Node> dst_ptr = nodes.Get(flow.dst);

        const vector<Ptr<Node>> path_nodes = getPathFromNextHop(nextHop, src_ptr, dst_ptr);
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
                    sw2merged_win[sw_id] = Window({Time(0), Time{0}});
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
                Window& cur_sw_merged_win = sw2merged_win.at(sw_id);

                // Time overlap_size;
                // if(cur_sw_wins.empty()){
                //     overlap_size = Time(0);
                // } else {
                //     overlap_size = cur_sw_wins.back().overlapSize(cur_win);
                // }
                Time overlap_size = cur_sw_merged_win.overlapSize(cur_win);
                if(overlap_size == Time(0)){
                    cur_sw_wins.push_back(cur_win);
                    cur_sw_merged_win.mergeWin(cur_win);
                    if(i == path_nodes.size()-1){
                        alloc_succ = true;
                    }
                } else {
                    // insert additional gap
                    flow.offset += overlap_size + this->shift_gap;
                    break;
                }
            }
        }
    }  
    // print_sw2wins(sw2wins);
}

void PlainRandomModel::insert_offsets(shared_ptr<vector<FlowInputEntry>>& flows,
                            const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                            const NodeContainer &nodes,
                            uint32_t mtu_byte){
    // shift_arr_curve_algo(flows, nextHop, nodes, mtu_byte);
    rand_slot_offset(flows, nextHop, nodes, mtu_byte);
}

void PlainRandomModel::rand_slot_offset(shared_ptr<vector<FlowInputEntry>>& flows,
                            const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                            const NodeContainer &nodes,
                            uint32_t mtu_byte){
    // get a random number between 0 and slot_num
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> rand_dist(0,this->slots_num-1); // distribution in range [1, 6]

    cout << "----------------- Random Slot Offset -----------------" << endl;
    cout << "slots_num:" << this->slots_num << " slots_interval_us:" << this->slots_interval.GetMicroSeconds() << endl;
    cout << "flow trans_time:" << this->flow_trans_time.GetMicroSeconds() << "us" << endl; 
    for (uint32_t flow_id = 0; flow_id < flows->size(); flow_id++){
        FlowInputEntry& flow = (*flows)[flow_id];
        uint32_t slot_offset = rand_dist(rng);
        flow.offset = ns3::MilliSeconds(
            this->slots_interval.GetMilliSeconds() * (double(slot_offset) / this->slots_num));
        // cout << "slots_interval.GetMilliSeconds():" << slots_interval.GetMilliSeconds() << endl;
        // cout << "slots_interval.GetMilliSeconds() * (slot_offset / slot_num):" << slots_interval.GetMilliSeconds() * (slot_offset / slot_num) << endl;
        cout << "flow_id:" << flow_id << " offset:" << flow.offset.GetMicroSeconds() << "us" 
             << " offset/flowtrans="<< flow.offset.GetMicroSeconds() / double(this->flow_trans_time.GetMicroSeconds())   << endl;
    }
    cout << "------------------------------------------------------" << endl;
}
}