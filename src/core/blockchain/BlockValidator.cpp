//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "BlockValidator.h"

BlockError BlockValidator::validateBlock(const Block& block) {
	if (block.header.caclulateHash() == blockChain->config.genesisBlockHash) {
		return BlockError::VALID;
	}

	if (block.header.version != blockChain->config.blockVersion) {
		return BlockError::INVALID_VERSION;
	}
	if (block.header.transactionCount != block.transactionTree.transactionHashes.size()) {
		return BlockError::INVALID_TRANSACTION_COUNT;
	}
	if (block.header.transactionTreeRoot != block.transactionTree.calculateRoot()) {
		return BlockError::INVALID_TRANSACTION_ROOT;
	}
	if (!eccValidPublicKey(block.header.beneficiary)) {
		return BlockError::INVALID_PUBLIC_KEY;
	}
	if (!eccValidPublicKey(block.header.validator)) {
		return BlockError::INVALID_PUBLIC_KEY;
	}
	if (!block.header.verifySignature()) {
		return BlockError::INVALID_SIGNATURE;
	}

	Block prev = blockChain->getBlock(block.header.previousBlockHash);
	if (prev.header.caclulateHash() != block.header.previousBlockHash) {
		return BlockError::PREVIOUS_BLOCK_NOT_FOUND;
	}
	if (block.header.blockNumber != prev.header.blockNumber + 1) {
		return BlockError::INVALID_BLOCK_NUMBER;
	}


	blockChain->accountTree.reset(prev.header.accountTreeRoot);
	for (auto &h : block.transactionTree.transactionHashes) {
		Transaction tx = blockChain->getTransaction(h);
		if (tx.header.caclulateHash() != h) {
			return BlockError::TRANSACTION_NOT_FOUND;
		}

		TransactionError error = validateTransaction(tx, block.header);
		if (error != TransactionError::VALID) {
			return BlockError::INVALID_TRANSACTION;
		}
	}

	if (blockChain->accountTree.getRoot() != block.header.accountTreeRoot) {
		return BlockError::INVALID_ACCOUNT_ROOT;
	}

	return BlockError::VALID;
}

TransactionError BlockValidator::validateTransaction(const Transaction& transaction, const BlockHeader &block) {
	if (transaction.header.version != blockChain->config.transactionVersion) {
		return TransactionError::INVALID_VERSION;
	}
	if (!eccValidPublicKey(transaction.header.sender)) {
		return TransactionError::INVALID_PUBLIC_KEY;
	}
	if (!eccValidPublicKey(transaction.header.recipient)) {
		return TransactionError::INVALID_PUBLIC_KEY;
	}
	if (!transaction.header.verifySignature()) {
		return TransactionError::INVALID_SIGNATURE;
	}

	Account sender = blockChain->accountTree.get(transaction.header.sender);
	if (transaction.header.amount > sender.balance) {
		return TransactionError::INVALID_BALANCE;
	}
	if (transaction.header.fee > sender.balance) {
		return TransactionError::INVALID_BALANCE;
	}
	if (transaction.header.amount + transaction.header.fee > sender.balance) {
		return TransactionError::INVALID_BALANCE;
	}
	if (transaction.header.amount + transaction.header.fee < transaction.header.amount) {
		return TransactionError::INVALID_BALANCE;
	}
	if (transaction.header.amount + transaction.header.fee < transaction.header.fee) {
		return TransactionError::INVALID_BALANCE;
	}
	if (transaction.header.transactionNumber != sender.transactionCount) {
		return TransactionError::INVALID_TRANSACTION_NUMBER;
	}

	sender.balance -= transaction.header.amount + transaction.header.fee;
	sender.transactionCount++;
	blockChain->accountTree.set(transaction.header.sender, sender);

	Account recipient = blockChain->accountTree.get(transaction.header.recipient);
	if (transaction.header.amount + recipient.balance < recipient.balance) {
		return TransactionError::INVALID_BALANCE;
	}
	recipient.balance += transaction.header.amount;
	blockChain->accountTree.set(transaction.header.recipient, recipient);

	Account beneficiary = blockChain->accountTree.get(block.beneficiary);
	if (transaction.header.fee + beneficiary.balance < beneficiary.balance) {
		return TransactionError::INVALID_BALANCE;
	}
	beneficiary.balance += transaction.header.fee;
	blockChain->accountTree.set(block.beneficiary, beneficiary);

	return TransactionError::VALID;
}

const char* blockErrorToString(BlockError error) {
	switch (error)
	{
	case BlockError::VALID:
		return "VALID";
	case BlockError::INVALID_VERSION:
		return "INVALID_VERSION";
	case BlockError::INVALID_TRANSACTION_COUNT:
		return "INVALID_TRANSACTION_COUNT";
	case BlockError::INVALID_TRANSACTION_ROOT:
		return "INVALID_TRANSACTION_ROOT";
	case BlockError::INVALID_PUBLIC_KEY:
		return "INVALID_PUBLIC_KEY";
	case BlockError::INVALID_SIGNATURE:
		return "INVALID_SIGNATURE";
	case BlockError::PREVIOUS_BLOCK_NOT_FOUND:
		return "PREVIOUS_BLOCK_NOT_FOUND";
	case BlockError::INVALID_BLOCK_NUMBER:
		return "INVALID_BLOCK_NUMBER";
	case BlockError::TRANSACTION_NOT_FOUND:
		return "TRANSACTION_NOT_FOUND";
	case BlockError::INVALID_TRANSACTION:
		return "INVALID_TRANSACTION";
	case BlockError::INVALID_ACCOUNT_ROOT:
		return "INVALID_ACCOUNT_ROOT";
	default:
		return "UNKNOWN";
	}
}

const char* transactionErrorToString(TransactionError error) {
	switch (error)
	{
	case TransactionError::VALID:
		return "VALID";
	case TransactionError::INVALID_VERSION:
		return "INVALID_VERSION";
	case TransactionError::INVALID_PUBLIC_KEY:
		return "INVALID_PUBLIC_KEY";
	case TransactionError::INVALID_SIGNATURE:
		return "INVALID_SIGNATURE";
	case TransactionError::INVALID_BALANCE:
		return "INVALID_BALANCE";
	case TransactionError::INVALID_TRANSACTION_NUMBER:
		return "INVALID_TRANSACTION_NUMBER";
	default:
		return "UNKNOWN";
	}
}
