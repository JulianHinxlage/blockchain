//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "BlockCreator.h"

void BlockCreator::beginBlock(const EccPublicKey& validator, const EccPublicKey& beneficiary, uint32_t slot, uint64_t timestamp) {
	block = Block();
	BlockHeader prev = blockChain->getBlockHeader(blockChain->getHeadBlock());
	block.header.version = blockChain->config.blockVersion;
	block.header.timestamp = timestamp;
	block.header.previousBlockHash = blockChain->getHeadBlock();
	block.header.blockNumber = prev.blockNumber + 1;
	block.header.totalStakeAmount = prev.totalStakeAmount;
	block.header.validator = validator;
	block.header.beneficiary = beneficiary;
	block.header.slot = slot;
	block.header.seed = sha256((char*)&prev.seed, sizeof(prev.seed));
	totalFees = 0;

	accountTree = blockChain->getAccountTree(prev.accountTreeRoot);
	validatorTree = blockChain->getValidatorTree(prev.validatorTreeRoot);
}

void BlockCreator::addTransaction(const Transaction& transaction) {
	block.transactionTree.transactionHashes.push_back(transaction.transactionHash);

	if (transaction.header.type == TransactionType::TRANSFER) {
		Account sender = accountTree.get(transaction.header.sender);
		sender.balance -= transaction.header.amount;
		sender.balance -= transaction.header.fee;
		sender.transactionCount++;
		accountTree.set(transaction.header.sender, sender);
	
		Account recipient = accountTree.get(transaction.header.recipient);
		recipient.balance += transaction.header.amount;
		accountTree.set(transaction.header.recipient, recipient);
	}
	else if (transaction.header.type == TransactionType::STAKE) {
		Account sender = accountTree.get(transaction.header.sender);
		sender.balance -= transaction.header.amount + transaction.header.fee;
		sender.transactionCount++;
		accountTree.set(transaction.header.sender, sender);

		Account recipient = accountTree.get(transaction.header.recipient);
		if (recipient.stakeAmount == 0) {
			recipient.stakeBlockNumber = block.header.blockNumber;
			recipient.validatorNumber = block.header.totalStakeAmount / blockChain->config.minimumStakeAmount;
			validatorTree.set(recipient.validatorNumber, transaction.header.recipient);
		}
		recipient.stakeAmount += transaction.header.amount;
		recipient.stakeOwner = transaction.header.sender;
		accountTree.set(transaction.header.recipient, recipient);

		block.header.totalStakeAmount += transaction.header.amount;
	}
	else if (transaction.header.type == TransactionType::UNSTAKE) {
		Account sender = accountTree.get(transaction.header.sender);
		sender.balance += transaction.header.amount;
		sender.balance -= transaction.header.fee;
		sender.transactionCount++;
		accountTree.set(transaction.header.sender, sender);

		Account recipient = accountTree.get(transaction.header.recipient);
		recipient.stakeAmount -= transaction.header.amount;
		if (recipient.stakeAmount == 0) {
			recipient.stakeOwner = EccPublicKey(0);
			recipient.stakeBlockNumber = 0;

			EccPublicKey lastAddress = validatorTree.get((block.header.totalStakeAmount / blockChain->config.minimumStakeAmount) - 1);
			Account lastAccount = accountTree.get(lastAddress);
			validatorTree.set(recipient.validatorNumber, lastAddress);
			validatorTree.set(lastAccount.validatorNumber, EccPublicKey(0));
			lastAccount.validatorNumber = recipient.validatorNumber;
			accountTree.set(lastAddress, lastAccount);

			recipient.validatorNumber = 0;
		}
		accountTree.set(transaction.header.recipient, recipient);

		block.header.totalStakeAmount -= transaction.header.amount;
	}

	totalFees += transaction.header.fee;
	blockChain->addTransaction(transaction);
}

Block& BlockCreator::endBlock() {
	Account beneficiary = accountTree.get(block.header.beneficiary);
	beneficiary.balance += totalFees;
	accountTree.set(block.header.beneficiary, beneficiary);
	totalFees = 0;

	block.header.transactionCount = block.transactionTree.transactionHashes.size();
	block.header.transactionTreeRoot = block.transactionTree.calculateRoot();
	block.header.accountTreeRoot = accountTree.getRoot();
	block.header.validatorTreeRoot = validatorTree.getRoot();
	return block;
}

Transaction BlockCreator::createTransaction(const EccPublicKey& sender, const EccPublicKey& recipient, uint32_t transactionNumber, Amount amount, Amount fee, TransactionType type) {
	Transaction tx;
	tx.header.version = blockChain->config.transactionVersion;
	tx.header.type = type;
	tx.header.timestamp = time(nullptr);
	tx.header.dataHash = 0;
	tx.header.sender = sender;
	tx.header.recipient = recipient;
	tx.header.amount = amount;
	tx.header.fee = fee;
	tx.header.transactionNumber = transactionNumber;
	return tx;
}
