#include "netcalc_model.h"

namespace rand_offset{

void add_curve(std::vector<Point>& aggregate, const std::vector<Point>& curve){
    for (const auto& pt : curve) {
        auto it = std::lower_bound(aggregate.begin(), aggregate.end(), pt);
        if (it != aggregate.end() && it->x == pt.x) {
            // Existing point with the same x, add the y values
            it->y += pt.y;
        } else if(it != aggregate.end() && it->x != pt.x){
            // New unique point, insert it before the point that has a higher x
            // Check if insertion point is not the end and the x value is not the same
            aggregate.insert(it, Point{pt.x, pt.y+it->y});
        } else {
            // If it is the end, insert the point at the end
            aggregate.push_back(Point{pt.x, pt.y});
        }
    }
}

double evaluate_at(Curve& curve, double x) {
    // Simple linear interpolation between points
    for (size_t i = 0; i < curve.size() - 1; i++) {
        if (x >= curve[i].x && x <= curve[i+1].x) {
            double dy = curve[i+1].y - curve[i].y;
            double dx = curve[i+1].x - curve[i].x;
            return curve[i].y + (x - curve[i].x) * dy / dx;
        }
    }
    return 0; // Default return
}

void print_curve(const std::vector<Point>& curve) {
    for (const auto& pt : curve) {
        std::cout << "(" << pt.x << ", " << pt.y << ") ";
    }
    std::cout << std::endl;
}


void NetCalcModel::gen_offset(
    const vector<shared_ptr<FlowInputEntry>>& flows,
    const map<Ptr<Node>, 
            map<Ptr<Node>, vector<Ptr<Node>>>> &next_hop,
    const NodeContainer &node_container){
    
    for(int i = 0; i < flows.size(); i++){

    }
}

}