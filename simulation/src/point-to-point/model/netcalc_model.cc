#include "netcalc_model.h"

namespace rand_offset{


void NetCalcModel::gen_offset(
    const vector<shared_ptr<FlowInputEntry>>& flows,
    const map<Ptr<Node>, 
            map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop,
    const NodeContainer &node_container){
    
    for(int i = 0; i < flows.size(); i++){

    }
}

}