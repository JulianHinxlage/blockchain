//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "TcpSocket.h"
#include "Packet.h"
#include <functional>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace net {

	class Connection {
	public:
		std::vector<uint8_t> buffer;
		std::shared_ptr<TcpSocket> socket;
		std::shared_ptr<std::thread> thread;
		std::mutex reconnectMutex;
		std::mutex writeMutex;
		std::condition_variable reconnect;
		bool running;
		std::function<void(Connection* conn)> onDisconnect;
		std::function<void(Connection* conn)> onConnect;
		std::function<void(Connection* conn)> onFail;

		Connection();
		~Connection();
		void run(const std::function<void(Connection* conn, void* data, int bytes)>& callback);
		void runConnect(const std::string &address, uint16_t port, const std::function<void(Connection* conn, void* data, int bytes)>& callback);
		bool runListen(uint16_t port, const std::string &ip, const std::function<void(std::shared_ptr<Connection> conn)>& callback);
		void stop();

		bool write(const void* data, int bytes);
		bool write(Packet &packet);
	private:
		void runImpl(const std::function<void(Connection* conn, void* data, int bytes)>& callback);
	};

}
