#ifndef OPEN_JACKSON_MODEL_H
#define OPEN_JACKSON_MODEL_H

#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include "matrix.h"
#include "ns3/core-module.h"
#include "ns3/node.h"
#include "ns3/ptr.h"
#include "ns3/node-container.h"

namespace rand_offset{

using std::vector;
using std::string;
using std::ifstream;
using std::unordered_map;
using ns3::Node;
using ns3::Ptr;
using ns3::NodeContainer;
using std::pair;
using std::map;

typedef uint32_t NodeId;
typedef uint32_t FlowId;

struct FlowInputEntry{
    uint32_t src, dst, priority_group, dst_port; // pg: priority group
    uint64_t size_byte;
	double start_time_s;
    FlowId flow_idx;
};

struct HostFlowSum{
    NodeId host_id;
    uint64_t sum_flow_size_byte;
    double start_time_s;
    double end_time_s;
};

struct NodeEntry{
    uint32_t node_type;
    uint64_t bandwidth_Bps;
    uint64_t queue_size_byte;
};

struct Link{
    NodeId src, dst;
    uint64_t bandwidth_Bps;
};

typedef std::unordered_map<uint32_t, std::unordered_map<uint32_t, Link>> Topology;
class OpenJacksonModel {
public:
    OpenJacksonModel(){}
    /*idea: 
        when initializing input rate, don't directly use bytes as length. 
        Instead, use smallest chunk of data that can be sent in one packet, or the 
            smallest usually seen data size as a unit.
        Then, when calculating the service rate, use the same unit.
        -> reduce the probability calculation time complexity 
    */ 
    // init input_rate_Bps, service_rate_Bps, routing_matrix, node_type
    void initialize(ifstream& flow_file, ifstream& topo_file, 
                    const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                    const NodeContainer &nodes);
    
    pair<vector<long double>, vector<long double>> calcStateProb(); // return utilization and node drop probability

private:
    void readTopology(ifstream &topo_file);
    void readFlows(ifstream &flow_file);

    void initInputRate(); 
    void initServiceRate();
    void initRoutingMatrix(
        const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop,
        const NodeContainer &nodes);

    Vector<double> input_rate_Bps; // byte per second
    Vector<double> service_rate_Bps; // byte per second
    Matrix routing_matrix;
    vector<NodeEntry> node_info; // 0: host, 1: switch
    
    unordered_map<NodeId, vector<FlowInputEntry>> node2flows;
    unordered_map<NodeId, HostFlowSum> node2flowsums;
    
    // routing matrix:
    unordered_map<FlowId, Matrix> flow2routing_matrix;
    Topology topology;
};



} // namespace rand_offset
#endif /* OPEN_JACKSON_MODEL_H */