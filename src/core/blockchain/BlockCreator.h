//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "BlockChain.h"

class BlockCreator {
public:
	BlockChain* blockChain;
	void beginBlock(const EccPublicKey &validator, const EccPublicKey &beneficiary, uint32_t slot, uint64_t timestamp);
	Block &endBlock();
	void addTransaction(const Transaction& transaction);
	Transaction createTransaction(const EccPublicKey& sender, const EccPublicKey& recipient, uint32_t transactionNumber, Amount amount, Amount fee = 0, TransactionType type = TransactionType::TRANSFER);

private:
	Block block;
	Amount totalFees;
	AccountTree accountTree;
	ValidatorTree validatorTree;
};
