//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "BlockCreator.h"

void BlockCreator::beginBlock(const EccPublicKey& validator) {
	block = Block();
	BlockHeader prev = blockChain->getBlockHeader(blockChain->latestBlock);
	block.header.version = blockChain->config.blockVersion;
	block.header.timestamp = time(nullptr);
	block.header.previousBlockHash = blockChain->latestBlock;
	block.header.blockNumber = prev.blockNumber + 1;
	block.header.validator = validator;
	block.header.beneficiary = validator;
	blockChain->accountTree.reset(prev.accountTreeRoot);
}

void BlockCreator::addTransaction(const Transaction& transaction) {
	block.transactionTree.transactionHashes.push_back(transaction.transactionHash);

	Account sender = blockChain->accountTree.get(transaction.header.sender);
	sender.balance -= transaction.header.amount + transaction.header.fee;
	sender.transactionCount++;
	blockChain->accountTree.set(transaction.header.sender, sender);
	
	Account recipient = blockChain->accountTree.get(transaction.header.recipient);
	recipient.balance += transaction.header.amount;
	blockChain->accountTree.set(transaction.header.recipient, recipient);
	
	Account beneficiary = blockChain->accountTree.get(block.header.beneficiary);
	beneficiary.balance += transaction.header.fee;
	blockChain->accountTree.set(block.header.beneficiary, beneficiary);

	blockChain->addTransaction(transaction);
}

Block& BlockCreator::endBlock() {
	block.header.transactionCount = block.transactionTree.transactionHashes.size();
	block.header.transactionTreeRoot = block.transactionTree.calculateRoot();
	block.header.accountTreeRoot = blockChain->accountTree.getRoot();
	return block;
}

Transaction BlockCreator::createTransaction(const EccPublicKey& sender, const EccPublicKey& recipient, Amount amount, uint64_t fee) {
	Transaction tx;
	tx.header.version = blockChain->config.transactionVersion;
	tx.header.timestamp = time(nullptr);
	tx.header.dataHash = 0;
	tx.header.sender = sender;
	tx.header.recipient = recipient;
	tx.header.amount = amount;
	tx.header.fee = fee;
	tx.header.transactionNumber = blockChain->accountTree.get(sender).transactionCount;
	return tx;
}
