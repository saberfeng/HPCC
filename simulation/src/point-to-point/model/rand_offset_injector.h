#ifndef RAND_OFFSET_INJECTOR_H
#define RAND_OFFSET_INJECTOR_H

#include <iostream>
#include <unordered_map>
#include "open_jackson_model.h"

namespace rand_offset{

using std::unordered_map;
using std::cout;
using std::endl;
using namespace ns3;

class RandOffsetInjector {
public:
    RandOffsetInjector(){
        std::cout<<"RandOffsetInjector constructor"<<std::endl;
    }
    void initialize(const vector<shared_ptr<FlowInputEntry>>& flows, 
                    ifstream& topo_file,
                    const map<Ptr<Node>, map<Ptr<Node>, 
                            vector<Ptr<Node>>>> &next_hop,
                    const NodeContainer &node_container);

    void set_offset(double offset){
        this->offset = offset;
    }

    double get_offset_ns(uint32_t node_idx){
        return 100;
    }

    void gen_offset();

    pair<vector<long double>, vector<long double>> calcStateProb();

private:
    void init_flow2range_s();
    unordered_map<shared_ptr<FlowInputEntry>, 
                  long double> flow2range_s;
    OpenJacksonModel jackson_model;
};

}// namespace rand_offset
#endif /* RAND_OFFSET_INJECTOR_H */