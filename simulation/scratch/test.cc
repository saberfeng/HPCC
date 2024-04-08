#include "rand_offset_injector.h"

int main(int argc, char *argv[]){
    RandOffsetInjector injector;
    injector.set_offset(100);
    std::cout<<injector.get_offset_ns(0)<<std::endl;
}