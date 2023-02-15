#include "PeerNetwork.h"
#include "util/hex.h"
#include "util/random.h"
#include <sstream>

/*
-- Legend -- 
NOOP  <- Opcode (32 bits)
(128) <- size of field in bits
(str) <- byte sequence terminated by null character

-- Protocol --
NOOP
HANDSHAKE		id(128) port(16) ip(str)
HANDSHAKE_REPLY id(128) port(16) ip(str) answerIp(str)
LOOKUP			relay(128) target(128)
LOOKUP_REPLY	source(128) target(128) port(16) ip(str)
ROUTE			source(128) destination(128) exact(8) <packet>
MESSAGE			msg(str)
BORADCAST		source(128) nonce(128) msg(str)
DISCONNECT
*/

const char* getOpcodeName(PeerNetwork::Opcode opcode) {
	switch (opcode) {
	case PeerNetwork::Opcode::NOOP:
		return "NOOP";
	case PeerNetwork::Opcode::PING:
		return "PING";
	case PeerNetwork::Opcode::PONG:
		return "PONG";
	case PeerNetwork::Opcode::HANDSHAKE:
		return "HANDSHAKE";
	case PeerNetwork::Opcode::HANDSHAKE_REPLY:
		return "HANDSHAKE_REPLY";
	case PeerNetwork::Opcode::LOOKUP:
		return "LOOKUP";
	case PeerNetwork::Opcode::LOOKUP_REPLY:
		return "LOOKUP_REPLY";
	case PeerNetwork::Opcode::ROUTE:
		return "ROUTE";
	case PeerNetwork::Opcode::BROADCAST:
		return "BROADCAST";
	case PeerNetwork::Opcode::MESSAGE:
		return "MESSAGE";
	case PeerNetwork::Opcode::DISCONNECT:
		return "DISCONNECT";
	default:
		return "INVALID";
	}
}

PeerNetwork::PeerNetwork() {
	randomBytes(routingTable.localPeer->id);
	joinLockupCount = 64;
}

void PeerNetwork::addEntryNode(const Endpoint& ep) {
	entryNodes.push_back(ep);
}

bool PeerNetwork::start(uint16_t port, const std::string ip, uint16_t maxPortOffset) {
	routingTable.localPeer->ep.ip = ip;
	routingTable.localPeer->ep.port = port;

	bool portFound = false;
	for (int i = 0; i < maxPortOffset + 1; i++) {
		if (!listener.runListen(port, ip, [&](std::shared_ptr<net::Connection> con) {
			auto peer = std::make_shared<Peer>();
			peer->connection = con;
			auto ep = con->socket->getEndpoint();
			peer->ep.port = ep.getPort();
			peer->ep.ip = ep.getAddress();
			routingTable.add(peer);
			processConnection(peer.get());
		})) {
			port++;
			routingTable.localPeer->ep.port = port;
		}
		else {
			portFound = true;
			break;
		}
	}

	if (!portFound) {
		return false;
	}

	return true;
}

void PeerNetwork::stop() {
	if (listenerThread != nullptr) {
		listener.stop();
		listenerThread->join();
		listenerThread = nullptr;
	}
}

Peer *PeerNetwork::connectToPeer(const Endpoint& ep, PeerId id, bool waitForHandshake) {
	auto peer = std::make_shared<Peer>();
	peer->connection = std::make_shared<net::Connection>();
	peer->id = id;

	peer->connection->onConnect = [&, peer](net::Connection* conn) {
		peer->ep = ep;
		routingTable.add(peer);

		if (logCallback) {
			std::stringstream stream;
			stream << "connection " << peer->ep.ip << ":" << peer->ep.port;
			logCallback(stream.str(), 2);
		}

		Packet packet;
		packet.add(Opcode::HANDSHAKE);
		packet.add(routingTable.localPeer->id);
		packet.add(routingTable.localPeer->ep.port);
		packet.addStr(routingTable.localPeer->ep.ip);
		peer->connection->write(packet);
	};

	peer->connection->onDisconnect = [&, peer](net::Connection* conn) {
		onDisconnectPeer(peer.get());
	};

	peer->connection->runConnect(ep.ip, ep.port, [&, peer](net::Connection* conn, void* data, int bytes) {
		std::string msg((char*)data, bytes);
		processPacket(msg, peer.get(), peer->id, true);
	});

	return peer.get();
}

void PeerNetwork::lookupPeer(PeerId target, Peer *relay) {
	Packet packet;
	packet.add(Opcode::ROUTE);
	packet.add(routingTable.localPeer->id);
	packet.add(target);
	packet.add(uint8_t(0));
	packet.add(Opcode::LOOKUP);
	if (relay) {
		packet.add(relay->id);
	}
	else {
		packet.add(routingTable.localPeer->id);
	}
	packet.add(target);

	requestedLookups.insert(target);
	if (relay) {
		relay->connection->write(packet);
	}
	else {
		sendPacket(packet, target);
	}
}

bool PeerNetwork::connect() {
	for (auto& entry : entryNodes) {
		if (entry != routingTable.localPeer->ep) {
			if (logCallback) {
				std::stringstream stream;
				stream << "try to connect to entry node " << entry.ip << ":" << entry.port;
				logCallback(stream.str(), 1);
			}
			Peer* peer = connectToPeer(entry, PeerId(0), true);

			if (peer->connection->socket->isConnected()) {
				while (peer->id == PeerId(0)) {
					//wait for handshake to complete
				}
				if (peer) {
					for (int i = 0; i < joinLockupCount; i++) {
						PeerId target = routingTable.lookupTarget(i);
						lookupPeer(target, peer);
					}
				}
			}
			if (isConnected()) {
				break;
			}
		}
	}

	return isConnected();
}

void PeerNetwork::disconnect() {
	Packet packet;
	packet.add(Opcode::DISCONNECT);

	for (auto& peer : routingTable.peers) {
		if (peer && peer->connection) {
			if (peer->connection->socket->isConnected()) {
				peer->connection->write(packet);
			}
		}
	}
	routingTable.peers.clear();
}

bool PeerNetwork::isConnected() {
	return routingTable.peers.size() > 0;
}

const Peer& PeerNetwork::localPeer() {
	return *routingTable.localPeer;
}

void PeerNetwork::send(const std::string& msg, PeerId target) {
	Packet packet;
	packet.add(Opcode::ROUTE);
	packet.add(routingTable.localPeer->id);
	packet.add(target);
	packet.add(uint8_t(1));
	packet.add(Opcode::MESSAGE);
	packet.add(msg.data(), msg.size());

	sendPacket(packet, target);
}

void PeerNetwork::broadcast(const std::string& msg) {
	Packet packet;
	packet.add(Opcode::BROADCAST);
	Blob<128> nonce;
	randomBytes(nonce);
	packet.add(routingTable.localPeer->id);
	packet.add(nonce);
	packet.add(msg.data(), msg.size());

	broadcastNonces.insert(nonce);
	for(auto &peer : routingTable.peers) {
		if (peer && peer->connection) {
			if (peer->connection->socket->isConnected()) {
				peer->connection->write(packet);
			}
		}
	}
}

void PeerNetwork::sendPacket(Packet& packet, PeerId target, PeerId except) {
	Peer *next = routingTable.getClosest(target, except, false);
	if (next && next->connection) {
		if (next->connection->socket->isConnected()) {
			next->connection->write(packet);
		}
	}
}

void PeerNetwork::processConnection(Peer *peer) {
	if (logCallback) {
		std::stringstream stream;
		stream << "connection " << peer->ep.ip << ":" << peer->ep.port;
		logCallback(stream.str(), 2);
	}

	peer->connection->onDisconnect = [&, peer](net::Connection* conn) {
		onDisconnectPeer(peer);
	};

	peer->connection->run([&, peer](net::Connection* conn, void* data, int bytes) {
		std::string msg((char*)data, bytes);
		processPacket(msg, peer, peer->id, true);
	});
}

void PeerNetwork::processPacket(const std::string& msg, Peer* peer, PeerId msgSource, bool wasDirectlySend) {
	Packet packet;
	packet.add((char*)msg.data(), msg.size());

	Opcode opcode = packet.get<Opcode>();

	if (logCallback) {
		std::stringstream stream;
		stream << "packet opcode " << getOpcodeName(opcode) << " length " << msg.size();
		logCallback(stream.str(), 0);
	}

	switch (opcode) {
	case Opcode::NOOP:
		break;
	case Opcode::HANDSHAKE: {
		if (wasDirectlySend) {
			Endpoint ep;
			PeerId id = packet.get<PeerId>();
			ep.port = packet.get<uint16_t>();
			ep.ip = packet.getStr();

			peer->id = id;
			peer->ep = ep;

			Packet response;
			response.add(Opcode::HANDSHAKE_REPLY);
			response.add(routingTable.localPeer->id);
			response.add(routingTable.localPeer->ep.port);
			response.addStr(routingTable.localPeer->ep.ip);
			response.addStr(peer->connection->socket->getEndpoint().getAddress());
			sendPacket(response, peer->id);
		}
		break;
	}
	case Opcode::HANDSHAKE_REPLY: {
		if (wasDirectlySend) {
			Endpoint ep;
			PeerId id = packet.get<PeerId>();
			ep.port = packet.get<uint16_t>();
			ep.ip = packet.getStr();
			std::string answerId = packet.getStr();

			peer->id = id;
			peer->ep = ep;
		}
		break;
	}
	case Opcode::LOOKUP: {
		PeerId relay = packet.get<PeerId>();
		PeerId target = packet.get<PeerId>();

		Packet response;
		bool needsRelay = relay != routingTable.localPeer->id && relay != msgSource;
		if (needsRelay) {
			response.add(Opcode::ROUTE);
			response.add(routingTable.localPeer->id);
			response.add(relay);
			response.add(uint8_t(1));//exact
		}
		response.add(Opcode::ROUTE);
		response.add(routingTable.localPeer->id);
		response.add(msgSource);
		response.add(uint8_t(1));//exact
		response.add(Opcode::LOOKUP_REPLY);
		response.add(routingTable.localPeer->id);
		response.add(target);
		response.add(routingTable.localPeer->ep.port);
		response.addStr(routingTable.localPeer->ep.ip);

		if (needsRelay) {
			sendPacket(response, relay);
		}
		else {
			sendPacket(response, msgSource);
		}
		break;
	}
	case Opcode::LOOKUP_REPLY: {
		PeerId source = packet.get<PeerId>();
		PeerId target = packet.get<PeerId>();
		Endpoint ep;
		ep.port = packet.get<uint16_t>();
		ep.ip = packet.getStr();

		if (requestedLookups.contains(target)) {
			requestedLookups.erase(target);
		}
		else {
			break;
		}

		if (!routingTable.has(source) && source != routingTable.localPeer->id) {
			connectToPeer(ep, source, true);
		}
		break;
	}
	case Opcode::ROUTE: {
		PeerId source = packet.get<PeerId>();
		PeerId destination = packet.get<PeerId>();
		uint8_t exact = packet.get<uint8_t>();

		Peer* next = routingTable.getClosest(destination, peer->id, true);
		if (next) {
			if (next == routingTable.localPeer.get()) {
				if (exact == 0 || (exact == 1 && destination == routingTable.localPeer->id)) {
					processPacket(packet.remaining(), peer, source, false);
				}
				else {
					if (logCallback) {
						logCallback("packet dropped", 0);
					}
				}
			}
			else {
				if (next->connection) {
					packet.unskip(packet.offset);
					next->connection->write(packet);
				}
			}
		}
		break;
	}
	case Opcode::MESSAGE:
		if (msgCallback) {
			msgCallback(packet.remaining(), msgSource);
		}
		break;
	case Opcode::BROADCAST: {
		PeerId source = packet.get<PeerId>();
		PeerId nonce = packet.get<PeerId>();
		
		mutex.lock();
		if (broadcastNonces.contains(nonce)) {
			mutex.unlock();
			break;
		}
		broadcastNonces.insert(nonce);
		mutex.unlock();

		std::string msg = packet.remaining();
		packet.unskip(packet.offset);
		for (auto& i : routingTable.peers) {
			if (i && i->connection) {
				if (i.get() != peer) {
					i->connection->write(packet);
				}
			}
		}
		if (msgCallback) {
			msgCallback(msg, source);
		}
		break;
	}
	case Opcode::DISCONNECT:
		if (wasDirectlySend) {
			peer->connection->socket->disconnect();
		}
		break;
	default:
		break;
	}

}

PeerId PeerNetwork::getRandomNeighbor() {
	if (routingTable.peers.size() > 0) {
		int index = randomBytes<uint32_t>() % routingTable.peers.size();
		return routingTable.peers[index]->id;
	}
	else {
		return PeerId(0);
	}
}

void PeerNetwork::onDisconnectPeer(Peer* peer) {
	int level = routingTable.getLevel(peer->id);
	lookupPeer(routingTable.lookupTarget(level));

	if (logCallback) {
		std::stringstream stream;
		stream << "disconnect " << peer->ep.ip << ":" << peer->ep.port;
		logCallback(stream.str(), 2);
	}
}
