#ifndef RAND_OFFSET_INJECTOR_H
#define RAND_OFFSET_INJECTOR_H

#include <iostream>
#include <unordered_map>

using std::unordered_map;
using std::cout;
using std::endl;

class RandOffsetInjector {
public:
    RandOffsetInjector(){
        std::cout<<"RandOffsetInjector constructor"<<std::endl;
    }
    void set_offset(double offset){
        this->offset = offset;
    }
    double get_offset_ns(uint32_t node_idx){
        return 100;
    }
private:
    double offset;
};

#endif /* RAND_OFFSET_INJECTOR_H */