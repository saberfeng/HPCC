#ifndef MONITOR_H
#define MONITOR_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/type-id.h"
#include "ns3/nstime.h"
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>

namespace ns3 {

using std::vector;
using std::pair;
using std::unordered_map;
using std::string;
class Monitor : public Object {
public:
    static TypeId GetTypeId(void);
    static Ptr<Monitor> GetInstance(void);

    Monitor();
    virtual ~Monitor();

    // Add event to log
    void LogEvent(const std::string& event);
    
    // Output methods
    void OutputToFile(const std::string& filename);
    void Clear(); // Clear all stored data

private:
    static Ptr<Monitor> m_instance;
    std::vector<std::string> m_events;

public:
    void output_sw_monitor_pfc(string filename);
    void output_sw_monitor_ecn(string filename);
    void output_swmmu_inqbytes(string filename);
    void output_swmmu_outqbytes(string filename);
    void output_rdma_hw_rate(string filename);

    Time record_interval;
    // ------------ monitor switch_node --------------
    unordered_map<uint32_t, // switch id
        unordered_map<uint32_t, // port id
            vector<pair<uint64_t, uint32_t>>>> swPortTimePFCSeries; // pfc bytes; PFC: 0 for pause, 1 for resume
    
    unordered_map<uint32_t, // switch id
        unordered_map<uint32_t, // port id
            uint64_t>> swPortEcnCnt; // ecn cnt
    
    // ------------ monitor switch_mmu --------------
    unordered_map<uint32_t, // switch id
        vector<pair<uint64_t, int64_t>>> swTimeToInQBytes; // ingress queue length
    unordered_map<uint32_t, // switch id
        vector<pair<uint64_t, int64_t>>> swTimeToOutQBytes; // egress queue length
    unordered_map<uint32_t, Time> swLastInRecordTime;
    unordered_map<uint32_t, Time> swLastOutRecordTime;

    // ------------ monitor rdma_hw --------------
    unordered_map<uint32_t, // node id
        vector<std::pair<uint64_t, uint64_t>>> nodeTimeToRate; // rate
    unordered_map<uint32_t, Time> nodeRateLastRecordTime;
};

} // namespace ns3

#endif // MONITOR_H