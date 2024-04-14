//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "PeerNetwork.h"
#include "util/hex.h"
#include "util/random.h"
#include "util/log.h"

PeerNetwork::~PeerNetwork() {
	disconnect();
}

void PeerNetwork::connect(const std::string& address, uint16_t port, PeerType type) {
	state = PeerNetworkState::DISCONNECTED;
	this->type = type;

	peerId = random<PeerId>();
	pendingLookups = 0;
	server.packetize = true;

	server.connectCallback = [&](net::Connection* conn) {
		log(LogLevel::DEBUG, "Network", "connection %s %i", conn->getAddress().c_str(), conn->getPort());
		onConnect(conn);
	};
	server.disconnectCallback = [&](net::Connection* conn) {
		log(LogLevel::DEBUG, "Network", "disconnect %s %i", conn->getAddress().c_str(), conn->getPort());
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

	if (type != PeerType::CLIENT) {
		for (int i = 0; i < 100; i++) {
			if (server.listen(port, false, false, true) == net::ErrorCode::NO_ERROR) {
				break;
			}
			port++;
		}
		server.run();
		this->port = port;
		this->address = address;
	}

	changeState(PeerNetworkState::CONNECTING_TO_ENTRY_NODE);
	for (auto& node : entryNodes) {
		if (state != PeerNetworkState::DISCONNECTED) {
			if (node.getAddress() != "127.0.0.1" || node.getPort() != port || type == PeerType::CLIENT) {
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
	}

	server.errorCallback = [&](net::Connection* conn, net::ErrorCode error) {
		if (conn) {
			log(LogLevel::TRACE, "Network", "socket error %s: %s %i", net::getErrorString(error), conn->getAddress().c_str(), conn->getPort());
		}
		else {
			log(LogLevel::TRACE, "Network", "socket error %s", net::getErrorString(error));
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
	changeState(PeerNetworkState::DISCONNECTED);
}

void PeerNetwork::addEntryNode(const std::string& address, uint16_t port) {
	entryNodes.push_back(net::Endpoint(address, port, true));
}

void PeerNetwork::wait() {
	server.waitWhileRunning();
}

uint16_t PeerNetwork::getPort() {
	return port;
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

PeerId PeerNetwork::getRandomNeighbor() {
	int count = 0;
	for (auto& peer : peers) {
		if (peer->state == PeerState::CONNECTED) {
			if (peer->type == PeerType::SERVER) {
				count++;
			}
		}
	}
	if (count == 0) {
		return PeerId(0);
	}
	else {
		int index = random<uint32_t>() % count;
		count = 0;
		for (auto& peer : peers) {
			if (peer->state == PeerState::CONNECTED) {
				if (peer->type == PeerType::SERVER) {
					if (index == count) {
						return peer->id;
					}
					count++;
				}
			}
		}
		return PeerId(0);
	}
}

int PeerNetwork::getNeighborCount() {
	int count = 0;
	for (auto& peer : peers) {
		if (peer->state == PeerState::CONNECTED) {
			if (peer->type == PeerType::SERVER) {
				count++;
			}
		}
	}
	return count;
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
		for (auto& peer : peers) {
			if (peer->state == PeerState::CONNECTED) {
				return true;
			}
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

	if (peers.size() == 0) {
		changeState(PeerNetworkState::DISCONNECTED);
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
			PeerType type = msg.read<PeerType>();
			std::unique_lock<std::mutex> lock(mutex);
			for (auto& peer : peers) {
				if (peer->conn == conn) {
					peer->id = source;
					peer->type = type;
				}
			}
		}

		conn->write(msgCreateHandshakeReply());
	}
	else if (opcode == PeerOpcode::HANDSHAKE_REPLY && direct) {
		{
			PeerType type = msg.read<PeerType>();
			std::unique_lock<std::mutex> lock(mutex);
			for (auto& peer : peers) {
				if (peer->conn == conn) {
					peer->id = source;
					peer->state = PeerState::CONNECTED;
					peer->type = type;
				}
			}
		}
		if (state == PeerNetworkState::CONNECTING_TO_ENTRY_NODE) {
			if (type == PeerType::CLIENT) {
				changeState(PeerNetworkState::CONNECTED);
			}
			else {
				changeState(PeerNetworkState::JOINING_LOOKUP);
				join(conn, source);
			}
		}
		else if (state == PeerNetworkState::JOINING_CONNECTING) {
			changeState(PeerNetworkState::CONNECTED);
		}
		else if (state == PeerNetworkState::DISCONNECTED) {
			changeState(PeerNetworkState::CONNECTED);
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

			if (!lookupReplies.contains(source)) {
				lookupReplies.insert(source);

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

			if (pendingLookups == 0 && state == PeerNetworkState::JOINING_LOOKUP) {
				{
					std::unique_lock<std::mutex> lock(mutex);
					for (auto& p : peers) {
						if (p->state == PeerState::CONNECTED) {
							if (!lookupReplies.contains(p->id)) {
								p->state = PeerState::DISCONNECTED;
								p->conn->disconnect();
							}
						}
					}
				}

				bool found = false;
				for (auto& p : peers) {
					if (p->state == PeerState::CONNECTED) {
						found = true;
					}
				}

				if (found) {
					changeState(PeerNetworkState::CONNECTED);
				}
				else {
					changeState(PeerNetworkState::JOINING_CONNECTING);
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
						if (p->type == PeerType::SERVER || p->id == id) {
							minDist = dist;
							next = p.get();
						}
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

void PeerNetwork::changeState(PeerNetworkState newState) {
	if (state != newState) {
		state = newState;
		if (onStateChanged) {
			onStateChanged(state);
		}
	}
}

Buffer PeerNetwork::msgCreateHandshake(){
	Buffer msg;
	msg.write(PeerOpcode::HANDSHAKE);
	msg.write(peerId);
	msg.write(type);
	return msg;
}

Buffer PeerNetwork::msgCreateHandshakeReply() {
	Buffer msg;
	msg.write(PeerOpcode::HANDSHAKE_REPLY);
	msg.write(peerId);
	msg.write(type);
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
