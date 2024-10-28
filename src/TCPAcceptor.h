#pragma once
#include "ConnectionAcceptor.h"

#include <string>
#include <string_view>
#include <utility>

using ClientAddrTuple = std::pair<std::string, std::string>;

struct sockaddr_in;

class TCPAcceptor : public ConnectionAcceptor {
public:
	TCPAcceptor(const std::string& address);
	virtual ~TCPAcceptor();
	virtual int listen() override;
	virtual std::optional<ClientConnection> accept() override;

private:
	int parsePort(const std::string_view& strPort);
	ClientAddrTuple getClientAddress(const sockaddr_in& clientAddr, size_t size); 

private:
	std::string mAddress;
	int mSocketFd = -1;
};
