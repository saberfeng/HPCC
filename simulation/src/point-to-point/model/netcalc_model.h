#ifndef NETCALC_MODEL_H
#define NETCALC_MODEL_H

#include <memory>
#include <vector>
#include "net_model.h"

namespace rand_offset{

using namespace std;

class NetCalcModel : public NetModel{
public:
    NetCalcModel(const vector<shared_ptr<FlowInputEntry>>& flows, 
                ifstream &topo_file,
                const map<Ptr<Node>, 
                          map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop,
                const NodeContainer &node_container){

        num_flows = flows.size();
        readTopology(topo_file);
    }

    void gen_offset(const vector<shared_ptr<FlowInputEntry>>& flows,
                    const map<Ptr<Node>, 
                          map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop,
                    const NodeContainer &node_container);
private:

};

} // namespace rand_offset

#endif /* NETCALC_MODEL_H */