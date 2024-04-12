#include "CIOCP.h"
#include <Network/UserSession/Server/User_Server.hpp>
#include "Database/SQLDataFormat/user_info.h"
#include "Protocol/PacketProtocol.h"
#include "Packet/AccountInfo.h"

using namespace SERVER::NETWORK::USER_SESSION::USER_SERVER;
using namespace SERVER::FUNCTIONS::CRITICALSECTION;

CSQLIOCP::CSQLIOCP(const std::string& sHostName, const std::string& sUserName, const std::string& sPassword, PACKETPROCESSOR& packetProcessorMap, const std::chrono::milliseconds& transmitThreadDuration, const std::chrono::milliseconds& sqlProcessThreadDuration) 
	: IOCP(packetProcessorMap, 8), m_transmitThreadDuration(transmitThreadDuration),
	m_sqlProcessor(sHostName, sUserName, sPassword, this, 24), m_bTransmissionThreadRunState(true) {

	packetProcessorMap.insert(std::make_pair(static_cast<uint16_t>(EPacketProtocol::E_Account), 
							  std::bind(&CSQLIOCP::AccountPacketProcess, this, std::placeholders::_1)));

	m_pPacketTransmissionStockQueue = std::make_unique<TRANSMISSION_QUEUE>();
	m_pPacketTransmissionProcessQueue = std::make_unique<TRANSMISSION_QUEUE>();

	m_packetTransmissionThread = std::thread(std::bind(&CSQLIOCP::PacketTransmission, this));
}

void CSQLIOCP::Run() {
	IOCP::Run();

	if (!m_pPacketTransmissionStockQueue->IsEmpty() && m_pPacketTransmissionProcessQueue->IsEmpty()) {
		std::unique_lock<std::mutex> lck(m_transmitQueueMutex);
		m_pPacketTransmissionStockQueue->EnableCriticalSection(true);
		m_pPacketTransmissionStockQueue.swap(m_pPacketTransmissionProcessQueue);
		m_pPacketTransmissionStockQueue->EnableCriticalSection(false);
		m_cvTransmitQueue.notify_all();
	}
}

void CSQLIOCP::Destroy() {
	IOCP::Destroy();

	m_bTransmissionThreadRunState = false;
	m_packetTransmissionThread.join();
}

void CSQLIOCP::AccountPacketProcess(PacketQueueData* const pPacketData) {
	AccountInfo* accountInfoPacket = new AccountInfo();
	UTIL::Deserialize(pPacketData->m_packetData, *accountInfoPacket);

	if (auto pCachedClientInfo = reinterpret_cast<SERVER::NETWORKMODEL::IOCP::CONNECTION*>(pPacketData->m_pOwner))
		m_sqlProcessor.AddNewSQLData(new SQLPreparedStatementData(pCachedClientInfo, pPacketData->m_packetData.m_packetInfo.m_iPacketType, accountInfoPacket->m_iMessageType, 
									 reinterpret_cast<void*>(new CUSER_INFO(accountInfoPacket->m_token, accountInfoPacket->m_user_id, accountInfoPacket->m_user_password, accountInfoPacket->m_user_name))));
}

void CSQLIOCP::PacketTransmission() {
	while (true) {
		std::unique_lock<std::mutex> lck(m_transmitQueueMutex);
		if (m_cvTransmitQueue.wait_for(lck, m_transmitThreadDuration, [this]() { return !m_pPacketTransmissionProcessQueue->IsEmpty(); })) {

			if (!m_bTransmissionThreadRunState)
				break;

			TransmitQueueData* pQueueData = nullptr;
			m_pPacketTransmissionProcessQueue->Pop(pQueueData);
			lck.unlock();

			if (pQueueData) {
				pQueueData->m_pClientConnection->m_pUser->Send(SerializePacket(pQueueData->m_pPacket));

				delete pQueueData;
			}
		}
	}
}

void CSQLIOCP::AddNewTransmitData(TransmitQueueData* pNewTransmitData) {
	if (pNewTransmitData) {
		std::unique_lock<std::mutex> lck(m_transmitQueueMutex);
		m_pPacketTransmissionStockQueue->Push(pNewTransmitData);
	}
}

const SERVER::NETWORK::PACKET::PACKET_STRUCT& CSQLIOCP::SerializePacket(SERVER::NETWORK::PACKET::BasePacket* const pBasePacket) {
	SERVER::NETWORK::PACKET::PACKET_STRUCT returnValue;

	switch (static_cast<EPacketProtocol>(pBasePacket->m_iPacketType)) {
	case EPacketProtocol::E_Account:
	{
		auto pAccountPacket = reinterpret_cast<AccountInfo*>(pBasePacket);
		returnValue = UTIL::Serialize<AccountInfo>(*pAccountPacket);
		delete pAccountPacket;
	}
		break;
	}

	return returnValue;
}