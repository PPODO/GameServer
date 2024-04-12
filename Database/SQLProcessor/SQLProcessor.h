#pragma once
#define _WINSOCKAPI_
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <Functions/CircularQueue/CircularQueue.hpp>
#include <Functions/MySQL/MySQL.hpp>
#include <NetworkModel/IOCP/IOCP.hpp>
#include "Packet/AccountInfo.h"
#include "Util/Timer.h"
#include "Util/ThreadPool.h"

using namespace SERVER::FUNCTIONS::CIRCULARQUEUE;
using namespace SERVER::FUNCTIONS::MYSQL;

using namespace SERVER::NETWORKMODEL::IOCP;

struct SQLPreparedStatementData : public QUEUEDATA::BaseData<SQLPreparedStatementData, 400> {
public:
	CONNECTION* const m_pRequestConnection;

	uint8_t m_iSQLType; // packet protocol
	uint8_t m_iQueryType; // message protocol

	void* m_pSQLData;

public:
	SQLPreparedStatementData() : m_pRequestConnection(nullptr), m_iSQLType(0), m_iQueryType(0), m_pSQLData(nullptr) {};
	SQLPreparedStatementData(CONNECTION* const pRequestConnection, const uint8_t iSQLType, const uint8_t iQueryType, void* pSQLData) : m_pRequestConnection(pRequestConnection), m_iSQLType(iSQLType), m_iQueryType(iQueryType), m_pSQLData(pSQLData) {};

};

struct SQLPreparedStatementResultData : public QUEUEDATA::BaseData<SQLPreparedStatementResultData, 400> {
public:
	sql::PreparedStatement* m_pPrepareStatement;
	SQLPreparedStatementData* m_pSQLPrepareData;

public:
	SQLPreparedStatementResultData() : m_pPrepareStatement(nullptr), m_pSQLPrepareData(nullptr) {};
	SQLPreparedStatementResultData(sql::PreparedStatement* const pPrepareStatement, SQLPreparedStatementData* const pSQLData) : m_pPrepareStatement(pPrepareStatement), m_pSQLPrepareData(pSQLData) {};

};

class SQLProcessor {
	typedef CircularQueue<SQLPreparedStatementData*> SQL_QUEUE;
public:
	SQLProcessor(const std::string& sHostName, const std::string& sUserName, const std::string& sPassword, class CSQLIOCP* const pIOCPServerInstance, const uint16_t iWorkerThreadCount, const std::chrono::milliseconds& sqlProcessThreadDuration = std::chrono::milliseconds(250));
	~SQLProcessor();

public:
	void AddNewSQLData(SQLPreparedStatementData* const pSQLPrepareStatementData);

private:
	void* PrepareStatement(SQLPreparedStatementData* pSQLPrepareStatementData);
	void ExecuteQuery(void* pSQLPrepareStatementResult);

private:
	class CSQLIOCP* const m_pIOCPServerInstance;

	CMySQLPool m_sqlConnectionPool;

	ThreadPool m_sqlThreadPool;

};