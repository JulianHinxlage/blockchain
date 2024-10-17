//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "BlockChain.h"

const char* blockErrorToString(BlockError error);
const char* transactionErrorToString(TransactionError error);

class VerifyContext {
public:
	EccPublicKey beneficiary;
	uint64_t blockNumber;
	uint64_t totalStakeAmount;
	Amount totalFees;
	AccountTree accountTree;
	ValidatorTree validatorTree;
};

class BlockVerifier {
public:
	BlockChain *blockChain;

	//created verification context b ased on a block
	//transactions might be valid in on block and invalid in another
	VerifyContext createContext(const Hash& blockHash);

	//verifies a block header, note that it is assumed that the previous block is valid
	BlockError verifyBlockHeader(const BlockHeader& block, uint64_t unixTime);

	//verifies a block including all transactions, note that it is assumed that the previous block is valid
	BlockError verifyBlock(const Block& block, uint64_t unixTime);
	TransactionError verifyTransaction(const Transaction& transaction);
	TransactionError verifyTransaction(const Transaction& transaction, VerifyContext &context, bool checkTransactionNumber = true);
};
