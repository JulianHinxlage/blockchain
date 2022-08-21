#pragma once

#include "BlockChain.h"
#include "DataBase.h"

class BlockCreateor {
public:
	Block block;
	Amount totalFees;
	BlockChain* chain;

	BlockCreateor();
	void reset();
	TransactionError createTransaction(Transaction& transaction, const EccPublicKey& from,
		const EccPublicKey& to, Amount amount, Amount fee, const EccPrivateKey& key, const Account*accountOverride = nullptr);
	TransactionError addTransaction(const Transaction &transaction);
	BlockError createBlock(const EccPublicKey& beneficiary, const EccPublicKey& validator, const EccPrivateKey& key, Block *blockOutput = nullptr);
	BlockError validateBlock(const Block &block);

private:
	void processTransaction(const Transaction& transaction, BlockChainState& state);
	BlockError processBlock(const Block& block, BlockChainState& state, DataBase *db);
};
