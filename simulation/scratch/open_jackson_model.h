#ifndef OPEN_JACKSON_MODEL_H
#define OPEN_JACKSON_MODEL_H

#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>
#include "matrix.h"


class OpenJacksonModel {
public:
    OpenJacksonModel(){
        std::cout<<"OpenJacksonModel constructor"<<std::endl;
    }
    /*idea: 
        when initializing input rate, don't directly use bytes as length. 
        Instead, use smallest chunk of data that can be sent in one packet, or the 
            smallest usually seen data size as a unit.
        Then, when calculating the service rate, use the same unit.
        -> reduce the probability calculation time complexity 
    */ 

    void initInputRate(std::ifstream &flow_file); 
    void initServiceRateRoutingMatrix(std::ifstream &topo_file);
private:
    Vector<double> input_rate_bps; // byte per second
    Vector<double> service_rate_bps; // byte per second
    Matrix routing_matrix;
};


#endif /* OPEN_JACKSON_MODEL_H */