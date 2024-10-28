#pragma once

#include <memory>
#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <unordered_map>

class ConnectionAcceptor;

using MessageCount = unsigned long long;

class EchoServer {
	struct ConnectionInfo {
		int clientSocket;
		int shutDownPipe[2];
		std::string addr;
	};
public:
	explicit EchoServer(std::unique_ptr<ConnectionAcceptor>&& acceptor)
		: mAcceptor(acceptor){};
	~EchoServer() {};

	int serve();
private:
	int handle(ConnectionInfo conninfo);
	int setupPipe(int pipe[2]);

	int handleStatsCommand(const ConnectionInfo& connInfo);

	void addClient(const ConnectionInfo& connInfo);
	void removeClient(const ConnectionInfo& connInfo);
	void incrementMessagesForClient(const ConnectionInfo& connInfo);
private:
	std::unique_ptr<ConnectionAcceptor>& mAcceptor;
	std::vector<std::thread> mThreads;
	std::vector<ConnectionInfo> mConnInfos;

	std::unordered_map<std::string, MessageCount> mClients;
	std::mutex mClientsMutex;
};
