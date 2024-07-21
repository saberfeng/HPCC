#include "ns3/rand_offset_injector.h"

using namespace ns3;
using namespace std;
using namespace rand_offset;


void test_netcalc(){
    Curve curve1 = {{0, 0}, {1, 5}, {2, 10}};
    Curve curve2 = {{0, 0}, {1, 3}, {1.5, 7}, {2, 8}};
    Curve curve3 = {{0, 0}, {0.5, 2}, {1, 4}, {2, 9}};

    Curve aggregated_curve;

    // Add curves to the aggregate
    add_curve(aggregated_curve, curve1);
    add_curve(aggregated_curve, curve2);
    add_curve(aggregated_curve, curve3);

    std::cout << "Aggregated Curve: ";
    print_curve(aggregated_curve);

    // Test Case 1: Basic Overlap
    curve1 = {{1, 5}, {2, 10}};
    curve2 = {{1, 3}, {2, 8}};
    aggregated_curve.clear();
    add_curve(aggregated_curve, curve1);
    add_curve(aggregated_curve, curve2);
    std::cout << "Test Case 1 - Basic Overlap: ";
    print_curve(aggregated_curve);

    // Test Case 2: No Overlap
    aggregated_curve.clear();
    curve1 = {{0, 0}, {1, 5}};
    curve2 = {{2, 3}, {3, 8}};
    add_curve(aggregated_curve, curve1);
    add_curve(aggregated_curve, curve2);
    std::cout << "Test Case 2 - No Overlap: ";
    print_curve(aggregated_curve);

    // Test Case 3: Mixed Overlap
    aggregated_curve.clear();
    curve1 = {{0, 0}, {1, 5}, {3, 10}};
    curve2 = {{1, 3}, {2, 7}, {4, 1}};
    add_curve(aggregated_curve, curve1);
    add_curve(aggregated_curve, curve2);
    std::cout << "Test Case 3 - Mixed Overlap: ";
    print_curve(aggregated_curve);

    // Test Case 4: Boundary Values
    aggregated_curve.clear();
    curve1 = {{0, 10}};
    curve2 = {{5, 5}};
    add_curve(aggregated_curve, curve1);
    add_curve(aggregated_curve, curve2);
    std::cout << "Test Case 4 - Boundary Values: ";
    print_curve(aggregated_curve);

    // Test Case 5: Sequential Addition
    aggregated_curve.clear();
    curve1 = {{1, 1}};
    curve2 = {{1, 2}};
    curve3 = {{1, 3}};
    add_curve(aggregated_curve, curve1);
    add_curve(aggregated_curve, curve2);
    add_curve(aggregated_curve, curve3);
    std::cout << "Test Case 5 - Sequential Addition: ";
    print_curve(aggregated_curve);

    // Test Case 6: Single Point Curves
    aggregated_curve.clear();
    curve1 = {{0, 10}};
    curve2 = {{0, 20}};
    add_curve(aggregated_curve, curve1);
    add_curve(aggregated_curve, curve2);
    std::cout << "Test Case 6 - Single Point Curves: ";
    print_curve(aggregated_curve);

    // Test Case 7: Descending Order
    aggregated_curve.clear();
    curve1 = {{3, 10}, {2, 5}, {1, 15}};
    curve2 = {{2, 10}, {1, 5}, {0, 20}};
    std::sort(curve1.begin(), curve1.end());
    std::sort(curve2.begin(), curve2.end());
    add_curve(aggregated_curve, curve1);
    add_curve(aggregated_curve, curve2);
    std::cout << "Test Case 7 - Descending Order: ";
    print_curve(aggregated_curve);
}

int main(int argc, char *argv[])
{
    test_netcalc();
}