#include "netcalc_model.h"

namespace rand_offset{



bool Curve::operator==(const Curve& other) const {
    if (this->points.size() != other.points.size()) return false;
    for (size_t i = 0; i < this->points.size(); ++i) {
        if (!(this->points[i] == other.points[i])) return false;
    }
    return true;
}

double Curve::evaluate_at(double x) const {
        auto it = std::lower_bound(points.begin(), points.end(), 
                                    Point({x,0}));
        if(it == points.end()){
            throw runtime_error("evaluate at point error");
        } else if (it == points.begin() && Point({x,0}) < *it){
            // x is at the left of whole curve
            return 0;
        } else if (Point({x,0}) == *it){
            // x == a point of curve
            return it->y;
        } else{
            // x is between (it-1, it)
            // Simple linear interpolation between points
            double dy = it->y - (it-1)->y;
            double dx = it->x - (it-1)->x;
            return (it-1)->y + (x - (it-1)->x) * dy / dx;
        }
    }

void ArrivalCurve::add_curve(const Curve& curve){
    vector<TaggedPoint> taggedPtsThis, taggedPtsOther;
    for(const Point& pt : this->points){
        taggedPtsThis.emplace_back(pt, 1);
    }
    for(const Point& pt: curve.points){
        taggedPtsOther.emplace_back(pt, 2);
    }

    vector<TaggedPoint> merged_pts;
    merged_pts.reserve(this->points.size()+curve.points.size());
    std::merge(taggedPtsThis.begin(), taggedPtsThis.end(), 
                taggedPtsOther.begin(), taggedPtsOther.end(), merged_pts.begin());
    int left_curve_idx;
    // first two points in merged_pts must be (0,0)
    assert(this->points[0] == Point({0, 0}));
    assert(this->points[0] == curve.points[0]);

    for(int i = 0; i < merged_pts.size(); i++){
        TaggedPoint& tag_pt = merged_pts[i];
        if(i != 0 && tag_pt.pt.x == merged_pts[i-1].pt.x){
            // if this and previous points on same x
            continue;
        }
        if(tag_pt.origin == 1){ // point on *this curve
            double other_y = curve.evaluate_at(tag_pt.pt.x);
            tag_pt.pt.y += other_y;
        } else if (tag_pt.origin == 2){ // point on other curve
            double this_y = this->evaluate_at(tag_pt.pt.x);
            tag_pt.pt.y += this_y;
        } else {
            throw runtime_error("tag_pt origin error");
        }
    }

    this->points.clear();
    for(auto& tag_pt : merged_pts){
        this->points.emplace_back(tag_pt.pt);
    }    
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