#include "ns3/rand_offset_injector.h"

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
    ArrivalCurve result = {{0, 0}, {1, 12}, {1.5, 17}, {3, 27}};

    // Add curves to the aggregate
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
    curve2 = ArrivalCurve({{0, 0},{1, 0}, {2, 7}, {4, 7}});
    result = ArrivalCurve({{}});
    
    curve1.add_curve(curve2);
    std::cout << "Test Case 3 - Mixed Overlap: ";
    curve1.print();
    assert(curve1 == result);

    // Test Case 4: Boundary Values
    curve1 = ArrivalCurve({{0, 0},{0, 10}});
    curve2 = ArrivalCurve({{0, 0},{5, 5}});
    curve1.add_curve(curve2);
    std::cout << "Test Case 4 - Boundary Values: ";
    curve1.print();
    assert(curve1 == result);

    // Test Case 5: Sequential Addition
    curve1 = ArrivalCurve({{0, 0},{1, 1}});
    curve2 = ArrivalCurve({{0, 0},{1, 2}});
    curve3 = ArrivalCurve({{0, 0},{1, 3}});
    curve1.add_curve(curve2);
    curve1.add_curve(curve3);
    std::cout << "Test Case 5 - Sequential Addition: ";
    curve1.print();
    assert(curve1 == result);

    // Test Case 6: Single Point Curves
    curve1 = ArrivalCurve({{0, 0},{0, 10}});
    curve2 = ArrivalCurve({{0, 0},{0, 20}});
    curve1.add_curve(curve2);
    std::cout << "Test Case 6 - Single Point Curves: ";
    curve1.print();
    assert(curve1 == result);

    // Test Case 7: Descending Order
    curve1 = ArrivalCurve({{0, 0},{3, 10}, {2, 5}, {1, 15}});
    curve2 = ArrivalCurve({{0, 0},{2, 10}, {1, 5}, {0, 20}});
    curve1.add_curve(curve2);
    std::cout << "Test Case 7 - Descending Order: ";
    curve1.print();
    assert(curve1 == result);
}

int main(int argc, char *argv[])
{
    test_netcalc();
}