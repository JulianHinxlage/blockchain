#pragma once

#include "type.h"
#include "Block.h"
#include "BlockChainState.h"

class BlockChainConfig {
public:
	BlockChainConfig();
	void initDevnet();

	Hash getGenesisBlockHash();
	const Block& getGenesisBlock();
	void getGenesisState(BlockChainState &state);
	int getVersion();

private:
	uint16_t version;
	Hash genesisBlockHash;
	Block genesisBlock;
};
