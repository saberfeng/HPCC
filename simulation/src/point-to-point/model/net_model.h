#ifndef NET_MODEL_H
#define NET_MODEL_H

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

// struct FlowInput{
// 	uint32_t src, dst, pg, writeSizeByte, port, dport; // pg: priority group
// 	double start_time;
// 	uint32_t idx;
// };
struct FlowInputEntry{
    uint32_t src, dst, priority_group, src_port, dst_port; // pg: priority group
    uint64_t size_byte;
	Time start_time;
    uint32_t flow_idx;
    Time offset;
public:
    Time getOffsetStart() const {
        return start_time + offset;
    }

    friend std::ostream& operator<<(std::ostream& out, FlowInputEntry& flow){
        out << "F[id" << flow.flow_idx 
            << " src" << flow.src << " dst" << flow.dst
            << " p" << flow.priority_group << "sp" << flow.src_port
            << "dp" << flow.dst_port 
            << " T" << flow.start_time << " Of" << flow.offset << "]";
        return out;
    }

    string get_str(){
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }
};

struct NodeEntry{
    uint32_t node_type;
    // long double bandwidth_Bps;
    // 
};

struct Link{
    NodeId src, dst;
    long double bandwidth_Bps;
    uint64_t queue_size_byte;
};

typedef std::unordered_map<uint32_t, std::unordered_map<uint32_t, Link>> Topology;


class NetModel{
public: 
    NetModel(){}
    void readTopology(ifstream &topo_file);
    uint64_t getQueSzByBw(long double bandwidth_Bps);

protected:
    vector<NodeEntry> node_info; // 0: host, 1: switch
    unordered_map<NodeId, vector<shared_ptr<FlowInputEntry>>> node2flows;
    Topology topology;
    uint32_t num_flows;

};

Time transTime(long double bandwidth_Bps, uint64_t size_byte);

vector<Ptr<Node>> getPathFromNextHop(
    const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
    Ptr<Node> src, Ptr<Node> dst);
                            
} // namespace rand_offset
#endif /* NET_MODEL_H */