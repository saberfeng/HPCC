#ifndef PLAIN_RANDOM_MODEL_H
#define PLAIN_RANDOM_MODEL_H

#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <utility>
#include <cassert>
#include <random>
#include "ns3/core-module.h"
#include "ns3/node.h"
#include "ns3/ptr.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/net_model.h"


namespace rand_offset{

using std::vector;
using std::string;
using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::unordered_map;
using ns3::Node;
using ns3::Ptr;
using ns3::NodeContainer;
using std::pair;
using std::map;
using std::shared_ptr;
using std::cout;
using std::endl;
using ns3::Time;
using std::runtime_error;

typedef uint32_t NodeId;
typedef uint32_t FlowId;

struct Window{
    Time start; // switch trans start time
    Time end; // switch trans end time
    uint32_t flow_id;

    Time size(){
        return end-start;
    }

    Time overlapSize(Window& rhs){
        if((this->start <= rhs.start) && (this->end > rhs.start)){
            return this->end - rhs.start; // return overlap size
        } else if ((this->start < rhs.start) && (this->end > rhs.end)){
            throw runtime_error("this win totally cover rhs");
        } else if ((this->start > rhs.start) && (this->end < rhs.end)){
            throw runtime_error("rh totally cover this win");
        } else {
            return Time(0);
        }
    }

    void mergeWin(Window& rhs){
        assert(this->end <= rhs.start);
        this->end = rhs.end;
    }

    friend std::ostream& operator<<(std::ostream& os, const Window& win){
        os << "[" << win.flow_id << " " 
            << win.start << "-" << win.end << "]";
    }
};

class PlainRandomModel : public NetModel{
public:
    PlainRandomModel(shared_ptr<vector<FlowInputEntry>>& flows, 
                    ifstream& topo_file, 
                    const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                    const NodeContainer &nodes,
                    string& rand_param_file){
        readTopology(topo_file);
        read_param_file(rand_param_file);
    }

    void read_param_file(string& rand_param_file);

    pair<uint32_t, uint32_t> get_first_period_flow_range(shared_ptr<vector<FlowInputEntry>>& flows){
        return pair<uint32_t, uint32_t>(0, flows->size()); // now just the entire array
    }

    void shift_arr_curve_algo(shared_ptr<vector<FlowInputEntry>>& flows,
                            const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                            const NodeContainer &nodes,
                            uint32_t mtu_byte);
                            
    void insert_offsets(shared_ptr<vector<FlowInputEntry>>& flows,
                            const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                            const NodeContainer &nodes,
                            uint32_t mtu_byte);
    
    // given an interval and number of slots to divide,
    //      assign a random slot offset to each flow 
    void rand_slot_offset(shared_ptr<vector<FlowInputEntry>>& flows,
                            const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                            const NodeContainer &nodes,
                            uint32_t mtu_byte,
                            uint32_t slot_num,
                            Time slot_interval);

private:
    uint64_t get_rand(uint64_t range_start, uint64_t range_end){
        return range_start + (rand() % range_end);
    }

    void print_sw2wins(unordered_map<uint32_t, vector<Window>> sw2wins);

    Time shift_gap;
};

} // namespace rand_offset
#endif /* PLAIN_RANDOM_MODEL_H */