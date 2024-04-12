#include "SQLProcessor.h"
#include "Database/SQLProcessor/Query/Account/AccountQuery.h"
#include "Protocol/PacketProtocol.h"
#include "Protocol/MessageProtocol.h"
#include "Database/IOCP/CIOCP.h"

using namespace SERVER::FUNCTIONS::LOG;

SQLProcessor::SQLProcessor(const std::string& sHostName, const std::string& sUserName, const std::string& sPassword, CSQLIOCP* const pIOCPServerInstance, const uint16_t iWorkerThreadCount, const std::chrono::milliseconds& sqlProcessThreadDuration) 
	: m_pIOCPServerInstance(pIOCPServerInstance),
	  m_sqlConnectionPool(sHostName, sUserName, sPassword, iWorkerThreadCount * 2),
	  m_sqlThreadPool(std::bind(&SQLProcessor::ExecuteQuery, this, std::placeholders::_1), iWorkerThreadCount, sqlProcessThreadDuration) {
}

SQLProcessor::~SQLProcessor() {
}

void* SQLProcessor::PrepareStatement(SQLPreparedStatementData* pSQLPrepareStatementData) {
	SQLPreparedStatementResultData* pStatementResult = nullptr;
	auto pConnection = m_sqlConnectionPool.GetConnection("mf");
	
	if (pSQLPrepareStatementData && pConnection) {
		switch (static_cast<EPacketProtocol>(pSQLPrepareStatementData->m_iSQLType)) {
		case EPacketProtocol::E_Account:
			 return new SQLPreparedStatementResultData(db::query::account::MakePreparedStatementByMessageType(pSQLPrepareStatementData->m_iQueryType, 
													   pSQLPrepareStatementData->m_pSQLData, pConnection.get()), pSQLPrepareStatementData);



		case EPacketProtocol::E_None:
		default:
			return pStatementResult;
		}
	}
	return pStatementResult;
}

void SQLProcessor::ExecuteQuery(void* pSQLPrepareStatementResult) {
	if (!m_pIOCPServerInstance) {
		Log::WriteLog(L"SQL Processor - Invalid IOCP instance!");
		return;
	}

	if (auto pCachedPrepareStatementResult = reinterpret_cast<SQLPreparedStatementResultData*>(pSQLPrepareStatementResult)) {
		TransmitQueueData* pNewTransmitData = new TransmitQueueData(pCachedPrepareStatementResult->m_pSQLPrepareData->m_pRequestConnection);
		BasePacket* pQueryResultPacket = nullptr;

		switch (static_cast<EPacketProtocol>(pCachedPrepareStatementResult->m_pSQLPrepareData->m_iSQLType)) {
		case EPacketProtocol::E_Account:
			pQueryResultPacket = db::query::account::ExecuteQuery(pCachedPrepareStatementResult->m_pSQLPrepareData->m_iQueryType, 
																  pCachedPrepareStatementResult->m_pSQLPrepareData->m_pSQLData, pCachedPrepareStatementResult->m_pPrepareStatement);
			break;
		case EPacketProtocol::E_None:
		default:
			// error
			break;
		}
		delete pCachedPrepareStatementResult->m_pSQLPrepareData;
		delete pCachedPrepareStatementResult;

		pNewTransmitData->m_pPacket = pQueryResultPacket;

		if (pQueryResultPacket)
			m_pIOCPServerInstance->AddNewTransmitData(pNewTransmitData);
	}
}

void SQLProcessor::AddNewSQLData(SQLPreparedStatementData* const pSQLPrepareStatementData) {
	m_sqlThreadPool.EnqueueJob(std::bind(&SQLProcessor::PrepareStatement, this, std::placeholders::_1), pSQLPrepareStatementData);
}