#include "ns3/rand_offset_injector.h"


using namespace ns3;
using namespace std;
using namespace rand_offset;

void test_netcalc(){

}

int main(int argc, char *argv[])
{
    Curve curve1 = {{0, 0}, {1, 5}, {2, 10}};
    Curve curve2 = {{0, 0}, {1, 3}, {1.5, 7}, {2, 8}};
    Curve curve3 = {{0, 0}, {0.5, 2}, {1, 4}, {2, 9}};

    Curve aggregated_curve;

    // Add curves to the aggregate
    add_curve(aggregated_curve, curve1);
    add_curve(aggregated_curve, curve2);
    add_curve(aggregated_curve, curve3);
}