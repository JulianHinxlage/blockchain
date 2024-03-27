//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "BlockChain.h"
#include "util/hex.h"

void BlockChain::init(const std::string& directory) {
	this->directory = directory;
	accountTree.init(directory + "/accounts");
	config.initDevNet(accountTree);
	blockStorage.init(directory + "/blocks");
	transactionStorage.init(directory + "/transactions");
	
	loadBlockList();
	if (blockList.empty()) {
		addBlock(config.genesisBlock);
		addBlockToTip(config.genesisBlockHash);
	}

	accountTree.reset(getBlock(latestBlock).header.accountTreeRoot);
}

TransactionHeader BlockChain::getTransactionHeader(const Hash& hash) {
	return getTransaction(hash).header;
}

Transaction BlockChain::getTransaction(const Hash& hash) {
	Transaction transaction;
	if (transactionStorage.has(hash)) {
		transaction.deserial(transactionStorage.get(hash));
		transaction.transactionHash = hash;
	}
	return transaction;
}

BlockHeader BlockChain::getBlockHeader(const Hash& hash) {
	return getBlock(hash).header;
}

Block BlockChain::getBlock(const Hash &hash) {
	Block block;
	if (blockStorage.has(hash)) {
		block.deserial(blockStorage.get(hash));
		block.blockHash = hash;
	}
	return block;
}

void BlockChain::addBlock(const Block& block) {
	if (!blockStorage.has(block.blockHash)) {
		blockStorage.set(block.blockHash, block.serial());
	}
}

void BlockChain::addTransaction(const Transaction& transaction) {
	if (!transactionStorage.has(transaction.transactionHash)) {
		transactionStorage.set(transaction.transactionHash, transaction.serial());
	}
}

bool BlockChain::addBlockToTip(const Hash& blockHash, bool check) {
	if (check) {
		if (blockStorage.has(blockHash)) {
			Block block = getBlock(blockHash);
			if (block.header.previousBlockHash != latestBlock) {
				return false;
			}
		}
		else {
			return false;
		}
	}

	blockList.push_back(blockHash);
	latestBlock = blockHash;
	blockCount++;
	saveBlockList();
	return true;
}

void BlockChain::loadBlockList() {
	std::ifstream stream(directory + "/chain.dat");
	blockList.clear();
	blockCount = 0;
	if (stream.is_open()) {
		std::string line;
		while (std::getline(stream, line)) {
			blockList.push_back(fromHex<Hash>(line));
		}
		blockCount = blockList.size();
		if (!blockList.empty()) {
			latestBlock = blockList.back();
		}
		else {
			latestBlock = Hash(0);
		}
	}
}

void BlockChain::saveBlockList() {
	std::string content;
	for (auto& i : blockList) {
		content += toHex(i) + "\n";
	}
	std::ofstream stream(directory + "/chain.dat");
	stream.write(content.data(), content.size());
}
