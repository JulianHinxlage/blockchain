//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "network/tcp/Server.h"
#include "util/Blob.h"
#include <set>

typedef Blob<128> PeerId;

enum class PeerOpcode {
	NOOP,
	PING,
	PONG,
	HANDSHAKE,
	HANDSHAKE_REPLY,
	LOOKUP,
	LOOKUP_REPLY,
	ROUTE,
	BROADCAST,
	MESSAGE,
	DISCONNECT,
};

enum class PeerState {
	UNKNOWN,
	HANDSHAKE,
	CONNECTED,
	DISCONNECTED,
};

enum class PeerType {
	UNKNOWN,
	CLIENT,
	MULTI_CLIENT,
	SERVER,
};

enum class PeerNetworkState {
	DISCONNECTED,
	CONNECTING_TO_ENTRY_NODE,
	JOINING_LOOKUP,
	JOINING_CONNECTING,
	CONNECTED,
};

class Peer {
public:
	PeerId id = 0;
	PeerState state = PeerState::UNKNOWN;
	PeerType type;
	net::Connection* conn = nullptr;
};

class PeerNetwork {
public:
	std::function<void(const std::string &msg, PeerId source, bool broadcast)> messageCallback;
	std::function<void(PeerNetworkState)> onStateChanged;

	~PeerNetwork();
	void connect(const std::string &address, uint16_t port, PeerType type = PeerType::SERVER);
	void disconnect();
	void addEntryNode(const std::string& address, uint16_t port);
	void wait();
	uint16_t getPort();
	void send(PeerId destination, const std::string& message);
	PeerId getRandomNeighbor();
	int getNeighborCount();
	void broadcast(const std::string& message);
	bool isConnected();
private:
	std::vector<net::Endpoint> entryNodes;
	net::Server server;
	PeerId peerId;
	std::vector<std::shared_ptr<Peer>> peers;
	PeerNetworkState state = PeerNetworkState::DISCONNECTED;
	int pendingLookups = 0;
	PeerType type = PeerType::UNKNOWN;
	uint16_t port;
	std::string address;
	std::set<PeerId> lookupReplies;
	std::mutex mutex;
	std::set<Blob<128>> seenBroadcasts;

	void onConnect(net::Connection* conn);
	void onDisconnect(net::Connection* conn);
	void onMessage(net::Connection* conn, PeerId source, Buffer &msg, bool direct, bool broadcast = false);

	void join(net::Connection* conn, PeerId relay);
	Peer* nextPeer(PeerId id, bool ignoreLocal, PeerId except = PeerId(0));
	void sendRaw(PeerId destination, const Buffer& msg);
	void changeState(PeerNetworkState newState);

	Buffer msgCreateHandshake();
	Buffer msgCreateHandshakeReply();
	Buffer msgCreateLookup(PeerId target, PeerId relay);
	Buffer msgCreateLookupReply(PeerId destination, PeerId target, PeerId relay);
};
