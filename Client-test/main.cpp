#include <NetworkModel/EventSelect/EventSelect.hpp>
#include "../Protocol/PacketProtocol.h"
#include "../Protocol/MessageProtocol.h"
#include "../Packet/AccountInfo.h"

using namespace SERVER::NETWORKMODEL::EVENTSELECT;
using namespace SERVER::NETWORKMODEL::BASEMODEL;
using namespace SERVER::FUNCTIONS::SOCKETADDRESS;
using namespace SERVER::NETWORK::PROTOCOL::UTIL;

class CEventSelect : public EventSelect {
public:
	CEventSelect(const int iPacketProcessorLoopCount, PACKETPROCESSOR& packetProcessorMap) : EventSelect(iPacketProcessorLoopCount, packetProcessorMap) {
		packetProcessorMap.insert(std::make_pair(static_cast<uint8_t>(EPacketProtocol::E_Account), std::bind(&CEventSelect::RecvPacketProcess, this, std::placeholders::_1)));
	}

private:
	void RecvPacketProcess(SERVER::NETWORK::PACKET::PacketQueueData* const pPacket) {
		AccountInfo accountInfo;
		SERVER::NETWORK::PACKET::UTIL::Deserialize(pPacket->m_packetData, accountInfo);

		switch (static_cast<EAccountMeesageProtocol>(accountInfo.m_iMessageType)) {
		case EAccountMeesageProtocol::E_SignUpResult:
			std::cout << "Signup Succ!\n";
			break;
		case EAccountMeesageProtocol::E_SignInResult:
			std::cout << "Signin Succ!\n";
			break;
		case EAccountMeesageProtocol::E_InfoInquiryResult:
			std::cout << "InfoInquiry Succ!\n";
			break;
		default:

			break;
		}
	}


};

int main() {
	PACKETPROCESSOR packetProcessor;
	CEventSelect eventSelect(1, packetProcessor);
	SocketAddress serverAddress("127.0.0.1", 3550);

	eventSelect.Initialize(EPROTOCOLTYPE::EPT_BOTH, serverAddress);
	
	AccountInfo accountInfoPacket(static_cast<uint8_t>(EPacketProtocol::E_Account), static_cast<uint8_t>(EAccountMeesageProtocol::E_TrySignUp), 0, "admin", "admin123", "Admin:PPODO");
	auto pCating = reinterpret_cast<SERVER::NETWORK::PACKET::BasePacket*>(&accountInfoPacket);
	auto result = SERVER::NETWORK::PACKET::UTIL::Serialize<AccountInfo>(*pCating);

	eventSelect.Send(result);
	while (true) {
		eventSelect.Run();
	}
	eventSelect.Destroy();
	return 0;
}