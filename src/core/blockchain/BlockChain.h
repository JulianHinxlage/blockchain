//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "BlockChainConfig.h"
#include "BinaryTree.h"
#include "Consensus.h"
#include <map>
#include <set>

typedef BinaryTree<uint64_t, EccPublicKey, false> ValidatorTree;

class BlockMetaData {
public:
	BlockError lastCheck;
	uint64_t received;
};

class BlockChain {
public:
	BlockChainConfig config;
	Consensus consensus;

	void init(const std::string& directory);

	int getBlockCount();
	Hash getHeadBlock();
	bool setHeadBlock(const Hash& blockHash);
	
	Hash getBlockHash(int blockNumber);
	AccountTree getAccountTree(const Hash& root);
	AccountTree getAccountTree();
	ValidatorTree getValidatorTree(const Hash& root);

	TransactionHeader getTransactionHeader(const Hash& hash);
	Transaction getTransaction(const Hash& hash);
	BlockHeader getBlockHeader(const Hash& hash);
	Block getBlock(const Hash &hash);
	void removeBlock(const Hash &hash);

	bool hasBlock(const Hash& hash);
	bool hasTransaction(const Hash& hash);

	void addBlock(const Block& block);
	void addTransaction(const Transaction& transaction);

	BlockMetaData getMetaData(const Hash& blockHash);
	void setMetaData(const Hash& blockHash, BlockMetaData data);

	void addPendingTransaction(const Hash &transactionHash);
	void removePendingTransaction(const Hash& transactionHash);
	const std::set<Hash>& getPendingTransactions();


private:
	std::string directory;

	KeyValueStorage transactionStorage;
	KeyValueStorage blockStorage;
	KeyValueStorage accountTreeStorage;
	KeyValueStorage validatorTreeStorage;
	AccountTree accountTree;
	ValidatorTree validatorTree;
	std::map<Hash, BlockMetaData> metaData;
	std::set<Hash> pendingTransactions;

	std::vector<Hash> blockList;
	uint64_t blockListStartOffset;

	void loadBlockList();
	void saveBlockList();
	void loadMetaData();
	void saveMetaData();
	void savePendingTransactions();
	void loadPendingTransactions();
};
