#include "monitor.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Monitor");
Ptr<Monitor> Monitor::m_instance = nullptr;

TypeId Monitor::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::Monitor")
        .SetParent<Object>()
        .AddConstructor<Monitor>();
    return tid;
}

Ptr<Monitor> Monitor::GetInstance(void) {
    if (m_instance == nullptr) {
        m_instance = CreateObject<Monitor>();
    }
    return m_instance;
}

Monitor::Monitor() {
    // Initialize your monitoring state
    record_interval = NanoSeconds(6*1e5);
}

Monitor::~Monitor() {
    // Cleanup if needed
}

void Monitor::LogEvent(const std::string& event) {
    m_events.push_back(event);
}

void Monitor::OutputToFile(const std::string& filename) {
    std::ofstream outfile(filename);
    for (const auto& event : m_events) {
        outfile << event << std::endl;
    }
    outfile.close();
}

void Monitor::Clear() {
    m_events.clear();
}

void Monitor::output_sw_monitor_pfc(string filename){
    FILE* fout = fopen(filename.c_str(), "w");
    for (auto &sw_pfc_series : swPortTimePFCSeries){
        for (auto &port_pfc_series : sw_pfc_series.second){
            vector<pair<uint64_t, uint32_t>> TimePFCSeries = port_pfc_series.second;
            // output portTimePFCSeries in csv format
            // switch_id, port_id, time, pfc_type
            if (TimePFCSeries.size() == 0){
                continue;
            }
            for (auto &pfc_pair : TimePFCSeries){
                fprintf(fout, "%u,%u,%lu,%u\n", 
                        sw_pfc_series.first, port_pfc_series.first, pfc_pair.first, pfc_pair.second);
            }
        }
    }
    fclose(fout);
}

void Monitor::output_sw_monitor_ecn(string filename){
    FILE* fout = fopen(filename.c_str(), "w");
    for (auto &sw_ecn_series : swPortEcnCnt){
        // output ifEcnSeries in csv format
        // switch_id, port_id, count
        if (sw_ecn_series.second.size() == 0){
            continue;
        }
        for (auto &ecn_cnt : sw_ecn_series.second){
            if (ecn_cnt.second == 0){
                continue; // skip if ecn count is 0
            }
            fprintf(fout, "%u,%u,%lu\n", sw_ecn_series.first, ecn_cnt.first, ecn_cnt.second);
        }
    }
    fclose(fout);
}

void Monitor::output_swmmu_inqbytes(string filename){
    FILE* fout = fopen(filename.c_str(), "w");
	for (auto &sw_inq_series : swTimeToInQBytes){
		// output timeNsToInQBytes in csv format
		// switch_id, time, bytes
		if (sw_inq_series.second.size() == 0){
            continue;
		}
		for (auto &inq_pair : sw_inq_series.second){
			fprintf(fout, "%u,%lu,%lu\n", sw_inq_series.first, inq_pair.first, inq_pair.second);
		}
	}
    fclose(fout);
}

void Monitor::output_swmmu_outqbytes(string filename){
    FILE* fout = fopen(filename.c_str(), "w");
	for (auto &sw_outq_series : swTimeToOutQBytes){
		// output timeNsToOutQBytes in csv format
		// switch_id, time, bytes
		if (sw_outq_series.second.size() == 0){
            continue;
		}
		for (auto &outq_pair : sw_outq_series.second){
			fprintf(fout, "%u,%lu,%lu\n", sw_outq_series.first, outq_pair.first, outq_pair.second);
		}
	}
    fclose(fout);
}

void Monitor::output_rdma_hw_rate(string filename){
    FILE* fout = fopen(filename.c_str(), "w");
	for (auto &node_rate_series : nodeTimeToRate){
		// output rateSeries in csv format
		// node_id, time, rate
		if (node_rate_series.second.size() == 0){
            continue;
		}
		for (auto &rate_pair : node_rate_series.second){
			fprintf(fout, "%u,%lu,%lu\n", node_rate_series.first, rate_pair.first, rate_pair.second);
		}
	}
    fclose(fout);
}
} // namespace ns3
