#ifndef RAND_OFFSET_INJECTOR_H
#define RAND_OFFSET_INJECTOR_H

#include <iostream>
#include <unordered_map>
#include <memory>
#include <random>
#include "open_jackson_model.h"
#include "netcalc_model.h"

namespace rand_offset{

using std::unordered_map;
using std::cout;
using std::endl;
using std::shared_ptr;
using namespace ns3;

class RandOffsetInjector {
public:
    RandOffsetInjector(){
        std::cout<<"RandOffsetInjector constructor"<<std::endl;
        // std::random_device rd;
    }

    void initialize(const vector<shared_ptr<FlowInputEntry>>& flows, 
                    ifstream& topo_file,
                    const map<Ptr<Node>, map<Ptr<Node>, 
                            vector<Ptr<Node>>>> &next_hop,
                    const NodeContainer &node_container);

    double get_offset_ns(uint32_t node_idx, uint64_t range_ns){
        return get_rand(0, range_ns);
    }

    void gen_offset();

    uint64_t get_rand(uint64_t range_start, uint64_t range_end){
        return range_start + (rand() % range_end);
    }

    pair<vector<long double>, vector<long double>> calcStateProb();

private:
    void init_flow2range_s();
    // unordered_map<shared_ptr<FlowInputEntry>, 
    //               long double> flow2range_s;
    // OpenJacksonModel jackson_model;
    // std::mt19937 gen;
};
}// namespace rand_offset
#endif /* RAND_OFFSET_INJECTOR_H */