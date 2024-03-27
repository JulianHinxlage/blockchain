//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "BlockChain.h"

class BlockCreator {
public:
	BlockChain* blockChain;

	void beginBlock(const EccPublicKey &validator);
	Block &endBlock();
	void addTransaction(const Transaction& transaction);
	Transaction createTransaction(const EccPublicKey &sender, const EccPublicKey &recipient, uint64_t amount, uint64_t fee = 0);

private:
	Block block;
};
