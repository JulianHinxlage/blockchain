//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "BlockChain.h"

class BlockCreator {
public:
	BlockChain* blockChain;

	void beginBlock(const EccPublicKey &validator, uint32_t slot, uint64_t timestamp);
	Block &endBlock();
	void addTransaction(const Transaction& transaction);
	Transaction createTransaction(const EccPublicKey& sender, const EccPublicKey& recipient, uint64_t amount, uint64_t fee = 0, TransactionType type = TransactionType::TRANSFER);

private:
	Block block;
};
