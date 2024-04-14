//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "blockchain/BlockChain.h"
#include "blockchain/Network.h"
#include "blockchain/BlockCreator.h"
#include "blockchain/BlockVerifier.h"

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

private:
	std::map<Hash, Block> pendingBlocks;
	std::map<Hash, Hash> blockByPrev;
	std::map<Hash, Hash> blockByTrasnaction;
	std::mutex verifyMutex;
	bool synchronisationPending = false;


	void onBlock(const Block& block);
	void onTransaction(const Transaction& transaction);

	bool prepareBlock(const Block& block, bool onlyCheck = false);
	bool verifyBlock(const Block& block);
	void onVerifiedBlock(const Block& block);
	bool checkForTip(const Block& block);
};
