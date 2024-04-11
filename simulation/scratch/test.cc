// #include "rand_offset_injector.h"
#include "matrix.h"
#include <iostream>
using namespace std;

void test_matrix(){
    double data[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    Matrix A(data);
    Matrix B = Eye(3);
    std::cout<<A <<endl << A+B << endl;

    double data2[2][2] = {{2,1},{7,4}};
    Matrix C(data2);
    std::cout<<Inverse(C)<<endl;

    double data3[2][2] = {{4,7},{2,6}};
    Matrix D(data3);
    std::cout<<Inverse(D)<<endl;
}

int main(int argc, char *argv[]){
    test_matrix();
    cout<<"Hello World"<<endl;
    // RandOffsetInjector injector;
    // injector.set_offset(100);
    // std::cout<<injector.get_offset_ns(0)<<std::endl;
}