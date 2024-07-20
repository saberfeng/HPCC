#ifndef OPEN_JACKSON_MODEL_H
#define OPEN_JACKSON_MODEL_H

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
#include "net_model.h"

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


typedef vector<vector<uint32_t>> Matrix;
// struct FlowInput{
// 	uint32_t src, dst, pg, writeSizeByte, port, dport; // pg: priority group
// 	double start_time;
// 	uint32_t idx;
// };
struct HostFlowSum{
    NodeId host_id;
    uint64_t sum_flow_size_byte;
    Time start_time;
    Time end_time;
};


class OpenJacksonModel : public NetModel{
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
    void initialize(const vector<shared_ptr<FlowInputEntry>>& flows, 
                    ifstream& topo_file, 
                    const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>>& nextHop,
                    const NodeContainer &nodes);
    
    pair<vector<long double>, vector<long double>> calcStateProb(); // return utilization and node drop probability
    unordered_map<NodeId, vector<shared_ptr<FlowInputEntry>>>& getNode2Flows(){
        return node2flows;
    }

private:
    void buildNode2Flows(const vector<shared_ptr<FlowInputEntry>>& flows);

    void updateInputRate(const NodeContainer &node_container); 
    void updateServiceRate();
    void updateRoutingMatrix(
        const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop,
        const NodeContainer &nodes);
    void updateNode2FlowSums(
        const NodeContainer &node_container,
        const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop);

    uint64_t getBwByLinkNodeId(uint32_t link_src_id, uint32_t link_dst_id,
        const NodeContainer &node_container,
        const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop);
    

    vector<double> input_rate_Bps; // byte per second
    unordered_map<shared_ptr<FlowInputEntry>, vector<double>> flow2input_Bps;
    vector<double> service_rate_Bps; // byte per second
    // Matrix routing_matrix;
    
    unordered_map<NodeId, HostFlowSum> node2flowsums;
    
    // routing matrix:
    unordered_map<shared_ptr<FlowInputEntry>, Matrix> flow2routing_matrix;
};



} // namespace rand_offset
#endif /* OPEN_JACKSON_MODEL_H */