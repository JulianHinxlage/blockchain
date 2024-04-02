//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include "Connection.h"

namespace net {

	Connection::Connection() {
		thread = nullptr;
		running = false;
		readCallback = nullptr;
		disconnectCallback = nullptr;
		connectCallback = nullptr;
		errorCallback = nullptr;
		packetize = false;
		outbound = false;
		maxPacketSize = 1024 * 1024 * 16;
	}

	Connection::Connection(Connection&& conn) {
		thread = conn.thread;
		running = conn.running;
		readCallback = conn.readCallback;
		disconnectCallback = conn.disconnectCallback;
		connectCallback = conn.connectCallback;
		errorCallback = conn.errorCallback;

		conn.thread = nullptr;
		conn.socket = nullptr;
		conn.readCallback = nullptr;
		conn.disconnectCallback = nullptr;
		conn.connectCallback = nullptr;
		conn.errorCallback = nullptr;
	}

	Connection::~Connection() {
		close();
	}

	ErrorCode Connection::connect(const Endpoint& endpoint) {
		if (!socket) {
			socket = std::make_shared<TcpSocket>();
		}
		ErrorCode error = socket->connect(endpoint);
		if (error != ErrorCode::NO_ERROR) {
			if (errorCallback) {
				errorCallback(this, error);
			}
		}
		else {
			outbound = true;
			if (connectCallback) {
				connectCallback(this);
			}
		}
		return error;
	}

	ErrorCode Connection::connect(const std::string& address, uint16_t port, bool resolve, bool prefereIpv4) {
		if (!socket) {
			socket = std::make_shared<TcpSocket>();
		}
		ErrorCode error = socket->connect(address, port, resolve, prefereIpv4);
		if (error != ErrorCode::NO_ERROR) {
			if (errorCallback) {
				errorCallback(this, error);
			}
		}
		else {
			outbound = true;
			if (connectCallback) {
				connectCallback(this);
			}
		}
		return error;
	}

	void Connection::run() {
		if (running) {
			return;
		}

		running = true;
		thread = new std::thread([&]() {
			Buffer buffer;
			while (running) {
				buffer.reset();
				ErrorCode error = read(buffer);
				if (error != ErrorCode::NO_ERROR) {
					break;
				}
				if (readCallback) {
					readCallback(this, buffer);
				}
			}
			running = false;
			if (disconnectCallback) {
				disconnectCallback(this);
			}
		});
	}

	bool Connection::isRunning() {
		return running;
	}

	ErrorCode Connection::write(const Buffer& buffer) {
		std::unique_lock<std::mutex> lock(writeMutex);
		if (!socket) {
			return ErrorCode::DISCONNECTED;
		}

		ErrorCode error;
		if (packetize) {
			int packetSize = buffer.size();
			error = socket->write(&packetSize, sizeof(packetSize));
			if (error == ErrorCode::NO_ERROR) {
				error = socket->write(buffer.data(), buffer.size());
			}
		}
		else {
			error = socket->write(buffer.data(), buffer.size());
		}

		if (error != ErrorCode::NO_ERROR) {
			if (errorCallback) {
				errorCallback(this, error);
			}
		}
		return error;
	}

	ErrorCode Connection::read(Buffer& buffer) {
		if (!socket) {
			return ErrorCode::DISCONNECTED;
		}

		ErrorCode error;
		if (packetize) {
			int packetSize = 0;
			int bytes = sizeof(packetSize);
			error = socket->read(&packetSize, bytes);
			if (error == ErrorCode::NO_ERROR) {
				if (packetSize <= maxPacketSize && packetSize >= 0) {
					buffer.reserve(buffer.getWriteIndex() + packetSize);

					while (packetSize > 0) {
						bytes = packetSize;
						error = socket->read(buffer.dataWrite(), bytes);
						if (error != ErrorCode::NO_ERROR) {
							break;
						}
						buffer.skipWrite(bytes);
						packetSize -= bytes;
					}
				}
				else {
					error = ErrorCode::INVALID_PACKET;
				}
			}
		}
		else {
			buffer.reserve(buffer.getWriteIndex() + 1024);
			int bytes = buffer.sizeWrite();
			error = socket->read(buffer.dataWrite(), bytes);
			buffer.skipWrite(bytes);
			buffer.reserve(buffer.getWriteIndex());
		}

		if (error != ErrorCode::NO_ERROR) {
			if (errorCallback) {
				errorCallback(this, error);
			}
		}
		return error;
	}

	void Connection::close(bool force) {
		if (socket) {
			socket->disconnect();
		}
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

	void Connection::disconnect() {
		if (socket) {
			socket->disconnect();
		}
		running = false;
	}

	void Connection::waitWhileRunning() {
		if (thread) {
			thread->join();
		}
	}

	uint16_t Connection::getPort() const {
		if (socket) {
			return socket->getEndpoint().getPort();
		}
		else {
			return 0;
		}
	}

	std::string Connection::getAddress() const {
		if (socket) {
			return socket->getEndpoint().getAddress();
		}
		else {
			return "";
		}
	}

}
