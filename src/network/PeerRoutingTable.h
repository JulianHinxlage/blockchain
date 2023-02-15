#pragma once

#include "util/Blob.h"
#include "Connection.h"
#include <vector>
#include <string>

class Endpoint {
public:
	uint16_t port;
	std::string ip;

    Endpoint() {
        port = 0;
        ip = "";
    }

    Endpoint(const std::string &ip, uint16_t port)
        : port(port), ip(ip) {}

    bool operator==(const Endpoint& ep) const {
        return port == ep.port && ip == ep.ip;
    }
};

typedef Blob<128> PeerId;

class Peer {
public:
	PeerId id;
	Endpoint ep;
    std::shared_ptr<net::Connection> connection;
};

class PeerRoutingTable {
public:
    std::vector<std::shared_ptr<Peer>> peers;
    std::shared_ptr<Peer> localPeer;

    PeerRoutingTable();
    Peer* getClosest(const PeerId& id, const PeerId& except = PeerId(0), bool includeLocal = false);
    void add(std::shared_ptr<Peer> peer);
    bool has(const PeerId& id);
    std::shared_ptr<Peer> remove(const PeerId &id);
    PeerId lookupTarget(int level);
    int getLevel(const PeerId &id);
};
