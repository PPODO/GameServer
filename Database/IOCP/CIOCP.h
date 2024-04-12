#pragma once
#include <NetworkModel/IOCP/IOCP.hpp>
#include <Functions/CircularQueue/CircularQueue.hpp>
#include "Database/SQLProcessor/SQLProcessor.h"
#include <atomic>

using namespace SERVER::FUNCTIONS::CIRCULARQUEUE;
using namespace SERVER::FUNCTIONS::SOCKETADDRESS;

using namespace SERVER::NETWORK::PACKET;

using namespace SERVER::NETWORKMODEL::IOCP;
using namespace SERVER::NETWORKMODEL::BASEMODEL;

struct TransmitQueueData : QUEUEDATA::BaseData<TransmitQueueData, 400> {
public:
	CONNECTION* m_pClientConnection;
	BasePacket* m_pPacket;

public:
	TransmitQueueData() : m_pClientConnection(nullptr), m_pPacket(nullptr) {};
	TransmitQueueData(CONNECTION* pConnectedUser) : m_pClientConnection(pConnectedUser), m_pPacket(nullptr) {};
	TransmitQueueData(CONNECTION* pConnectedUser, BasePacket* pBasePacket) : m_pClientConnection(pConnectedUser), m_pPacket(pBasePacket) {};

};

class CSQLIOCP : public IOCP {
	typedef CircularQueue<TransmitQueueData*> TRANSMISSION_QUEUE;
public:
	CSQLIOCP(const std::string& sHostName, const std::string& sUserName, const std::string& sPassword, PACKETPROCESSOR& packetProcessorMap, const std::chrono::milliseconds& transmitThreadDuration = std::chrono::milliseconds(250), const std::chrono::milliseconds& sqlProcessThreadDuration = std::chrono::milliseconds(250));

public:
	virtual void Run() override;
	virtual void Destroy() override;

	void AddNewTransmitData(TransmitQueueData* pNewTransmitData);

private:
	void AccountPacketProcess(PacketQueueData* const pPacketData);

private:
	void PacketTransmission();
	const SERVER::NETWORK::PACKET::PACKET_STRUCT& SerializePacket(SERVER::NETWORK::PACKET::BasePacket* const pBasePacket);

private:
	const std::chrono::milliseconds m_transmitThreadDuration;

	SQLProcessor m_sqlProcessor;

	std::atomic_bool m_bTransmissionThreadRunState;
	std::thread m_packetTransmissionThread;

	std::unique_ptr<TRANSMISSION_QUEUE> m_pPacketTransmissionStockQueue;
	std::unique_ptr<TRANSMISSION_QUEUE> m_pPacketTransmissionProcessQueue;
	std::mutex m_transmitQueueMutex;
	std::condition_variable m_cvTransmitQueue;

};