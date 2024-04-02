//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Connection.h"

namespace net {

	class Server {
	public:
		std::vector<std::shared_ptr<net::Connection>> connections;
		bool packetize;
		int maxPacketSize;

		std::function<void(Connection*, Buffer&)> readCallback;
		std::function<void(Connection*)> disconnectCallback;
		std::function<void(Connection*)> connectCallback;
		std::function<void(Connection*, ErrorCode)> errorCallback;

		Server();
		Server(Server&& server);
		~Server();

		ErrorCode listen(uint16_t port, bool prefereIpv4 = false, bool reuseAddress = false, bool dualStacking = false);
		void run();
		bool isRunning();
		bool hasAnyConnection();
		void close(bool force = false);
		void waitWhileRunning();

		ErrorCode connectAsClient(const Endpoint& endpoint);
		ErrorCode connectAsClient(const std::string& address, uint16_t port, bool resolve = true, bool prefereIpv4 = false);

	private:
		std::shared_ptr<TcpSocket> listener;
		std::thread* thread;
		std::atomic_bool running;
		std::vector<std::shared_ptr<net::Connection>> disconnectedConnections;

		void addConnection(std::shared_ptr<net::Connection> conn);
	};

}
