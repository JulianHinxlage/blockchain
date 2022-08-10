#pragma once

#include "Block.h"
#include "BlockChainState.h"
#include "BlockChainConfig.h"

class BlockChain {
public:
	std::vector<Hash> blocks;
	BlockChainState state;
	BlockChainConfig config;
	class DataBase* db;

	BlockChain();
	int getBlockCount();
	Hash getLatestBlock();

	void addBlock(const Block &block);
};
