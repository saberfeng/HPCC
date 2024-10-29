#ifndef SWITCH_MMU_H
#define SWITCH_MMU_H

#include <unordered_map>
#include <ns3/node.h>
#include <ns3/monitor.h>
namespace ns3 {

class Packet;
using std::vector;
using std::pair;

class SwitchMmu: public Object{
public:
	static const uint32_t pCnt = 257;	// Number of ports used
	static const uint32_t qCnt = 8;	// Number of queues/priorities used

	static TypeId GetTypeId (void);

	SwitchMmu(void);

	bool CheckIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	bool CheckEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	void UpdateIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	void UpdateEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	void RemoveFromIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	void RemoveFromEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);

	bool CheckShouldPause(uint32_t port, uint32_t qIndex);
	bool CheckShouldResume(uint32_t port, uint32_t qIndex);
	void SetPause(uint32_t port, uint32_t qIndex);
	void SetResume(uint32_t port, uint32_t qIndex);
	//void GetPauseClasses(uint32_t port, uint32_t qIndex);
	//bool GetResumeClasses(uint32_t port, uint32_t qIndex);

	int64_t GetPfcThreshold(uint32_t port);
	int64_t GetSharedUsed(uint32_t port, uint32_t qIndex);

	bool ShouldSendCN(uint32_t ifindex, uint32_t qIndex);

	void ConfigEcn(uint32_t port, uint32_t _kmin, uint32_t _kmax, double _pmax);
	void ConfigHdrm(uint32_t port, int64_t size);
	void ConfigNPort(uint32_t n_port);
	void ConfigBufferSizeByte(int64_t size);
	void ConfigEnablePFC(bool enable_pfc);
	void NotifyDrop(uint32_t node_id, uint32_t port, uint32_t qIndex);

	int64_t GetInQLen();
	int64_t GetOutQLen();

	void RecordInQLen();
	void RecordOutQLen();

	// config
	// when pfc disabled, packets from all ports of all priorities
	// 		share one large buffer
	bool enable_pfc;
	uint32_t node_id;
	int64_t buffer_size;
	uint32_t pfc_a_shift[pCnt];
	int64_t reserve;
	int64_t headroom[pCnt];
	uint32_t resume_offset;
	uint32_t kmin[pCnt], kmax[pCnt];
	double pmax[pCnt];
	int64_t total_hdrm;
	int64_t total_rsrv;

	// runtime
	int64_t shared_used_bytes;
	int64_t hdrm_bytes[pCnt][qCnt];
	int64_t ingress_bytes[pCnt][qCnt];
	uint32_t paused[pCnt][qCnt];
	int64_t egress_bytes[pCnt][qCnt];

	Ptr<Monitor> monitor;
};

} /* namespace ns3 */

#endif /* SWITCH_MMU_H */

