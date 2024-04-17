//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "BlockCreator.h"

void BlockCreator::beginBlock(const EccPublicKey& validator, uint32_t slot, uint64_t timestamp) {
	block = Block();
	BlockHeader prev = blockChain->getBlockHeader(blockChain->getHeadBlock());
	block.header.version = blockChain->config.blockVersion;
	block.header.timestamp = timestamp;
	block.header.previousBlockHash = blockChain->getHeadBlock();
	block.header.blockNumber = prev.blockNumber + 1;
	block.header.totalStakeAmount = prev.totalStakeAmount;
	block.header.validator = validator;
	block.header.beneficiary = validator;
	block.header.slot = slot;
	block.header.seed = sha256((char*)&prev.seed, sizeof(prev.seed));
	blockChain->accountTree.reset(prev.accountTreeRoot);
	blockChain->validatorTree.reset(prev.validatorTreeRoot);
}

void BlockCreator::addTransaction(const Transaction& transaction) {
	block.transactionTree.transactionHashes.push_back(transaction.transactionHash);

	if (transaction.header.type == TransactionType::TRANSFER) {
		Account sender = blockChain->accountTree.get(transaction.header.sender);
		sender.balance -= transaction.header.amount;
		sender.balance -= transaction.header.fee;
		sender.transactionCount++;
		blockChain->accountTree.set(transaction.header.sender, sender);
	
		Account recipient = blockChain->accountTree.get(transaction.header.recipient);
		recipient.balance += transaction.header.amount;
		blockChain->accountTree.set(transaction.header.recipient, recipient);
	}
	else if (transaction.header.type == TransactionType::STAKE) {
		Account sender = blockChain->accountTree.get(transaction.header.sender);
		sender.balance -= transaction.header.amount + transaction.header.fee;
		sender.transactionCount++;
		blockChain->accountTree.set(transaction.header.sender, sender);

		Account recipient = blockChain->accountTree.get(transaction.header.recipient);
		if (recipient.stakeAmount == 0) {
			recipient.stakeBlockNumber = block.header.blockNumber;
			recipient.validatorNumber = block.header.totalStakeAmount / blockChain->config.minimumStakeAmount;
			blockChain->validatorTree.set(recipient.validatorNumber, transaction.header.recipient);
		}
		recipient.stakeAmount += transaction.header.amount;
		recipient.stakeOwner = transaction.header.sender;
		blockChain->accountTree.set(transaction.header.recipient, recipient);

		block.header.totalStakeAmount += transaction.header.amount;
	}
	else if (transaction.header.type == TransactionType::UNSTAKE) {
		Account sender = blockChain->accountTree.get(transaction.header.sender);
		sender.balance += transaction.header.amount;
		sender.balance -= transaction.header.fee;
		sender.transactionCount++;
		blockChain->accountTree.set(transaction.header.sender, sender);

		Account recipient = blockChain->accountTree.get(transaction.header.recipient);
		recipient.stakeAmount -= transaction.header.amount;
		if (recipient.stakeAmount == 0) {
			recipient.stakeOwner = EccPublicKey(0);
			recipient.stakeBlockNumber = 0;

			EccPublicKey lastAddress = blockChain->validatorTree.get((block.header.totalStakeAmount / blockChain->config.minimumStakeAmount) - 1);
			Account lastAccount = blockChain->accountTree.get(lastAddress);
			blockChain->validatorTree.set(recipient.validatorNumber, lastAddress);
			blockChain->validatorTree.set(lastAccount.validatorNumber, EccPublicKey(0));
			lastAccount.validatorNumber = recipient.validatorNumber;
			blockChain->accountTree.set(lastAddress, lastAccount);

			recipient.validatorNumber = 0;
		}
		blockChain->accountTree.set(transaction.header.recipient, recipient);

		block.header.totalStakeAmount -= transaction.header.amount;
	}

	Account beneficiary = blockChain->accountTree.get(block.header.beneficiary);
	beneficiary.balance += transaction.header.fee;
	blockChain->accountTree.set(block.header.beneficiary, beneficiary);

	blockChain->addTransaction(transaction);
}

Block& BlockCreator::endBlock() {
	block.header.transactionCount = block.transactionTree.transactionHashes.size();
	block.header.transactionTreeRoot = block.transactionTree.calculateRoot();
	block.header.accountTreeRoot = blockChain->accountTree.getRoot();
	block.header.validatorTreeRoot = blockChain->validatorTree.getRoot();
	return block;
}

Transaction BlockCreator::createTransaction(const EccPublicKey& sender, const EccPublicKey& recipient, Amount amount, uint64_t fee, TransactionType type) {
	Transaction tx;
	tx.header.version = blockChain->config.transactionVersion;
	tx.header.type = type;
	tx.header.timestamp = time(nullptr);
	tx.header.dataHash = 0;
	tx.header.sender = sender;
	tx.header.recipient = recipient;
	tx.header.amount = amount;
	tx.header.fee = fee;
	tx.header.transactionNumber = blockChain->accountTree.get(sender).transactionCount;
	return tx;
}
