#include <iostream>
#include <fstream>
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/object-vector.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/global-value.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "ns3/random-variable.h"
#include "switch-mmu.h"

NS_LOG_COMPONENT_DEFINE("SwitchMmu");
namespace ns3 {
	TypeId SwitchMmu::GetTypeId(void){
		static TypeId tid = TypeId("ns3::SwitchMmu")
			.SetParent<Object>()
			.AddConstructor<SwitchMmu>();
		return tid;
	}

	void SwitchMmu::ConfigEnablePFC(bool enable){
		enable_pfc = enable;
	}

	int64_t SwitchMmu::GetInQLen(){
		int64_t in_qlen = 0;
		for (uint32_t port = 1; port <= pCnt; port++){
			for (uint32_t i = 0; i < qCnt; i++){
				in_qlen += ingress_bytes[port][i];
			}
		}
		return in_qlen;
	}

	int64_t SwitchMmu::GetOutQLen(){
		int64_t out_qlen = 0;
		for (uint32_t port = 1; port <= pCnt; port++){
			for (uint32_t i = 0; i < qCnt; i++){
				out_qlen += egress_bytes[port][i];
			}
		}
		return out_qlen;
	}

	void SwitchMmu::RecordInQLen(){
		Time now = Simulator::Now();
		if (now - monitor->swLastInRecordTime[node_id] >= monitor->record_interval){
			monitor->swLastInRecordTime[node_id] = now;
			monitor->swTimeToInQBytes[node_id].push_back(
				pair<uint64_t, int64_t>(
					Simulator::Now().GetNanoSeconds(), GetInQLen()));
		}
	}

	void SwitchMmu::RecordOutQLen(){
		Time now = Simulator::Now();
		if (now - monitor->swLastOutRecordTime[node_id] >= monitor->record_interval){
			monitor->swLastOutRecordTime[node_id] = now;
			monitor->swTimeToOutQBytes[node_id].push_back(
				pair<uint64_t, int64_t>(
					Simulator::Now().GetNanoSeconds(), GetOutQLen()));
		}
	}

	SwitchMmu::SwitchMmu(void){
		buffer_size = 12 * 1024 * 1024;
		reserve = 4 * 1024;
		resume_offset = 3 * 1024;
		enable_pfc = true;

		monitor = Monitor::GetInstance();
		monitor->swLastInRecordTime[node_id] = Simulator::Now();
		monitor->swLastOutRecordTime[node_id] = Simulator::Now();

		RecordInQLen();
		RecordOutQLen();

		// headroom
		shared_used_bytes = 0;
		memset(hdrm_bytes, 0, sizeof(hdrm_bytes));
		memset(ingress_bytes, 0, sizeof(ingress_bytes));
		memset(paused, 0, sizeof(paused));
		memset(egress_bytes, 0, sizeof(egress_bytes));
	}

	void SwitchMmu::NotifyDrop(uint32_t node_id, uint32_t port, uint32_t qIndex){
		auto pfc_notify = "Headroom full";
		auto nonpfc_notify = "Buffer full";
		printf("Time:%luus Node:%u Drop: queue:%u,%u: %s total:%ldbytes\n(port, ingress_bytes):\n", 
			Simulator::Now().GetMicroSeconds(), node_id, port, qIndex, 
			enable_pfc?pfc_notify:nonpfc_notify, buffer_size);
		printf("pfc_threshold:%ld, headroom:%ld\n", GetPfcThreshold(port), headroom[port]);
		for (uint32_t i = 1; i < 20; i++){
			if(enable_pfc){
				printf("(%ld,%ld)", hdrm_bytes[i][3], ingress_bytes[i][3]);
			} else {
				printf("(p%u, %ld, %.2f)", i, ingress_bytes[i][3], (double)ingress_bytes[i][3]/(double)buffer_size);
			}
		}
		printf("\n");	
	}

	bool SwitchMmu::CheckIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		if(enable_pfc){
			if (int64_t(psize) + hdrm_bytes[port][qIndex] > headroom[port] && 
				int64_t(psize) + total_hdrm + total_rsrv + shared_used_bytes > buffer_size){
				// psize + GetSharedUsed(port, qIndex) > GetPfcThreshold(port)){
				NotifyDrop(node_id, port, qIndex);
				return false;
			}
			return true;
		} else {
			if(int64_t(psize) + shared_used_bytes > buffer_size){
				NotifyDrop(node_id, port, qIndex);
				return false;
			} else {
				return true;
			}	
		}
	}

	bool SwitchMmu::CheckEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		return true;
	}

	void SwitchMmu::UpdateIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		if(enable_pfc){
			uint32_t new_bytes = ingress_bytes[port][qIndex] + int64_t(psize);
			if (new_bytes <= reserve){
				ingress_bytes[port][qIndex] += int64_t(psize);
			}else {
				uint32_t thresh = GetPfcThreshold(port);
				if (new_bytes - reserve > thresh){
					hdrm_bytes[port][qIndex] += int64_t(psize);
				}else {
					ingress_bytes[port][qIndex] += int64_t(psize);
					shared_used_bytes += std::min(int64_t(psize), new_bytes - reserve);
				}
			}
		} else {
			ingress_bytes[port][qIndex] += int64_t(psize);
			shared_used_bytes += int64_t(psize);
		}
		RecordInQLen();
	}

	void SwitchMmu::UpdateEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		egress_bytes[port][qIndex] += int64_t(psize);
		RecordOutQLen();
	}

	void SwitchMmu::RemoveFromIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		if(enable_pfc){
			int64_t from_hdrm = std::min(hdrm_bytes[port][qIndex], int64_t(psize));
			int64_t from_shared = std::min(int64_t(psize) - from_hdrm, ingress_bytes[port][qIndex] > reserve ? ingress_bytes[port][qIndex] - reserve : 0);
			hdrm_bytes[port][qIndex] -= from_hdrm;
			ingress_bytes[port][qIndex] -= int64_t(psize) - from_hdrm;
			shared_used_bytes -= from_shared;
		} else {
			// when pfc disabled, only use ingress_bytes
			ingress_bytes[port][qIndex] -= int64_t(psize); 
			shared_used_bytes -= int64_t(psize);
		}
		RecordInQLen();
	}

	void SwitchMmu::RemoveFromEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		egress_bytes[port][qIndex] -= int64_t(psize);
		RecordOutQLen();
	}

	bool SwitchMmu::CheckShouldPause(uint32_t port, uint32_t qIndex){
		if(enable_pfc){
			return !paused[port][qIndex] && (hdrm_bytes[port][qIndex] > 0 || GetSharedUsed(port, qIndex) >= GetPfcThreshold(port));
		} else {
			return false;
		}
	}

	bool SwitchMmu::CheckShouldResume(uint32_t port, uint32_t qIndex){
		if (!paused[port][qIndex])
			return false;
		int64_t shared_used = GetSharedUsed(port, qIndex);
		return hdrm_bytes[port][qIndex] == 0 && (shared_used == 0 || shared_used + resume_offset <= GetPfcThreshold(port));
	}

	void SwitchMmu::SetPause(uint32_t port, uint32_t qIndex){
		if(enable_pfc){
			paused[port][qIndex] = true;
		}
	}

	void SwitchMmu::SetResume(uint32_t port, uint32_t qIndex){
		paused[port][qIndex] = false;
	}

	int64_t SwitchMmu::GetPfcThreshold(uint32_t port){
		return (buffer_size - total_hdrm - total_rsrv - shared_used_bytes) >> pfc_a_shift[port];
	}

	int64_t SwitchMmu::GetSharedUsed(uint32_t port, uint32_t qIndex){
		int64_t used = ingress_bytes[port][qIndex];
		return used > reserve ? used - reserve : 0;
	}

	bool SwitchMmu::ShouldSendCN(uint32_t ifindex, uint32_t qIndex){
		if(enable_pfc){
			if (qIndex == 0)
				return false;
			if (egress_bytes[ifindex][qIndex] > kmax[ifindex])
				return true;
			if (egress_bytes[ifindex][qIndex] > kmin[ifindex]){
				double p = pmax[ifindex] * double(egress_bytes[ifindex][qIndex] - kmin[ifindex]) / (kmax[ifindex] - kmin[ifindex]);
				if (UniformVariable(0, 1).GetValue() < p)
					return true;
			}
			return false;
		} else {
			return false;
		}
	}

	void SwitchMmu::ConfigEcn(uint32_t port, uint32_t _kmin, uint32_t _kmax, double _pmax){
		kmin[port] = _kmin * 1000;
		kmax[port] = _kmax * 1000;
		pmax[port] = _pmax;
	}

	void SwitchMmu::ConfigHdrm(uint32_t port, int64_t size){
		headroom[port] = size;
	}

	void SwitchMmu::ConfigNPort(uint32_t n_port){
		total_hdrm = 0;
		total_rsrv = 0;
		for (uint32_t i = 1; i <= n_port; i++){
			total_hdrm += headroom[i];
			total_rsrv += reserve;
		}
	}

	void SwitchMmu::ConfigBufferSizeByte(int64_t size){
		buffer_size = size;
	}
}
