#pragma once
#include <Network/Packet/BasePacket.hpp>
#include "Protocol/PacketProtocol.h"
#include "Protocol/MessageProtocol.h"
#include "Protocol/SQLCode.h"
#include "Database/SQLDataFormat/user_info.h"
#include "Packet/AccountInfo.h"
#include <functional>
#include <string>

using namespace SERVER::FUNCTIONS::MYSQL::SQL;
// TODO : all query must return error code (int)
namespace db {
	namespace query {
		namespace account {
			namespace func {
				static sql::PreparedStatement* SignUpQuery(sql::Connection* const pDBConnection, CUSER_INFO* const pUserInfo) {
					std::hash<std::string> str_to_hash;
					pUserInfo->m_token.m_rawData = str_to_hash(pUserInfo->m_user_id.m_rawData);

					return CUSER_INFO::PreparedQueryForInsert(pDBConnection);
				}

				static sql::PreparedStatement* SignInQuery(sql::Connection* const pDBConnection, CUSER_INFO* const pUserInfo) {
					std::hash<std::string> str_to_hash;
					int32_t iToken = str_to_hash(pUserInfo->m_user_id.m_rawData);
					return CUSER_INFO::PreparedQueryForSelect(pDBConnection, CQueryWhereConditional("token", std::to_string(iToken)));
				}


				static SERVER::NETWORK::PACKET::BasePacket* SignUpQueryResult(sql::PreparedStatement* const pPreparedStatement, CUSER_INFO* const pUserInfo) {
					const uint8_t iAccountMessageProtocol = static_cast<uint8_t>(EAccountMeesageProtocol::E_SignUpResult);

					AccountInfo* pRet = new AccountInfo(static_cast<uint8_t>(EPacketProtocol::E_Account), 0);
					int iErrorCode = CUSER_INFO::ExecuteQueryForInsert(pPreparedStatement, *pUserInfo);
					uint8_t iQueryResult = 0, iFailedReason = 0;

					if (!pRet) {
						SERVER::FUNCTIONS::LOG::Log::WriteLog(TEXT("cannot alloc memory!"));
						return nullptr;
					}

					switch (iErrorCode) {
					case ESQLErrorCode::E_None:
						iQueryResult = static_cast<uint8_t>(EMessageResultProtocol::E_Succeeded);
						iFailedReason = static_cast<uint8_t>(EMessageFailedReason::E_None);

						pRet->m_token = pUserInfo->m_token.m_rawData;
						pRet->m_user_id = pUserInfo->m_user_id.m_rawData;
						pRet->m_user_password = pUserInfo->m_uesr_pw.m_rawData;
						pRet->m_user_name = pUserInfo->m_user_name.m_rawData;
						break;
					case ESQLErrorCode::E_DuplicateKey:
						iQueryResult = static_cast<uint8_t>(EMessageResultProtocol::E_Failed);
						iFailedReason = static_cast<uint8_t>(EMessageFailedReason::E_ExistUserInfo);
						break;
					default:
						iQueryResult = static_cast<uint8_t>(EMessageResultProtocol::E_Failed);
						iFailedReason = static_cast<uint8_t>(EMessageFailedReason::E_ServerError);
						break;
					}

					pRet->m_iMessageType = util::MakeMessageProtocol(iAccountMessageProtocol, iQueryResult, iFailedReason);

					return pRet;
				}

				static SERVER::NETWORK::PACKET::BasePacket* SignInQueryResult(sql::PreparedStatement* const pPreparedStatement, CUSER_INFO* const pUserInfo) {
					std::vector<CUSER_INFO> output;
					AccountInfo* pRet = new AccountInfo(static_cast<uint8_t>(EPacketProtocol::E_Account), 0);

					if (CUSER_INFO::ExecuteQueryForSelect(pPreparedStatement, output)) {
						if (output.size() > 0) {
							const auto& outputData = output.front();

							pRet->m_token = outputData.m_token.m_rawData;
							pRet->m_user_id = outputData.m_user_id.m_rawData;
							pRet->m_user_password = outputData.m_uesr_pw.m_rawData;
							pRet->m_user_name = outputData.m_user_name.m_rawData;

							pRet->m_iMessageType = util::MakeMessageProtocol(static_cast<uint8_t>(EAccountMeesageProtocol::E_SignInResult), static_cast<uint8_t>(EMessageResultProtocol::E_Succeeded), static_cast<uint8_t>(EMessageFailedReason::E_None));
						}
						else
							pRet->m_iMessageType = util::MakeMessageProtocol(static_cast<uint8_t>(EAccountMeesageProtocol::E_SignInResult), static_cast<uint8_t>(EMessageResultProtocol::E_Failed), static_cast<uint8_t>(EMessageFailedReason::E_WrongUserInfo));
					}
					else
						pRet->m_iMessageType = util::MakeMessageProtocol(static_cast<uint8_t>(EAccountMeesageProtocol::E_SignInResult), static_cast<uint8_t>(EMessageResultProtocol::E_Failed), static_cast<uint8_t>(EMessageFailedReason::E_ServerError));

					return pRet;
				}
			}


			static sql::PreparedStatement* MakePreparedStatementByMessageType(const uint32_t iMessageProtocol, void* pSQLData, sql::Connection* pDBConnection) {
				if (!pDBConnection) return nullptr;

				if (auto pSQLUserData = reinterpret_cast<CUSER_INFO*>(pSQLData)) {
					switch (static_cast<EAccountMeesageProtocol>(iMessageProtocol)) {
					case EAccountMeesageProtocol::E_TrySignUp:
						return func::SignUpQuery(pDBConnection, pSQLUserData);
					case EAccountMeesageProtocol::E_TrySignIn:
						return func::SignInQuery(pDBConnection, pSQLUserData);
						break;
					case EAccountMeesageProtocol::E_TryInfoInquiry:

						break;
					}
				}
				return nullptr;
			}

			static SERVER::NETWORK::PACKET::BasePacket* ExecuteQuery(const uint32_t iMessageProtocol, void* pSQLData, sql::PreparedStatement* const pPreparedStatement) {
				auto pSQLUserData = reinterpret_cast<CUSER_INFO*>(pSQLData);
				SERVER::NETWORK::PACKET::BasePacket* pResultPacket = nullptr;

				if (!pSQLUserData) return nullptr;

				if (pPreparedStatement) {
					switch (static_cast<EAccountMeesageProtocol>(iMessageProtocol)) {
					case EAccountMeesageProtocol::E_TrySignUp:
						pResultPacket = func::SignUpQueryResult(pPreparedStatement, pSQLUserData);
						break;
					case EAccountMeesageProtocol::E_TrySignIn:
						pResultPacket = func::SignInQueryResult(pPreparedStatement, pSQLUserData);
						break;
					case EAccountMeesageProtocol::E_TryInfoInquiry:

						break;
					}
				}

				delete pSQLUserData;
				return pResultPacket;
			}
		}
	}
}