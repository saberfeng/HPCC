#ifndef OPEN_JACKSON_MODEL_H
#define OPEN_JACKSON_MODEL_H

#include <vector>
#include <cassert>

using Mat = std::vector<std::vector<double>>;


class OpenJacksonModel {
public:
    OpenJacksonModel(){
        std::cout<<"OpenJacksonModel constructor"<<std::endl;
    }
    void set_offset(double offset){
        this->offset = offset;
    }
    double get_offset_ns(uint32_t node_idx){
        return 100;
    }
    
};


#endif /* OPEN_JACKSON_MODEL_H */