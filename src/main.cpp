#include <iostream>
#include "EchoServer.h"
#include "TCPAcceptor.h"

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "addr:port argument required\n";
		return EXIT_FAILURE;
	}

	std::unique_ptr<ConnectionAcceptor> tcpAcceptor = std::make_unique<TCPAcceptor>(argv[1]);
	EchoServer echoServer{ std::move(tcpAcceptor) };
	echoServer.serve();
	return EXIT_SUCCESS;
}
