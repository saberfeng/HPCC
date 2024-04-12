#ifndef OPEN_JACKSON_MODEL_H
#define OPEN_JACKSON_MODEL_H

#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include "matrix.h"

namespace rand_offset{

using std::vector;
using std::string;
using std::ifstream;
using std::unordered_map;

struct FlowInputEntry{
    uint32_t src, dst, priority_group, dst_port; // pg: priority group
    uint64_t size_byte;
	double start_time_s;
};

struct HostFlowSum{
    uint32_t host_id;
    uint64_t sum_flow_size_byte;
    double start_time_s;
    double end_time_s;
};

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
    void initialize(const string& flow_file, const string& topo_file); // init input_rate_bps, service_rate_bps, routing_matrix, node_type
private:
    void initInputRate(ifstream &flow_f); 
    void initServiceRateRoutingMatrix(ifstream &flow_f);

    Vector<double> input_rate_bps; // byte per second
    Vector<double> service_rate_bps; // byte per second
    Matrix routing_matrix;
    vector<uint32_t> node_type; // 0: host, 1: switch
    unordered_map<uint32_t, vector<FlowInputEntry>> node2flows;
    unordered_map<uint32_t, HostFlowSum> node2flowsums;
};



} // namespace rand_offset
#endif /* OPEN_JACKSON_MODEL_H */