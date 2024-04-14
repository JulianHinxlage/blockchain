//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "network/peer/PeerNetwork.h"
#include "Block.h"
#include "Account.h"
#include "util/Serializer.h"
#include <map>
#include <condition_variable>

enum class NetworkOpcode {
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
	TRANSACTION_BROADCAST,
	REQUEST_ERROR,
	TIMEOUT,
};

enum class NetworkState {
	UNKNOWN,
	DISCONNECTED,
	JOINING,
	CONNECTED,
};

typedef Blob<128> RequestId;

class Request {
public:
	RequestId id;
	uint32_t createTime;
	std::function<void(NetworkOpcode, Serializer &)> callback;
};

class RequestContenxt {
public:
	std::map<RequestId, Request> requests;
	uint32_t timeoutTime = 10;
	std::thread* thread = nullptr;
	std::atomic_bool running;
	std::condition_variable cv;
	std::mutex mutex;

	void add(RequestId id, const std::function<void(NetworkOpcode, Serializer&)>& callback);
	bool onRequest(RequestId id, NetworkOpcode opcode, Serializer& serial);
	void start();
	void stop();
};

class Network {
public:
	class BlockChain* blockChain;
	std::function<void(const Block&)> onBlockRecived;
	std::function<void(const Transaction&)> onTransactionRecived;
	std::function<void(NetworkState)> onStateChanged;

	~Network();
	void init(PeerType type, const std::string& entryNodeFile = "");
	void connect(const std::string& address, uint16_t port);
	void disconnect();
	void sendBlock(const Block& block);
	void sendTransaction(const Transaction& transaction);
	NetworkState getState();

	void getBlockHashes(int begin, int end, const std::function<void(int, int, const std::vector<Hash>&, PeerId)>& callback, PeerId peer = PeerId(0));
	void getBlocks(const std::vector<Hash> &blockHashes, const std::function<void(const std::vector<Block>&, PeerId)>& callback, PeerId peer = PeerId(0));
	void getTransactions(const std::vector<Hash>& transactionHashs, const std::function<void(const std::vector<Transaction>&, PeerId)>& callback, PeerId peer = PeerId(0));
	void getAccount(Hash treeRoot, EccPublicKey address, const std::function<void(const Account&, PeerId)>& callback, PeerId peer = PeerId(0));

private:
	NetworkState state = NetworkState::UNKNOWN;
	PeerType type = PeerType::UNKNOWN;
	RequestContenxt requestContext;
	PeerNetwork network;
	std::thread* connectThread = nullptr;

	void changeState(NetworkState newState);
	void onMessage(const std::string &msg, PeerId source);
};
