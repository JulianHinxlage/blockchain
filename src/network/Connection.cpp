//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Connection.h"

namespace net {

	Connection::Connection() {
		thread = nullptr;
		socket = std::make_shared<TcpSocket>();
		onDisconnect = nullptr;
		onConnect = nullptr;
		onFail = nullptr;
	}

	Connection::~Connection() {
		stop();
	}

	void Connection::run(const std::function<void(Connection* conn, void* data, int bytes)>& callback) {
		if (thread) {
			thread->detach();
			thread = nullptr;
		}
		thread = std::make_shared<std::thread>([&, callback]() {
			runImpl(callback);
			if (onDisconnect) {
				onDisconnect(this);
			}
		});
	}

	void Connection::runConnect(const std::string& address, uint16_t port, const std::function<void(Connection* conn, void* data, int bytes)>& callback) {
		if (thread) {
			thread->detach();
			thread = nullptr;
		}
		running = true;

		if (socket->connect(address, port)) {
			if (onConnect) {
				onConnect(this);
			}
		}
		else {
			if (onFail) {
				onFail(this);
			}
			return;
		}

		thread = std::make_shared<std::thread>([&, callback]() {
			runImpl(callback);
			if (onDisconnect) {
				onDisconnect(this);
			}
		});
	}

	bool Connection::runListen(uint16_t port, const std::string &ip, const std::function<void(std::shared_ptr<Connection>conn)>& callback) {
		if (thread) {
			thread->detach();
			thread = nullptr;
		}
		
		Endpoint ep;
		ep.set(ip, port);

		if (socket->listen(port, ep.isIpv4())) {
			if (onConnect) {
				onConnect(this);
			}
		}
		else{
			if (onFail) {
				onFail(this);
			}
			return false;
		}

		running = true;
		thread = std::make_shared<std::thread>([&, port, callback]() {
			
			while (running && socket->isConnected()) {
				std::shared_ptr<TcpSocket> newSocket = socket->accept();
				if (newSocket) {
					std::shared_ptr<Connection> conn = std::make_shared<Connection>();
					conn->socket = newSocket;
					callback(conn);
				}
			}
			if (onDisconnect) {
				onDisconnect(this);
			}
		});
		return true;
	}

	void Connection::stop() {
		running = false;
		reconnect.notify_all();
		socket->disconnect();
		if (thread) {
			thread->detach();
			thread = nullptr;
		}
	}

	bool Connection::write(const void* data, int bytes) {
		std::unique_lock<std::mutex> lock(writeMutex);
		int magic = 'blok';
		socket->write(&magic, sizeof(magic));
		socket->write(&bytes, sizeof(bytes));
		return socket->write(data, bytes);
	}

	bool Connection::write(Packet& packet) {
		return write(packet.data(), packet.size());
	}

	void Connection::runImpl(const std::function<void(Connection* conn, void* data, int bytes)>& callback) {
		while (socket->isConnected()) {

			int magic = 0;
			while (magic != 'blok') {
				int magicSize = sizeof(magic);
				if (!socket->read(&magic, magicSize)) {
					break;
				}
				if (magic != 'blok') {
					//packet magic mismatch
				}
			}
			if (magic != 'blok') {
				break;
			}

			int packetSize = 0;
			int packetSizeSize = sizeof(packetSize);
			if (socket->read(&packetSize, packetSizeSize)) {
				if (packetSizeSize != sizeof(packetSize)) {
					break;
				}
				if (packetSize > 1024 * 128) {
					//packet size limit exceeded
					break;
				}

				if (buffer.size() < packetSize) {
					buffer.resize(packetSize);
				}
				int index = 0;
				while (index < packetSize) {
					int bytes = packetSize - index;
					if (socket->read(buffer.data() + index, bytes)) {
						index += bytes;
					}
					else {
						break;
					}
				}
				if (index != packetSize) {
					break;
				}
				callback(this, buffer.data(), packetSize);
			}
			else {
				break;
			}
		}
	}

}
