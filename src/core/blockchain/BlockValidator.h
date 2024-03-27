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
	TRANSACTION_NOT_FOUND,
	INVALID_TRANSACTION,
	INVALID_ACCOUNT_ROOT,
};

enum class TransactionError {
	VALID,
	INVALID_VERSION,
	INVALID_PUBLIC_KEY,
	INVALID_SIGNATURE,
	INVALID_BALANCE,
	INVALID_TRANSACTION_NUMBER,
};

const char* blockErrorToString(BlockError error);
const char* transactionErrorToString(TransactionError error);

class BlockValidator {
public:
	BlockChain *blockChain;

	BlockError validateBlock(const Block& block);
	TransactionError validateTransaction(const Transaction& transaction, const BlockHeader& block);
};