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

enum class StorageMode {
	NO_STORAGE,
	BLOCK_HEADERS,
	PRUNED,
	FULL,
};

enum FullNodeState {
	INIT,
	RUNNING,
	VERIFY_CHAIN,
	SYNCHRONISING_FETCH,
	SYNCHRONISING_VERIFY,
};

class FullNode {
public:
	std::function<void(const Block&)> onNewBlock;
	std::function<void(const Transaction&)> onNewTransaction;

	NetworkMode networkMode = NetworkMode::CLIENT;
	StorageMode storageMode = StorageMode::NO_STORAGE;

	BlockChain blockChain;
	Network network;
	BlockCreator creator;
	BlockVerifier verifier;
	
	void init(const std::string& chainDir, const std::string& entryNodeFile);
	void synchronize();
	void synchronizePendingTransactions();
	void verifyChain();
	FullNodeState getState();

private:
	FullNodeState state;
	BlockFetcher fetcher;
	ThreadedQueue<Block> verifyQueue;
	std::map<Hash, Hash> pendingVerifies;
	bool synchronisationPending = false;

	void verify(const Block& block);
};
