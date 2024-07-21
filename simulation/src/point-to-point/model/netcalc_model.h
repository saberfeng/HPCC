#ifndef NETCALC_MODEL_H
#define NETCALC_MODEL_H

#include <memory>
#include <vector>
#include <algorithm>
#include "net_model.h"

namespace rand_offset{

using namespace std;

struct Point{
    double x, y;

    bool operator<(const Point& other) const {
        return x < other.x;
    }

    bool operator==(const Point& other) const {
        const double tolerance = 1e-6;
        return std::fabs(this->x - other.x) < tolerance && 
               std::fabs(this->y - other.y) < tolerance;
    }
};

typedef std::vector<Point> Curve;
bool operator==(const Curve& curve1, const Curve& curve2);

// Aggregates the curve values into an existing curve
void add_curve(Curve& aggregate, const Curve& curve); 

double evaluate_at(Curve& curve, double x); 

void print_curve(const Curve& curve);

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