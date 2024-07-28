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
        if((this->start < rhs.start) && (this->end > rhs.start)){
            return this->end - rhs.start; // return overlap size
        } else if ((this->start < rhs.start) && (this->end > rhs.end)){
            throw runtime_error("this win totally cover rhs");
        } else if ((this->start > rhs.start) && (this->end < rhs.end)){
            throw runtime_error("rh totally cover this win");
        } else {
            return Time(0);
        }
    }

    void appendWin(Window& rhs){
        assert(this->end <= rhs.start);
        this->end = rhs.end;
    }
};

class PlainRandomModel : public NetModel{
public:
    PlainRandomModel(shared_ptr<vector<FlowInputEntry>>& flows, 
                    ifstream& topo_file, 
                    const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                    const NodeContainer &nodes){
        readTopology(topo_file);
    }

    pair<uint32_t, uint32_t> get_first_period_flow_range(shared_ptr<vector<FlowInputEntry>>& flows){
        return pair<uint32_t, uint32_t>(0, flows->size()); // now just the entire array
    }

    void shift_arr_curve_algo(shared_ptr<vector<FlowInputEntry>>& flows,
                            const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                            const NodeContainer &nodes,
                            uint32_t mtu_byte);
                            
    void insert_offsets(shared_ptr<vector<FlowInputEntry>> flows_ptr, uint32_t offset_upbound_us);
    

private:
    uint64_t get_rand(uint64_t range_start, uint64_t range_end){
        return range_start + (rand() % range_end);
    }

    // shared_ptr<vector<FlowInputEntry>>& flows_ptr;
};

} // namespace rand_offset
#endif /* PLAIN_RANDOM_MODEL_H */