//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "BlockChain.h"

enum class BlockError {
	VALID,
	INVALID_VERSION,
	INVALID_TRANSACTION_COUNT,
	INVALID_TRANSACTION_ROOT,
	INVALID_PUBLIC_KEY,
	INVALID_SIGNATURE,
	PREVIOUS_BLOCK_NOT_FOUND,
	INVALID_BLOCK_NUMBER,
	INVALID_VALIDATOR,
	INVALID_TIMESTAMP,
	INVALID_FUTURE_BLOCK,
	INVALID_SEED,
	TRANSACTION_NOT_FOUND,
	INVALID_TRANSACTION,
	INVALID_ACCOUNT_TREE_ROOT,
	INVALID_STAKE_TREE_ROOT,
	INVALID_TOTAL_STAKE_AMOUNT,
};

enum class TransactionError {
	VALID,
	INVALID_VERSION,
	INVALID_TYPE,
	INVALID_PUBLIC_KEY,
	INVALID_SIGNATURE,
	INVALID_BALANCE,
	INVALID_TRANSACTION_NUMBER,
	INVALID_STAKE_AMOUNT,
	INVALID_STAKE_OWNER,
};

const char* blockErrorToString(BlockError error);
const char* transactionErrorToString(TransactionError error);

class VerifyContext {
public:
	EccPublicKey beneficiary;
	uint64_t blockNumber;
	uint64_t totalStakeAmount;
};

class BlockVerifier {
public:
	BlockChain *blockChain;

	BlockError verifyBlockHeader(const BlockHeader& block, uint64_t unixTime);
	BlockError verifyBlock(const Block& block, uint64_t unixTime);
	TransactionError verifyTransaction(const Transaction& transaction, bool updateAccounts = false, VerifyContext *context = nullptr);
};
