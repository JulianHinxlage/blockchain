#pragma once

#include "Listener.h"
#include "PeerRoutingTable.h"
#include "Packet.h"
#include <set>

class PeerNetwork {
public:
	enum class Opcode {
		NOOP,
		PING,
		PONG,
		HANDSHAKE,
		HANDSHAKE_REPLY,
		LOOKUP,
		LOOKUP_REPLY,
		ROUTE,
		MESSAGE,
		BROADCAST,
		DISCONNECT,
	};
	std::function<void(const std::string& msg, int level)> logCallback;
	std::function<void(const std::string& msg, PeerId id)> msgCallback;

	PeerNetwork();
	void addEntryNode(const Endpoint& ep);

	bool start(uint16_t port, const std::string ip = "127.0.0.1", uint16_t maxPortOffset = 0);
	void stop();
	bool connect();
	void disconnect();
	bool isConnected();

	const Peer& localPeer();
	void send(const std::string& msg, PeerId target);
	void broadcast(const std::string& msg);

	PeerId getRandomNeighbor();

private:
	std::vector<Endpoint> entryNodes;
	Listener listener;
	std::shared_ptr<std::thread> listenerThread;
	PeerRoutingTable routingTable;
	std::set<Blob<128>> broadcastNonces;
	int joinLockupCount;
	std::set<PeerId> requestedLookups;

	void sendPacket(Packet& packet, PeerId target, PeerId except = PeerId(0));
	void processConnection(Peer* peer);
	void processNextPacket(Peer *peer);
	void processPacket(const std::string &msg, Peer* peer, PeerId msgSource, bool wasDirectlySend);
	Peer *connectToPeer(const Endpoint &ep, PeerId id, bool waitForHandshake);
	void lookupPeer(PeerId target, Peer* relay = nullptr);
};