//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "blockchain/BlockChain.h"
#include "blockchain/Network.h"
#include "blockchain/BlockCreator.h"
#include "blockchain/BlockVerifier.h"
#include "BlockFetcher.h"

enum class NetworkMode {
	CLIENT,
	MULTI_CLIENT,
	SERVER,
};

enum class VerifyMode {
	NO_VERIFY,
	MULTI_NODE_VOTE,
	CONSENSUS_VERIFY,
	FULL_VERIFY,
};

enum class StorageMode {
	NO_STORAGE,
	BLOCK_HEADERS,
	PRUNED,
	FULL,
};

enum NodeState {
	INIT,
	RUNNING,
	VERIFY_CHAIN,
	SYNCHRONISING_FETCH,
	SYNCHRONISING_VERIFY,
};

class Node {
public:
	std::function<void(const Block&)> onBlockRecived;
	std::function<void(const Transaction&)> onTransactionRecived;

	NetworkMode networkMode = NetworkMode::CLIENT;
	VerifyMode verifyMode = VerifyMode::NO_VERIFY;
	StorageMode storageMode = StorageMode::NO_STORAGE;

	BlockChain blockChain;
	Network network;
	BlockCreator creator;
	BlockVerifier verifier;
	
	void init(const std::string& chainDir, const std::string& entryNodeFile);
	void synchronize();
	void verifyChain();
	NodeState getState();

private:
	NodeState state;
	BlockFetcher fetcher;
	ThreadedQueue<Block> verifyQueue;
	std::map<Hash, Hash> pendingVerifies;
	bool synchronisationPending = false;

	void verify(const Block& block);
};
