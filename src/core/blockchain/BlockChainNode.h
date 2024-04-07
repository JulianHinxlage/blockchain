//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "network/peer/PeerNetwork.h"
#include "Block.h"
#include "util/Serializer.h"
#include <map>

enum class NodeOpcode {
	NOOP,
	BLOCK_HASH_REQUEST,
	BLOCK_HASH_REPLY,
	BLOCK_REQUEST,
	BLOCK_REPLY,
	TRANSACTION_REQUEST,
	TRANSACTION_REPLY,
	ACCOUNT_REQUEST,
	ACCOUNT_REPLY,
	BLOCK_BROADCAST,
	TRANSACTION_BREADCAST,
	REQUEST_ERROR,
};

enum class NodeType {
	UNKNOWN,
	FULL_NODE,
	LIGHT_NODE,
};

enum class NodeState {
	UNKNOWN,
	DISCONNECTED,
	JOINING,
	SYNCHRONIZING,
	CONNECTED,
};

typedef Blob<128> RequestId;

class Request {
public:
	RequestId id;
	std::function<void(NodeOpcode, Serializer &)> callback;
};

class RequestContenxt {
public:
	std::map<RequestId, Request> requests;

	void add(RequestId id, const std::function<void(NodeOpcode, Serializer&)>& callback) {
		requests[id] = { id, callback };
	}

	bool onRequest(RequestId id, NodeOpcode opcode, Serializer& serial) {
		auto i = requests.find(id);
		if (i != requests.end()) {
			if (i->second.callback) {
				i->second.callback(opcode, serial);
			}
			requests.erase(id);
			return true;
		}
		return false;
	}
};


class BlockChainNode {
public:
	class BlockChain* blockChain;
	std::function<void(const Block&)> onBlockRecived;
	std::function<void(const Transaction&)> onTransactionRecived;
	std::function<void(const std::string& msg, bool error)> logCalback;

	void init(NodeType type, const std::string& entryNodeFile = "");
	void connect(const std::string& address, uint16_t port);
	void disconnect();
	void sendBlock(const Block& block);
	void sendTransaction(const Transaction& transaction);
	void synchronize();

private:
	NodeState state = NodeState::UNKNOWN;
	NodeType type = NodeType::UNKNOWN;
	RequestContenxt requestContext;
	PeerNetwork network;
	bool isSynchronizationPending = false;

	void changeState(NodeState newState);
	void onMessage(const std::string &msg, PeerId source);

	void blockHashRequest(PeerId peer, int blockNumberBegin, int blockNumberEnd, const std::function<void(int, const std::vector<Hash>&)>& callback);
	void blockRequest(PeerId peer, const std::vector<Hash>& hashes);
};
