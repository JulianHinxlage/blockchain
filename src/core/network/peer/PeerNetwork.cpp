//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "PeerNetwork.h"
#include "util/hex.h"
#include <random>

template<typename T>
T random() {
	static std::mt19937_64 *rng = nullptr;
	if (rng == nullptr) {
		rng = new std::mt19937_64();
		rng->seed(std::random_device()());
	}
	T output = T();
	uint64_t value = rng->operator()();
	int j = 0;
	for (int i = 0; i < sizeof(T); i++) {
		if (j >= sizeof(value)) {
			value = rng->operator()();
			j = 0;
		}
		((uint8_t*)&output)[i] = ((uint8_t*)&value)[j++];
	}
	return output;
}


PeerNetwork::~PeerNetwork() {
	disconnect();
}

void PeerNetwork::connect(uint16_t port) {
	state = PeerNetworkState::DISCONNECTED;
	peerId = random<PeerId>();
	pendingLookups = 0;
	server.packetize = true;

	server.connectCallback = [&](net::Connection* conn) {
		if (logCallback) {
			logCallback("connection: " + conn->getAddress() + " " + std::to_string(conn->getPort()), 0);
		}
		onConnect(conn);
	};
	server.disconnectCallback = [&](net::Connection* conn) {
		if (logCallback) {
			logCallback("disconnect: " + conn->getAddress() + " " + std::to_string(conn->getPort()), 0);
		}
		onDisconnect(conn);
	};
	server.readCallback = [&](net::Connection* conn, Buffer& msg) {
		Peer* peer = nullptr;

		{
			std::unique_lock<std::mutex> lock(mutex);
			for (auto& p : peers) {
				if (p->conn == conn) {
					peer = p.get();
				}
			}
		}

		if (peer == nullptr) {
			return;
		}
		
		onMessage(conn, peer->id, msg, true);
	};
	for (int i = 0; i < 100; i++) {
		if (server.listen(port, false, false, true) == net::ErrorCode::NO_ERROR) {
			break;
		}
		port++;
	}
	server.run();
	this->port = port;
	this->address = "127.0.0.1";

	state = PeerNetworkState::CONNECTING_TO_ENTRY_NODE;
	for (auto& node : entryNodes) {
		if (node.getAddress() != "127.0.0.1" || node.getPort() != port) {
			if (server.connectAsClient(node) == net::ErrorCode::NO_ERROR) {
				break;
			}
			else {
				if (peers.size() > 0) {
					break;
				}
			}
		}
	}

	server.errorCallback = [&](net::Connection* conn, net::ErrorCode error) {
		if (conn) {
			if (logCallback) {
				logCallback("error " + std::string(net::getErrorString(error)) + ": " + conn->getAddress() + " " + std::to_string(conn->getPort()), 1);
			}
		}
		else {
			if (logCallback) {
				logCallback("error " + std::string(net::getErrorString(error)), 1);
			}
		}
	};
}

void PeerNetwork::disconnect() {
	server.close();
	std::unique_lock<std::mutex> lock(mutex);
	for (auto& p : peers) {
		p->conn->disconnect();
	}
	peers.clear();
	state = PeerNetworkState::DISCONNECTED;
}

void PeerNetwork::addEntryNode(const std::string& address, uint16_t port) {
	entryNodes.push_back(net::Endpoint(address, port, true));
}

void PeerNetwork::wait() {
	server.waitWhileRunning();
}

void PeerNetwork::send(PeerId destination, const std::string& message) {
	Buffer packet;
	packet.write(PeerOpcode::ROUTE);
	packet.write(peerId);
	packet.write(destination);
	packet.write((bool)true);

	packet.write(PeerOpcode::MESSAGE);
	packet.write(peerId);
	packet.writeBytes(message.data(), message.size());
	sendRaw(destination, packet);
}

void PeerNetwork::broadcast(const std::string& message) {
	Buffer packet;
	packet.write(PeerOpcode::BROADCAST);
	packet.write(peerId);

	auto msgId = random<Blob<128>>();
	packet.write(msgId);

	packet.write(PeerOpcode::MESSAGE);
	packet.write(peerId);
	packet.writeBytes(message.data(), message.size());

	seenBroadcasts.insert(msgId);

	for (auto& peer : peers) {
		peer->conn->write(packet);
	}
}

bool PeerNetwork::isConnected() {
	if (state == PeerNetworkState::CONNECTED) {
		if (peers.size() > 0) {
			return true;
		}
	}
	return false;
}

void PeerNetwork::onConnect(net::Connection* conn) {
	auto peer = std::make_shared<Peer>();
	peer->conn = conn;
	peer->state = PeerState::HANDSHAKE;
	{
		std::unique_lock<std::mutex> lock(mutex);
		peers.push_back(peer);
	}

	conn->write(msgCreateHandshake());
}

void PeerNetwork::onDisconnect(net::Connection* conn) {
	PeerId id;
	{
		std::unique_lock<std::mutex> lock(mutex);
		for (int i = 0; i < peers.size(); i++) {
			if (peers[i]->conn == conn) {
				id = peers[i]->id;
				peers.erase(peers.begin() + i);
				break;
			}
		}
	}

	int minIndex = -1;
	PeerId minDist = PeerId(0);
	for (int i = 0; i < sizeof(PeerId) * 8; i++) {
		PeerId target = peerId ^ (PeerId(1) << i);
		PeerId dist = target ^ id;
		if (minIndex == -1 || dist < minDist) {
			minIndex = i;
			minDist = dist;
		}
	}

	if (minIndex != -1) {
		PeerId target = peerId ^ (PeerId(1) << minIndex);
		Peer* peer = nextPeer(target, true);
		if (peer) {
			sendRaw(target, msgCreateLookup(target, peerId));
			pendingLookups++;
		}
	}
}

void PeerNetwork::onMessage(net::Connection* conn, PeerId msgSource, Buffer& msg, bool direct, bool broadcast) {
	int readIndex = msg.getReadIndex();
	PeerOpcode opcode = msg.read<PeerOpcode>();
	PeerId source = msg.read<PeerId>();

	if (broadcast && opcode != PeerOpcode::MESSAGE) {
		return;
	}

	if (opcode == PeerOpcode::HANDSHAKE && direct) {
		{
			std::unique_lock<std::mutex> lock(mutex);
			for (auto& peer : peers) {
				if (peer->conn == conn) {
					peer->id = source;
				}
			}
		}

		conn->write(msgCreateHandshakeReply());
	}
	else if (opcode == PeerOpcode::HANDSHAKE_REPLY && direct) {
		{
			std::unique_lock<std::mutex> lock(mutex);
			for (auto& peer : peers) {
				if (peer->conn == conn) {
					peer->id = source;
					peer->state = PeerState::CONNECTED;
				}
			}
		}

		if (state == PeerNetworkState::CONNECTING_TO_ENTRY_NODE) {
			state = PeerNetworkState::JOINING;
			join(conn, source);
		}
	}
	else if (opcode == PeerOpcode::ROUTE) {
		PeerId destination = msg.read<PeerId>();
		bool exect = msg.read<bool>();

		Peer* next = nextPeer(destination, false, source);
		if (next) {
			msg.reset();
			msg.skip(readIndex);
			next->conn->write(msg);
		}
		else {
			if (!exect || destination == peerId) {
				onMessage(conn, source, msg, false);
			}
		}
	}
	else if (opcode == PeerOpcode::LOOKUP) {
		PeerId target = msg.read<PeerId>();
		PeerId relay = msg.read<PeerId>();

		if (peerId != relay) {
			sendRaw(relay, msgCreateLookupReply(source, target, relay));
		}
		else {
			sendRaw(source, msgCreateLookupReply(source, target, relay));
		}
	}
	else if (opcode == PeerOpcode::LOOKUP_REPLY) {
		PeerId target = msg.read<PeerId>();
		std::string address = msg.readStr();
		uint16_t port = msg.read<uint16_t>();

		if (pendingLookups > 0) {
			pendingLookups--;

			if (!lookupRelies.contains(source)) {
				lookupRelies.insert(source);

				bool found = false;
				{
					std::unique_lock<std::mutex> lock(mutex);
					for (auto& peer : peers) {
						if (peer->id == source) {
							found = true;
							break;
						}
					}
				}
				if (!found) {
					server.connectAsClient(address, port);
				}
			}

			if (pendingLookups == 0 && state == PeerNetworkState::JOINING) {
				state = PeerNetworkState::CONNECTED;
				{
					std::unique_lock<std::mutex> lock(mutex);
					for (auto& p : peers) {
						if (p->state == PeerState::CONNECTED) {
							if (!lookupRelies.contains(p->id)) {
								p->conn->disconnect();
							}
						}
					}
				}
			}
		}
	}
	else if (opcode == PeerOpcode::BROADCAST && direct) {
		auto msgId = msg.read<Blob<128>>();
		mutex.lock();
		if (!seenBroadcasts.contains(msgId)) {
			seenBroadcasts.insert(msgId);
			mutex.unlock();

			int subIndex = msg.getReadIndex();
			msg.reset();
			msg.skip(readIndex);
			for (auto& peer : peers) {
				if (peer->conn != conn) {
					peer->conn->write(msg);
				}
			}

			msg.reset();
			msg.skip(subIndex);
			onMessage(conn, source, msg, false, true);
		}
		else {
			mutex.unlock();
		}
	}
	else if (opcode == PeerOpcode::MESSAGE) {
		std::string str((char*)msg.data(), msg.size());
		if (messageCallback) {
			messageCallback(str, source, broadcast);
		}
	}
}

void PeerNetwork::join(net::Connection* conn, PeerId relay) {
	pendingLookups = 0;
	for (int i = 0; i < sizeof(PeerId) * 8; i++) {
		PeerId target = peerId ^ (PeerId(1) << i);
		conn->write(msgCreateLookup(target, relay));
		pendingLookups++;
	}
}

Peer* PeerNetwork::nextPeer(PeerId id, bool ignoreLocal, PeerId except) {
	PeerId minDist = peerId ^ id;
	Peer* next = nullptr;
	{
		std::unique_lock<std::mutex> lock(mutex);
		for (auto& p : peers) {
			if (p->state == PeerState::CONNECTED) {
				if (p->id != except) {
					PeerId dist = p->id ^ id;
					if (dist < minDist || (ignoreLocal && next == nullptr)) {
						minDist = dist;
						next = p.get();
					}
				}
			}
		}
	}
	return next;
}

void PeerNetwork::sendRaw(PeerId destination, const Buffer& msg) {
	Peer* peer = nextPeer(destination, true);
	if (peer) {
		peer->conn->write(msg);
	}
}

Buffer PeerNetwork::msgCreateHandshake(){
	Buffer msg;
	msg.write(PeerOpcode::HANDSHAKE);
	msg.write(peerId);
	return msg;
}

Buffer PeerNetwork::msgCreateHandshakeReply() {
	Buffer msg;
	msg.write(PeerOpcode::HANDSHAKE_REPLY);
	msg.write(peerId);
	return msg;
}

Buffer PeerNetwork::msgCreateLookup(PeerId target, PeerId relay) {
	Buffer msg;
	msg.write(PeerOpcode::ROUTE);
	msg.write(peerId);
	msg.write(target);
	msg.write((bool)false);

	msg.write(PeerOpcode::LOOKUP);
	msg.write(peerId);
	msg.write(target);
	msg.write(relay);
	return msg;
}

Buffer PeerNetwork::msgCreateLookupReply(PeerId destination, PeerId target, PeerId relay) {
	Buffer msg;
	if (peerId != relay && relay != destination) {
		msg.write(PeerOpcode::ROUTE);
		msg.write(peerId);
		msg.write(relay);
		msg.write((bool)true);
	}

	msg.write(PeerOpcode::ROUTE);
	msg.write(peerId);
	msg.write(destination);
	msg.write((bool)true);


	msg.write(PeerOpcode::LOOKUP_REPLY);
	msg.write(peerId);
	msg.write(target);
	msg.writeStr(address);
	msg.write(port);
	return msg;
}
