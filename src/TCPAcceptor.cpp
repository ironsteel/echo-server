#include "TCPAcceptor.h"
#include "StringUtils.h"

#include <iostream>
#include <charconv>
#include <array>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

TCPAcceptor::TCPAcceptor(const std::string& address)
: mAddress{address}
{
}

TCPAcceptor::~TCPAcceptor()
{
	if (mSocketFd > -1) {
		close(mSocketFd);
	}
}

int TCPAcceptor::listen() {
	const auto addrPortPair = StringUtils::split(mAddress, ':');
	if (addrPortPair.size() < 2) {
		std::cerr << "invalid address:port pair" << "\n";
		return -1;
	}

	int port = parsePort(addrPortPair[1]);
	if (port == -1) {
		return -1;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	int r = inet_pton(AF_INET, addrPortPair[0].data(), &addr.sin_addr);

	if (r == 0) {
		std::cerr << "invalid address format" << "\n";
		return -1;
	}

	if (r < 0) {
		perror("inet_pton");
		return -1;
	}

	mSocketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (mSocketFd == -1) {
		perror("socket");
		return -1;
	}


	int reuse = 1;
	if (setsockopt(mSocketFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
		perror("setsockopt(SO_REUSEADDR) failed");
	}

	if (setsockopt(mSocketFd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) {
		perror("setsockopt(SO_REUSEPORT) failed");
	}

	int bindRes = bind(mSocketFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if (bindRes < 0) {
		perror(("cannot bind to " + mAddress).data());
		return -1;
	}

	if (::listen(mSocketFd, SOMAXCONN) < 0) {
		perror("faied to listen");
		return -1;
	}

	std::cerr << "listening on: " << mAddress << "\n";
	return mSocketFd;
}

std::optional<ClientConnection> TCPAcceptor::accept() {
	sockaddr_in clientAddr;
	socklen_t clientAddrSize = sizeof(clientAddr);

	int sockClient = -1;
	if ((sockClient = ::accept(mSocketFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize)) < 0) {
		perror("failed to accept connection");
		return {};
	}

 	ClientAddrTuple addrPortPair = getClientAddress(clientAddr, clientAddrSize);

	if (addrPortPair.first.empty() || addrPortPair.second.empty()) {
		close(sockClient);
		std::cerr << "cannot retrieve client addr:port pair\n";
		return {};
	}

	ClientConnection connection{sockClient, addrPortPair.first, addrPortPair.second};
	return connection;
}

ClientAddrTuple TCPAcceptor::getClientAddress(const sockaddr_in& clientAddr, size_t size) {
	char host[NI_MAXHOST];
	char port[NI_MAXSERV];
	std::array<char, INET_ADDRSTRLEN> addr;

	if (getnameinfo(reinterpret_cast<const sockaddr*>(&clientAddr), size,
				host, NI_MAXHOST,
				port, NI_MAXSERV, 0) == 0) {
		return {host, port};
	}

	if (!inet_ntop(AF_INET, &clientAddr.sin_addr, addr.data(), addr.size())) {
		perror("inet_ntop: failed to convert addr");
		return {};
	}

	uint16_t intPort = ntohs(clientAddr.sin_port);
	return { std::string(addr.data(), addr.size()), std::to_string(intPort) };
}

int TCPAcceptor::parsePort(const std::string_view& strPort) {
	int port;
	std::from_chars_result res = std::from_chars(strPort.data(), strPort.data() + strPort.size(), port);
	if (res.ec == std::errc::invalid_argument) {
		std::cerr << "invalid port: " << strPort << "\n";
		return -1;
	}

	if (res.ec == std::errc::result_out_of_range || port > 65535) {
		std::cerr << "port out of range: " << strPort << "\n";
		return -1;
	}

	return port;
}
