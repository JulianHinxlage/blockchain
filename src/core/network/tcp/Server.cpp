//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include "Server.h"

namespace net {

	Server::Server() {
		listener = nullptr;
		thread = nullptr;
		running = false;
		readCallback = nullptr;
		disconnectCallback = nullptr;
		connectCallback = nullptr;
		errorCallback = nullptr;
		packetize = false;
		maxPacketSize = 1024 * 1024 * 16;
	}

	Server::Server(Server&& server) {
		listener = server.listener;
		thread = server.thread;
		running = server.running.load();
		connections = server.connections;
		readCallback = server.readCallback;
		disconnectCallback = server.disconnectCallback;
		connectCallback = server.connectCallback;
		errorCallback = server.errorCallback;

		server.listener = nullptr;
		server.thread = nullptr;
		server.connections.clear();
		server.readCallback = nullptr;
		server.disconnectCallback = nullptr;
		server.connectCallback = nullptr;
		server.errorCallback = nullptr;
	}

	Server::~Server() {
		close();
	}

	ErrorCode Server::listen(uint16_t port, bool prefereIpv4, bool reuseAddress, bool dualStacking) {
		if (!listener) {
			listener = std::make_shared<TcpSocket>();
		}
		ErrorCode error = listener->listen(port, prefereIpv4, reuseAddress, dualStacking);
		if (error != ErrorCode::NO_ERROR) {
			if (errorCallback) {
				errorCallback(nullptr, error);
			}
		}
		return error;
	}

	void Server::run() {
		if (running) {
			return;
		}

		running = true;
		thread = new std::thread([&]() {
			while (running) {
				auto socket = listener->accept();
				if (!socket) {
					break;
				}

				std::shared_ptr<Connection> conn = std::make_shared<Connection>();
				conn->socket = socket;
				conn->outbound = false;
				addConnection(conn);
			}
			running = false;
		});
	}

	bool Server::isRunning() {
		return running;
	}

	bool Server::hasAnyConnection() {
		for (auto& conn : connections) {
			if (conn && conn->socket) {
				if (conn->socket->isConnected()) {
					return true;
				}
			}
		}
		return false;
	}

	void Server::close(bool force) {
		if (listener) {
			listener->disconnect();
		}

		auto tmp = connections;
		connections.clear();
		tmp.clear();
		disconnectedConnections.clear();

		running = false;
		if (thread) {
			if (thread->joinable()) {
				if (force) {
					thread->detach();
				}
				else {
					thread->join();
				}
			}
			delete thread;
			thread = nullptr;
		}
	}

	void Server::waitWhileRunning() {
		if (thread) {
			thread->join();
		}
	}

	ErrorCode Server::connectAsClient(const Endpoint& endpoint) {
		std::shared_ptr<Connection> conn = std::make_shared<Connection>();
		conn->errorCallback = errorCallback;
		ErrorCode error = conn->connect(endpoint);
		if (error == ErrorCode::NO_ERROR) {
			addConnection(conn);
		}
		return error;
	}

	ErrorCode Server::connectAsClient(const std::string& address, uint16_t port, bool resolve, bool prefereIpv4) {
		std::shared_ptr<Connection> conn = std::make_shared<Connection>();
		conn->errorCallback = errorCallback;
		ErrorCode error = conn->connect(address, port, resolve, prefereIpv4);
		if (error == ErrorCode::NO_ERROR) {
			addConnection(conn);
		}
		return error;
	}

	void Server::addConnection(std::shared_ptr<net::Connection> conn) {
		disconnectedConnections.clear();

		conn->packetize = packetize;
		conn->maxPacketSize = maxPacketSize;
		conn->readCallback = readCallback;
		conn->errorCallback = errorCallback;
		conn->disconnectCallback = [&](Connection* conn) {
			if (disconnectCallback) {
				disconnectCallback(conn);
			}
			for (int i = 0; i < connections.size(); i++) {
				if (connections[i].get() == conn) {
					disconnectedConnections.push_back(connections[i]);
					connections.erase(connections.begin() + i);
					break;
				}
			}
		};

		connections.push_back(conn);
		if (connectCallback) {
			connectCallback(conn.get());
		}

		conn->run();
	}

}
