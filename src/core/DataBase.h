#pragma once

#include "BlockChain.h"
#include "CachedKeyValueStore.h"
#include <string>
#include <unordered_map>

class DataBase {
public:
	DataBase();
	void init(const std::string& path, BlockChain *chain);
	const std::string& getDataPath();

	bool getTransactionHeader(Hash transactionHash, TransactionHeader &transaction);
	bool getTransaction(Hash transactionHash, Transaction &transaction);
	bool getBlockHeader(Hash blockHash, BlockHeader& block);
	bool getBlock(Hash blockHash, Block& block);

	void addTransaction(const Transaction& transaction);
	void addBlock(const Block& block);
	void load();
	void save();

private:
	std::string dataPath;
	BlockChain* chain;

	CachedKeyValueStore blockStore;
	CachedKeyValueStore transactionStore;

	void loadChain();
	void saveChain();
};
