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

typedef uint32_t NodeId;
typedef uint32_t FlowId;

class PlainRandomModel : public NetModel{
public:
    PlainRandomModel(shared_ptr<vector<FlowInputEntry>>& flows, 
                    ifstream& topo_file, 
                    const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                    const NodeContainer &nodes){
        readTopology(topo_file);
    }

    void insert_offsets(shared_ptr<vector<FlowInputEntry>> flows_ptr){
        for(auto& flow : *flows_ptr){
            flow.offset = ns3::NanoSeconds(get_rand(0, 100));
        }
    }

private:
    uint64_t get_rand(uint64_t range_start, uint64_t range_end){
        return range_start + (rand() % range_end);
    }

    // shared_ptr<vector<FlowInputEntry>>& flows_ptr;
};

} // namespace rand_offset
#endif /* PLAIN_RANDOM_MODEL_H */