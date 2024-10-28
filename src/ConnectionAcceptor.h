#pragma once

#include <string>
#include <optional>

struct ClientConnection {
	int socket;
	std::string addr;
	std::string port;
};

class ConnectionAcceptor {
public:
	virtual int listen() = 0;
	virtual std::optional<ClientConnection> accept() = 0;
	virtual ~ConnectionAcceptor() {};
};
