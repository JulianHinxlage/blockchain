#pragma once

#include "BlockChain.h"

class BlockCreateor {
public:
	Block block;
	Amount totalFees;
	BlockChain* chain;

	BlockCreateor();
	void reset();
	TransactionError createTransaction(Transaction& transaction, const EccPublicKey& from, const EccPublicKey& to, Amount amount, Amount fee, const EccPrivateKey& key);
	TransactionError addTransaction(const Transaction &transaction);
	BlockError createBlock(const EccPublicKey& beneficiary, const EccPublicKey& validator, const EccPrivateKey& key);
	BlockError validateBlock(const Block &block);
};
