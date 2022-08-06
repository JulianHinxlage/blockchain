#pragma once

#include "BlockChain.h"
#include <string>
#include <unordered_map>

class DataBase {
public:
	DataBase();
	void init(const std::string& path, BlockChain *chain);
	const std::string& getDataPath();

	bool getTransaction(Hash transactionHash, Transaction &transaction);
	bool getFullTransaction(Hash transactionHash, FullTransaction &transaction);
	bool getBlock(Hash blockHash, Block& block);
	bool getFullBlock(Hash blockHash, FullBlock& block);

	void addTransaction(FullTransaction& transaction);
	void addBlock(FullBlock& block);
	void save();

private:
	std::string dataPath;
	BlockChain* chain;

	std::unordered_map<Hash, FullBlock> blocks;
	std::unordered_map<Hash, FullTransaction> transactions;

	void loadChain();
	void saveChain();
	void loadBlocks();
	void saveBlocks();
	void loadTransactions();
	void saveTransactions();
	void loadState();
	void saveState();
};
