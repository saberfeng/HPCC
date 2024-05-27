#include "rand_offset_injector.h"

namespace rand_offset{

void RandOffsetInjector::initialize(ifstream& flow_file, ifstream& topo_file,
                                const map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop,
                                const NodeContainer &node_container){
    jackson_model.initialize(flow_file, topo_file, next_hop, node_container);
}


}// namespace rand_offset