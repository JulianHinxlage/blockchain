//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "BlockChainConfig.h"
#include "BinaryTree.h"

class BlockChain {
public:
	BlockChainConfig config;
	KeyValueStorage transactionStorage;
	KeyValueStorage blockStorage;
	AccountTree accountTree;
	Hash latestBlock;
	uint64_t blockCount;
	std::vector<Hash> blockList;

	void init(const std::string& directory);

	TransactionHeader getTransactionHeader(const Hash& hash);
	Transaction getTransaction(const Hash& hash);
	BlockHeader getBlockHeader(const Hash& hash);
	Block getBlock(const Hash &hash);

	void addBlock(const Block& block);
	void addTransaction(const Transaction& transaction);
	bool resetTip(const Hash& blockHash);
	bool addBlockToTip(const Hash& blockHash, bool check = true);

	void loadBlockList();
	void saveBlockList();
private:
	std::string directory;
};
