//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "BlockChainConfig.h"
#include "BinaryTree.h"
#include "Consensus.h"

typedef BinaryTree<uint64_t, EccPublicKey, false> ValidatorTree;

class BlockChain {
public:
	BlockChainConfig config;
	AccountTree accountTree;
	ValidatorTree validatorTree;
	Consensus consensus;

	void init(const std::string& directory);

	Hash getHeadBlock();
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
	
	bool setHeadBlock(const Hash& blockHash);

private:
	std::string directory;

	KeyValueStorage transactionStorage;
	KeyValueStorage blockStorage;
	KeyValueStorage accountTreeStorage;
	KeyValueStorage stakeTreeStorage;

	std::vector<Hash> blockList;
	uint64_t blockListStartOffset;

	void loadBlockList();
	void saveBlockList();
};
