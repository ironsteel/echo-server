#include "EchoServer.h"
#include "ConnectionAcceptor.h"

#include <iostream>
#include <array>
#include <string_view>
#include <sstream>
#include <cstring>

#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <fcntl.h>


static int selfPipe[2] = {0};

const char* statsCommand = "stats\n";

void signalHandler(int) {
	uint8_t empty[1] = {0};
	int writePipe = selfPipe[1];
	write(writePipe, empty, 1);
}

int EchoServer::serve() {
	struct sigaction a;
	a.sa_handler = signalHandler;
	a.sa_flags = 0;
	sigemptyset(&a.sa_mask);
	sigaction(SIGINT, &a, NULL);

	int serverSocket = mAcceptor->listen();
	if (serverSocket == -1) {
		return -1;
	}

	pollfd pollfds[2];
	pollfds[0].fd = serverSocket;
	pollfds[0].events = POLLIN | POLLPRI;

	if (setupPipe(selfPipe) == -1) {
		std::cerr << "failed to setup self pipe\n";
		return -1;
	}


	pollfds[1].fd = selfPipe[0];
	pollfds[1].events = POLLIN | POLLPRI;

	while (true) {
		int pollResult = poll(pollfds, 2, -1);
		if (pollResult < 0) {
			if (errno == EINTR) {
				continue;
			}
			perror("poll_failed");
			break;
		}
		if (pollResult == 0) {
			continue;
		}
		if (pollfds[0].revents & POLLIN) {
			std::optional<ClientConnection> clientConn = mAcceptor->accept();
			if (!clientConn) {
				continue;
			}

			ConnectionInfo info;
			info.clientSocket = clientConn->socket;
			info.addr = clientConn->addr + ":" + clientConn->port;

			if (setupPipe(info.shutDownPipe) == -1) {
				close(clientConn->socket);
				continue;
			}

			std::cerr << "accepted connection from " << clientConn->addr << ":" << clientConn->port << "\n";
			addClient(info);

			mConnInfos.push_back(info);
			std::thread t(&EchoServer::handle, this, info);
			mThreads.push_back(std::move(t));
		}

		// Self pipe was written to, shut down the server loop
		if (pollfds[1].revents & POLLIN) {
			break;
		}
	}

	// Tell the connection handling threads to shutdown/exit
	for (const auto& info : mConnInfos) {
		uint8_t empty[1] = {0};
		int writePipe = info.shutDownPipe[1];
		write(writePipe, empty, 1);
	}

	std::cerr << "terminating" << "\n";

	for (auto& t : mThreads) {
		if (t.joinable()) {
			t.join();
		}
	}
	return 0;
}

int EchoServer::setupPipe(int p[2]) {
	if (pipe(p) == -1) {
		perror("pipe");
		return -1;
	}

	if (fcntl(p[0], F_SETFD, O_NONBLOCK) == -1) {
		perror("fcntl");
		return -1;
	}

	if (fcntl(p[1], F_SETFD, O_NONBLOCK) == -1) {
		perror("fcntl");
		return -1;
	}
	return 0;
}

int EchoServer::handle(ConnectionInfo connInfo) {
	std::array<char, 4096> buf;
	int bytes;
	pollfd pollfds[2];
	pollfds[0].fd = connInfo.clientSocket;
	pollfds[0].events = POLLIN | POLLPRI;

	pollfds[1].fd = connInfo.shutDownPipe[0];
	pollfds[1].events = POLLIN | POLLPRI;
	while (true) {
		int pollResult = poll(pollfds, 2, -1);
		if (pollResult < 0) {
			perror("poll_failed");
			break;
		}
		if (pollResult == 0) {
			continue;
		}
		if (pollfds[0].revents & POLLIN) {
			bytes = recv(connInfo.clientSocket, buf.data(), buf.size(), 0);
			if (bytes == 0) {
				break;
			}

			if (bytes < 0) {
				perror("recv failed");
				break;
			}

			if (static_cast<size_t>(bytes) == strlen(statsCommand)) {
				if (strncmp(buf.data(), statsCommand, strlen(statsCommand) - 1) == 0) {
					if (handleStatsCommand(connInfo) == -1) {
						perror("write() failed");
						break;
					}
				}
			} else {
				if (write(connInfo.clientSocket, buf.data(), bytes) == -1) {
					perror("write() failed");
					break;
				}
			}
			if (buf[bytes - 1] == '\n') {
				incrementMessagesForClient(connInfo);
			}
		}
		if (pollfds[1].revents & POLLIN) {
			break;
		}
	}

	removeClient(connInfo);
	close(connInfo.clientSocket);
	return 0;
}

int EchoServer::handleStatsCommand(const ConnectionInfo& connInfo) {
	std::unique_lock l(mClientsMutex);
	std::stringstream stats;
	for (const auto& info : mClients) {
		stats << info.first << ":" << info.second << "\n";
	}
	l.unlock();
	std::string str = stats.str();
	return write(connInfo.clientSocket, str.data(), str.size());
}

void EchoServer::addClient(const ConnectionInfo& connInfo) {
	std::scoped_lock lock{mClientsMutex};
	mClients[connInfo.addr] = 0;
}

void EchoServer::removeClient(const ConnectionInfo& connInfo) {
	std::scoped_lock lock{mClientsMutex};
	mClients.erase(connInfo.addr);
}

void EchoServer::incrementMessagesForClient(const ConnectionInfo& connInfo) {
	std::scoped_lock lock{mClientsMutex};
	mClients[connInfo.addr] = ++mClients[connInfo.addr];
}
