#ifndef NETCALC_MODEL_H
#define NETCALC_MODEL_H

#include <memory>
#include <vector>
#include <algorithm>
#include "ns3/net_model.h"
#include <limits>
#include <initializer_list>
#include <cassert>

namespace rand_offset{

using namespace std;



struct Point{
    double x, y;
    static constexpr double tolerance = 1e-6;

    Point(double x = 0, double y = 0) : x(x), y(y) {}

    bool operator==(const Point& other) const {
        return std::abs(this->x - other.x) < tolerance && 
               std::abs(this->y - other.y) < tolerance;
    }

    bool operator<(const Point& other) const {
        if(*this == other){
            // in case the difference is within tolerance
            return false; 
        } else {
            return x < other.x;
        }
    }
};

struct TaggedPoint {
    Point pt;
    int origin;  

    TaggedPoint(Point pt, int o) : pt(pt), origin(o) {}

    bool operator<(const TaggedPoint& other) const {
        return pt < other.pt;
    }
};

class Curve{
public:
    Curve(std::initializer_list<Point> init) : points(init) {
        assert(points[0] == Point({0, 0})); // Curves should start from (0,0)
        std::sort(points.begin(), points.end());
    }

    bool operator==(const Curve& other) const; 

    void print() const {
        for (const auto& point : points) {
            std::cout << "(" << point.x << ", " << point.y << ") ";
        }
        std::cout << std::endl;
    }

    void clear(){points.clear();}

    virtual void add_curve(const Curve& curve) = 0;

    double evaluate_at(double x) const; 

public:
    std::vector<Point> points;
};

class ArrivalCurve: public Curve{
public:
    ArrivalCurve(std::initializer_list<Point> init) : Curve(init){
        assert(points.size()>=2);
        // for arrival curve, the first point must be (0, 0) 
        assert(points[0] == Point({0, 0}));
        // the last point must have infinite x and same y as the next to last point
        // check if not already have, insert an infinite ponit
        const vector<Point>::iterator& it_last = points.end() - 1;
        if(it_last->x != std::numeric_limits<double>::max()){
            points.insert(points.end(), 
                          Point({std::numeric_limits<double>::max(), it_last->y}));
        }
    }

    virtual void add_curve(const Curve& curve);
};

class ServiceCurve: public Curve{
public:
    ServiceCurve(std::initializer_list<Point> init) : Curve(init){
        // for service curve, the first point also must be (0, 0) 
        assert(points[0] == Point({0, 0}));
    }
};


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
