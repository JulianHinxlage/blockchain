#pragma once

#include "type.h"
#include "Block.h"
#include "BlockChainState.h"

class BlockChainConfig {
public:
	uint16_t version;
	Hash genesisBlockHash;

	BlockChainConfig();
	void createGenesis(Block& block, BlockChainState& state);
};