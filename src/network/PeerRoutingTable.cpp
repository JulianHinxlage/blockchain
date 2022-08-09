#include "PeerRoutingTable.h"

PeerRoutingTable::PeerRoutingTable() {
    localPeer = std::make_shared<Peer>();
}

Peer* PeerRoutingTable::getClosest(const PeerId& id, const PeerId& except, bool includeLocal) {
    int minIndex = -1;
    PeerId minDistance(0);

    for (int i = 0; i < peers.size(); i++) {
        auto& peer = peers[i];
        if (peer && peer->id != PeerId(0)) {
            if (peer->id != except) {
                PeerId distance = id ^ peer->id;
                if (minIndex == -1 || distance < minDistance) {
                    minDistance = distance;
                    minIndex = i;
                }
            }
        }
    }

    if (includeLocal) {
        PeerId distance = id ^ localPeer->id;
        if (minIndex == -1 || distance < minDistance) {
            minDistance = distance;
            return localPeer.get();
        }
    }

    if (minIndex != -1) {
        return peers[minIndex].get();
    }
    else {
        return nullptr;
    }
}

void PeerRoutingTable::add(std::shared_ptr<Peer> peer) {
    peers.push_back(peer);
}

bool PeerRoutingTable::has(const PeerId& id) {
    for (int i = 0; i < peers.size(); i++) {
        auto& peer = peers[i];
        if (peer) {
            if (peer->id == id) {
                return true;
            }
        }
    }
    return false;
}

std::shared_ptr<Peer> PeerRoutingTable::remove(const PeerId& id) {
    for (int i = 0; i < peers.size(); i++) {
        auto peer = peers[i];
        if (peer) {
            if (peer->id == id) {
                peers.erase(peers.begin() + i);
                return peer;
            }
        }
    }
    return nullptr;
}

PeerId PeerRoutingTable::lookupTarget(int level) {
    level = 255 - level;
    int bit = level / 2;
    PeerId dist(0);
    dist ^= (PeerId(1) << bit);
    if (level % 2 == 1) {
        dist ^= (PeerId(1) << bit - 1);
    }
    return localPeer->id ^ dist;
}

int PeerRoutingTable::getLevel(const PeerId& id) {
    PeerId dist = id ^ localPeer->id;
    int bit = -1;
    while (dist > PeerId(0)) {
        dist = dist >> 1;
        bit++;
    }

    int level = bit * 2;
    dist = id ^ localPeer->id;
    if ((dist & (PeerId(1) << bit - 1)) == (PeerId(1) << bit - 1)) {
        level++;
    }

    return 255 - level;
}
