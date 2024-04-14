//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "BlockChainConfig.h"
#include "BinaryTree.h"

class BlockChain {
public:
	BlockChainConfig config;
	AccountTree accountTree;

	void init(const std::string& directory);

	Hash getLatestBlock();
	int getBlockCount();
	Hash getBlockHash(int blockNumber);


	TransactionHeader getTransactionHeader(const Hash& hash);
	Transaction getTransaction(const Hash& hash);
	BlockHeader getBlockHeader(const Hash& hash);
	Block getBlock(const Hash &hash);

	bool hasBlock(const Hash& hash);
	bool hasTransaction(const Hash& hash);

	void addBlock(const Block& block);
	void addTransaction(const Transaction& transaction);
	
	bool resetTip(const Hash& blockHash);
	bool addBlockToTip(const Hash& blockHash, bool check = true);

private:
	std::string directory;

	KeyValueStorage transactionStorage;
	KeyValueStorage blockStorage;
	KeyValueStorage accountTreeStorage;

	Hash latestBlock;
	uint64_t blockCount;
	std::vector<Hash> blockList;

	void loadBlockList();
	void saveBlockList();
};
