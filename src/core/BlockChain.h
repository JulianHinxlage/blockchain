#pragma once

#include "Block.h"
#include "BlockChainState.h"

class BlockChain {
public:
	std::vector<Hash> blocks;
	BlockChainState state;
	class DataBase* db;

	BlockChain();
	int getBlockCount();
	Hash getLatest();
};
