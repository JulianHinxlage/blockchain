//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "BlockVerifier.h"
#include "util/log.h"

BlockError BlockVerifier::verifyBlockHeader(const BlockHeader& block, uint64_t unixTime) {
	if (block.caclulateHash() == blockChain->config.genesisBlockHash) {
		return BlockError::VALID;
	}

	if (block.version != blockChain->config.blockVersion) {
		return BlockError::INVALID_VERSION;
	}
	if (!eccValidPublicKey(block.beneficiary)) {
		return BlockError::INVALID_PUBLIC_KEY;
	}
	if (!eccValidPublicKey(block.validator)) {
		return BlockError::INVALID_PUBLIC_KEY;
	}
	if (!block.verifySignature()) {
		return BlockError::INVALID_SIGNATURE;
	}

	Block prev = blockChain->getBlock(block.previousBlockHash);
	if (prev.header.caclulateHash() != block.previousBlockHash) {
		return BlockError::PREVIOUS_BLOCK_NOT_FOUND;
	}
	if (block.blockNumber != prev.header.blockNumber + 1) {
		return BlockError::INVALID_BLOCK_NUMBER;
	}

	blockChain->validatorTree.reset(prev.header.validatorTreeRoot);
	EccPublicKey selectedValidator = blockChain->consensus.selectNextValidator(prev.header, block.slot);
	if (block.validator != selectedValidator && selectedValidator != EccPublicKey(0)) {
		return BlockError::INVALID_VALIDATOR;
	}

	if (block.blockNumber >= 2) {
		uint64_t epochBeginTime = ((uint64_t)(prev.header.timestamp / blockChain->config.slotTime)) * blockChain->config.slotTime + blockChain->config.slotTime;
		if (block.timestamp < epochBeginTime) {
			return BlockError::INVALID_TIMESTAMP;
		}
		if (block.slot != (block.timestamp - epochBeginTime) / blockChain->config.slotTime) {
			return BlockError::INVALID_TIMESTAMP;
		}
		if (block.timestamp <= prev.header.timestamp) {
			return BlockError::INVALID_TIMESTAMP;
		}
	}
	if (block.timestamp > unixTime) {
		return BlockError::INVALID_FUTURE_BLOCK;
	}

	if (block.seed != sha256((char*)&prev.header.seed, sizeof(prev.header.seed))) {
		return BlockError::INVALID_SEED;
	}

	return BlockError::VALID;
}

BlockError BlockVerifier::verifyBlock(const Block& block, uint64_t unixTime) {
	BlockError headerError = verifyBlockHeader(block.header, unixTime);
	if (headerError != BlockError::VALID) {
		return headerError;
	}
	if (block.header.caclulateHash() == blockChain->config.genesisBlockHash) {
		return BlockError::VALID;
	}

	if (block.header.transactionCount != block.transactionTree.transactionHashes.size()) {
		return BlockError::INVALID_TRANSACTION_COUNT;
	}
	if (block.header.transactionTreeRoot != block.transactionTree.calculateRoot()) {
		return BlockError::INVALID_TRANSACTION_ROOT;
	}

	Block prev = blockChain->getBlock(block.header.previousBlockHash);
	blockChain->accountTree.reset(prev.header.accountTreeRoot);
	VerifyContext context;
	context.beneficiary = block.header.beneficiary;
	context.blockNumber = block.header.blockNumber;
	context.totalStakeAmount = prev.header.totalStakeAmount;

	for (auto &h : block.transactionTree.transactionHashes) {
		Transaction tx = blockChain->getTransaction(h);
		if (tx.header.caclulateHash() != h) {
			return BlockError::TRANSACTION_NOT_FOUND;
		}

		TransactionError error = verifyTransaction(tx, true, &context);
		if (error != TransactionError::VALID) {
			log(LogLevel::DEBUG, "Block Validator", "invalid transaction: %s", transactionErrorToString(error));
			return BlockError::INVALID_TRANSACTION;
		}
	}

	if (blockChain->accountTree.getRoot() != block.header.accountTreeRoot) {
		return BlockError::INVALID_ACCOUNT_TREE_ROOT;
	}
	if (blockChain->validatorTree.getRoot() != block.header.validatorTreeRoot) {
		return BlockError::INVALID_STAKE_TREE_ROOT;
	}
	if (block.header.totalStakeAmount != context.totalStakeAmount) {
		return BlockError::INVALID_TOTAL_STAKE_AMOUNT;
	}

	return BlockError::VALID;
}

bool amountSub(Amount& out, Amount lhs, Amount rhs) {
	if (rhs > lhs) {
		return false;
	}
	if (lhs - rhs > lhs) {
		return false;
	}
	out = lhs - rhs;
	return true;
}

bool amountAdd(Amount &out, Amount lhs, Amount rhs) {
	if (lhs + rhs < lhs) {
		return false;
	}
	if (lhs + rhs < rhs) {
		return false;
	}
	out = lhs + rhs;
	return true;
}

TransactionError BlockVerifier::verifyTransaction(const Transaction& transaction, bool updateAccounts, VerifyContext* context) {
	if (transaction.header.version != blockChain->config.transactionVersion) {
		return TransactionError::INVALID_VERSION;
	}
	if (!eccValidPublicKey(transaction.header.sender)) {
		return TransactionError::INVALID_PUBLIC_KEY;
	}
	if (!eccValidPublicKey(transaction.header.recipient)) {
		return TransactionError::INVALID_PUBLIC_KEY;
	}

	if (transaction.header.type == TransactionType::TRANSFER) {
		Account senderAccount = blockChain->accountTree.get(transaction.header.sender);
		if (transaction.header.transactionNumber != senderAccount.transactionCount) {
			return TransactionError::INVALID_TRANSACTION_NUMBER;
		}
		senderAccount.transactionCount++;
		if (!amountSub(senderAccount.balance, senderAccount.balance, transaction.header.amount)) {
			return TransactionError::INVALID_BALANCE;
		}
		if (!amountSub(senderAccount.balance, senderAccount.balance, transaction.header.fee)) {
			return TransactionError::INVALID_BALANCE;
		}
		if (updateAccounts) {
			blockChain->accountTree.set(transaction.header.sender, senderAccount);
		}

		Account recipientAccount = blockChain->accountTree.get(transaction.header.recipient);
		if (!amountAdd(recipientAccount.balance, recipientAccount.balance, transaction.header.amount)) {
			return TransactionError::INVALID_BALANCE;
		}
		if (updateAccounts) {
			blockChain->accountTree.set(transaction.header.recipient, recipientAccount);
		}
	}
	else if (transaction.header.type == TransactionType::STAKE) {
		Account senderAccount = blockChain->accountTree.get(transaction.header.sender);
		if (transaction.header.transactionNumber != senderAccount.transactionCount) {
			return TransactionError::INVALID_TRANSACTION_NUMBER;
		}
		senderAccount.transactionCount++;
		if (!amountSub(senderAccount.balance, senderAccount.balance, transaction.header.amount)) {
			return TransactionError::INVALID_BALANCE;
		}
		if (!amountSub(senderAccount.balance, senderAccount.balance, transaction.header.fee)) {
			return TransactionError::INVALID_BALANCE;
		}
		if (updateAccounts) {
			blockChain->accountTree.set(transaction.header.sender, senderAccount);
		}

		Account recipientAccount = blockChain->accountTree.get(transaction.header.recipient);
		if (recipientAccount.stakeOwner != EccPublicKey(0) && recipientAccount.stakeOwner != transaction.header.sender) {
			return TransactionError::INVALID_STAKE_OWNER;
		}
		recipientAccount.stakeOwner = transaction.header.sender;
		if (context) {
			if (recipientAccount.stakeAmount == 0) {
				recipientAccount.stakeBlockNumber = context->blockNumber;
				recipientAccount.validatorNumber = context->totalStakeAmount / blockChain->config.minimumStakeAmount;
				blockChain->validatorTree.set(recipientAccount.validatorNumber, transaction.header.recipient);
			}
		}

		if (!amountAdd(recipientAccount.stakeAmount, recipientAccount.stakeAmount, transaction.header.amount)) {
			return TransactionError::INVALID_BALANCE;
		}

		if (blockChain->config.uniformStakeSize) {
			if (recipientAccount.stakeAmount != blockChain->config.minimumStakeAmount) {
				return TransactionError::INVALID_STAKE_AMOUNT;
			}
		}
		else {
			if (recipientAccount.stakeAmount < blockChain->config.minimumStakeAmount) {
				return TransactionError::INVALID_STAKE_AMOUNT;
			}
		}

		if (updateAccounts) {
			blockChain->accountTree.set(transaction.header.recipient, recipientAccount);
		}

		if (context) {
			if (!amountAdd(context->totalStakeAmount, context->totalStakeAmount, transaction.header.amount)) {
				return TransactionError::INVALID_BALANCE;
			}
		}
	}
	else if(transaction.header.type == TransactionType::UNSTAKE) {
		Account recipientAccount = blockChain->accountTree.get(transaction.header.recipient);
		if (!amountSub(recipientAccount.stakeAmount, recipientAccount.stakeAmount, transaction.header.amount)) {
			return TransactionError::INVALID_BALANCE;
		}
		if (recipientAccount.stakeOwner != transaction.header.sender) {
			return TransactionError::INVALID_STAKE_OWNER;
		}
		if (blockChain->config.uniformStakeSize) {
			if (recipientAccount.stakeAmount != 0) {
				return TransactionError::INVALID_STAKE_AMOUNT;
			}
		}
		else {
			if (recipientAccount.stakeAmount != 0 && recipientAccount.stakeAmount < blockChain->config.minimumStakeAmount) {
				return TransactionError::INVALID_STAKE_AMOUNT;
			}
		}

		if (recipientAccount.stakeAmount == 0) {
			recipientAccount.stakeOwner = EccPublicKey(0);
			recipientAccount.stakeBlockNumber = 0;
			
			if (updateAccounts && context) {
				EccPublicKey lastAddress = blockChain->validatorTree.get((context->totalStakeAmount / blockChain->config.minimumStakeAmount)-1);
				Account lastAccount = blockChain->accountTree.get(lastAddress);
				blockChain->validatorTree.set(recipientAccount.validatorNumber, lastAddress);
				blockChain->validatorTree.set(lastAccount.validatorNumber, EccPublicKey(0));
				lastAccount.validatorNumber = recipientAccount.validatorNumber;
				blockChain->accountTree.set(lastAddress, lastAccount);
			}

			recipientAccount.validatorNumber = 0;

		}
		if (updateAccounts) {
			blockChain->accountTree.set(transaction.header.recipient, recipientAccount);
		}

		Account senderAccount = blockChain->accountTree.get(transaction.header.sender);
		if (transaction.header.transactionNumber != senderAccount.transactionCount) {
			return TransactionError::INVALID_TRANSACTION_NUMBER;
		}
		senderAccount.transactionCount++;
		if (!amountAdd(senderAccount.balance, senderAccount.balance, transaction.header.amount)) {
			return TransactionError::INVALID_BALANCE;
		}
		if (!amountSub(senderAccount.balance, senderAccount.balance, transaction.header.fee)) {
			return TransactionError::INVALID_BALANCE;
		}
		if (updateAccounts) {
			blockChain->accountTree.set(transaction.header.sender, senderAccount);
		}

		if (context) {
			if (!amountSub(context->totalStakeAmount, context->totalStakeAmount, transaction.header.amount)) {
				return TransactionError::INVALID_BALANCE;
			}
		}
	}
	else {
		return TransactionError::INVALID_TYPE;
	}

	if (updateAccounts && context) {
		Account beneficiaryAccount = blockChain->accountTree.get(context->beneficiary);
		if (!amountAdd(beneficiaryAccount.balance, beneficiaryAccount.balance, transaction.header.fee)) {
			return TransactionError::INVALID_BALANCE;
		}
		blockChain->accountTree.set(context->beneficiary, beneficiaryAccount);
	}

	if (!transaction.header.verifySignature()) {
		return TransactionError::INVALID_SIGNATURE;
	}
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
	case BlockError::INVALID_VALIDATOR:
		return "INVALID_VALIDATOR";
	case BlockError::INVALID_TIMESTAMP:
		return "INVALID_TIMESTAMP";
	case BlockError::INVALID_FUTURE_BLOCK:
		return "INVALID_FUTURE_BLOCK";
	case BlockError::INVALID_SEED:
		return "INVALID_SEED";
	case BlockError::TRANSACTION_NOT_FOUND:
		return "TRANSACTION_NOT_FOUND";
	case BlockError::INVALID_TRANSACTION:
		return "INVALID_TRANSACTION";
	case BlockError::INVALID_ACCOUNT_TREE_ROOT:
		return "INVALID_ACCOUNT_TREE_ROOT";
	case BlockError::INVALID_STAKE_TREE_ROOT:
		return "INVALID_STAKE_TREE_ROOT";
	case BlockError::INVALID_TOTAL_STAKE_AMOUNT:
		return "INVALID_TOTAL_STAKE_AMOUNT";
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
	case TransactionError::INVALID_TYPE:
		return "INVALID_TYPE";
	case TransactionError::INVALID_PUBLIC_KEY:
		return "INVALID_PUBLIC_KEY";
	case TransactionError::INVALID_SIGNATURE:
		return "INVALID_SIGNATURE";
	case TransactionError::INVALID_BALANCE:
		return "INVALID_BALANCE";
	case TransactionError::INVALID_TRANSACTION_NUMBER:
		return "INVALID_TRANSACTION_NUMBER";
	case TransactionError::INVALID_STAKE_AMOUNT:
		return "INVALID_STAKE_AMOUNT";
	case TransactionError::INVALID_STAKE_OWNER:
		return "INVALID_STAKE_OWNER";
	default:
		return "UNKNOWN";
	}
}
