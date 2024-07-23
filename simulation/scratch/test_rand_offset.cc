#include <iostream>
#include <fstream>
#include <unordered_map>
#include <time.h> 
#include <memory>
#include "ns3/core-module.h"
#include "ns3/qbb-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/packet.h"
#include "ns3/error-model.h"
#include <ns3/rdma.h>
#include <ns3/rdma-client.h>
#include <ns3/rdma-client-helper.h>
#include <ns3/rdma-driver.h>
#include <ns3/switch-node.h>
#include <ns3/sim-setting.h>
#include "ns3/netcalc_model.h"
#include "ns3/rand_offset_injector.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"

using namespace ns3;
using namespace std;
using namespace rand_offset;


void test_evaluate_at(){
    ArrivalCurve curve1 = {{0, 0}, {1, 5}, {3, 10}};
    ArrivalCurve curve2 = {{0, 0},{1, 0}, {2, 7}, {4, 7}};
    curve1.evaluate_at(0);
}

void test_netcalc(){
    // Test Case 0: 
    ArrivalCurve curve1 = {{0, 0}, {1, 5}, {2, 10}};
    ArrivalCurve curve2 = {{0, 0}, {1, 3}, {1.5, 7}, {2, 8}};
    ArrivalCurve curve3 = {{0, 0}, {0.5, 2}, {1, 4}, {2, 9}};
    ArrivalCurve result = {{0, 0}, {0.5, 6}, {1, 12}, {1.5, 21}, 
                           {2, 27}};
    curve1.add_curve(curve2);
    curve1.add_curve(curve3);
    std::cout << "Aggregated Curve: ";
    curve1.print();
    assert(curve1 == result);

    // Test Case 1: Basic Overlap
    curve1 = ArrivalCurve({{0, 0},{1, 5}, {2, 10}});
    curve2 = ArrivalCurve({{0, 0},{1, 3}, {2, 8}});
    result = ArrivalCurve({{0, 0},{1, 8}, {2, 18}});
    curve1.add_curve(curve2);
    std::cout << "Test Case 1 - Basic Overlap: ";
    curve1.print();
    assert(curve1 == result);

    // Test Case 2: No Overlap
    curve1 = ArrivalCurve({{0, 0}, {1, 5}});
    curve2 = ArrivalCurve({{0, 0}, {1.75, 0}, {2, 3}, {3, 8}});
    result = ArrivalCurve({{0, 0}, {1, 5}, {1.75, 5}, {2, 8}, {3, 13}});
    curve1.add_curve(curve2);
    std::cout << "Test Case 2 - No Overlap: ";
    curve1.print();
    assert(curve1 == result);

    // Test Case 3: Mixed Overlap
    curve1 = ArrivalCurve({{0, 0}, {1, 5}, {3, 10}});
    curve2 = ArrivalCurve({{0, 0}, {1, 0}, {2, 7}, {4, 7}});
    result = ArrivalCurve({{0, 0}, {1, 5}, {2, 14.5}, {3, 17}, {4, 17}});
    curve1.add_curve(curve2);
    std::cout << "Test Case 3 - Mixed Overlap: ";
    curve1.print();
    assert(curve1 == result);

    // Test Case 4: Boundary Values
    curve1 = ArrivalCurve({{0, 0},{2.5, 10}});
    curve2 = ArrivalCurve({{0, 0},{5, 5}});
    result = ArrivalCurve({{0, 0},{2.5, 12.5}, {5, 15}});
    curve1.add_curve(curve2);
    std::cout << "Test Case 4 - Boundary Values: ";
    curve1.print();
    assert(curve1 == result);

    // Test Case 5: Sequential Addition
    curve1 = ArrivalCurve({{0, 0},{1, 1}});
    curve2 = ArrivalCurve({{0, 0},{1, 2}});
    curve3 = ArrivalCurve({{0, 0},{1, 3}});
    result = ArrivalCurve({{0, 0},{1, 6}});
    curve1.add_curve(curve2);
    curve1.add_curve(curve3);
    std::cout << "Test Case 5 - Sequential Addition: ";
    curve1.print();
    assert(curve1 == result);
}

void WeirdCodeMustKeepToPassLinking(){
    RdmaClientHelper a;
}


int main(int argc, char *argv[])
{
	test_netcalc();
	return 0;
}
