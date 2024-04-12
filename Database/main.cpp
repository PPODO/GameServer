#include "IOCP/CIOCP.h"

int main() {
	PACKETPROCESSOR packetProcessorMap;
	CSQLIOCP iocp("tcp://localhost:3306", "root", "a2233212.", packetProcessorMap);
	SocketAddress bindAddress("127.0.0.1", 3550);

	iocp.Initialize(SERVER::NETWORK::PROTOCOL::UTIL::BSD_SOCKET::EPROTOCOLTYPE::EPT_TCP, bindAddress);
	while (true)
		iocp.Run();

	iocp.Destroy();
	return 0;
}