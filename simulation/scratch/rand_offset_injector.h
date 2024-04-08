#ifndef RAND_OFFSET_INJECTOR_H
#define RAND_OFFSET_INJECTOR_H

#include <iostream>

class RandOffsetInjector {
public:
    RandOffsetInjector(){
        std::cout<<"RandOffsetInjector constructor"<<std::endl;
    }
    void set_offset(double offset);
    double get_offset();
    double get_injected_value(double value);
private:
    double offset;
};

#endif /* RAND_OFFSET_INJECTOR_H */