//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "BlockChain.h"
#include "util/hex.h"

void BlockChain::init(const std::string& directory) {
	this->directory = directory;
	consensus.blockChain = this;
	accountTreeStorage.init(directory + "/accounts");
	accountTree.init(&accountTreeStorage);
	config.initDevNet(accountTree);
	blockStorage.init(directory + "/blocks");
	transactionStorage.init(directory + "/transactions");
	validatorTreeStorage.init(directory + "/validators");
	validatorTree.init(&validatorTreeStorage);

	loadBlockList();
	if (!hasBlock(config.genesisBlockHash)) {
		addBlock(config.genesisBlock);
	}
	if (blockList.empty()) {
		setHeadBlock(config.genesisBlockHash);
	}

	accountTree.reset(getBlock(getHeadBlock()).header.accountTreeRoot);
	validatorTree.reset(getBlock(getHeadBlock()).header.validatorTreeRoot);

	loadMetaData();
	loadPendingTransactions();
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

Block BlockChain::getBlock(const Hash& hash) {
	Block block;
	if (blockStorage.has(hash)) {
		block.deserial(blockStorage.get(hash));
		block.blockHash = hash;
	}
	return block;
}

bool BlockChain::hasBlock(const Hash& hash) {
	return blockStorage.has(hash);
}

bool BlockChain::hasTransaction(const Hash& hash) {
	return transactionStorage.has(hash);
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

BlockMetaData BlockChain::getMetaData(const Hash& blockHash) {
	if (blockHash == Hash(0) || blockHash == config.genesisBlockHash) {
		BlockMetaData data;
		data.lastCheck = BlockError::VALID;
		return data;
	}
	return metaData[blockHash];
}

void BlockChain::setMetaData(const Hash& blockHash, BlockMetaData data) {
	metaData[blockHash] = data;
	saveMetaData();
}

void BlockChain::addPendingTransaction(const Hash& transactionHash) {
	if (!pendingTransactions.contains(transactionHash)) {
		pendingTransactions.insert(transactionHash);
		savePendingTransactions();
	}
}

void BlockChain::removePendingTransaction(const Hash& transactionHash) {
	if(pendingTransactions.contains(transactionHash)){
		pendingTransactions.erase(transactionHash);
		savePendingTransactions();
	}
}

const std::set<Hash>& BlockChain::getPendingTransactions() {
	return pendingTransactions;
}

bool BlockChain::setHeadBlock(const Hash& blockHash) {
	uint64_t commonBlockNumber = 0;
	std::vector<Hash> newChain;

	if (blockHash == config.genesisBlockHash) {
		blockList.resize(0);
		blockList.push_back(config.genesisBlockHash);
		saveBlockList();
		return true;
	}

	Hash current = blockHash;
	while (true) {
		if (hasBlock(current)) {
			BlockHeader block = getBlockHeader(current);
			if (block.blockNumber < getBlockCount()) {
				if (getBlockHash(block.blockNumber) == current) {
					commonBlockNumber = block.blockNumber;
					break;
				}
			}
			newChain.push_back(current);
			current = block.previousBlockHash;
		}
		else {
			return false;
		}
	}

	if (newChain.size() == 0 && commonBlockNumber + 1 == getBlockCount()) {
		return false;
	}

	blockList.resize(commonBlockNumber + 1 - blockListStartOffset);
	for (int i = newChain.size() - 1; i >= 0; i--) {
		blockList.push_back(newChain[i]);
	}

	saveBlockList();
	return true;
}

Hash BlockChain::getHeadBlock() {
	if (blockList.size() == 0) {
		return Hash(0);
	}
	return blockList[blockList.size() - 1];
}

int BlockChain::getBlockCount() {
	return blockListStartOffset + blockList.size();
}

Hash BlockChain::getBlockHash(int blockNumber) {
	if (blockNumber >= 0 && blockNumber < getBlockCount()) {
		return blockList[blockNumber - blockListStartOffset];
	}
	return Hash(0);
}

AccountTree BlockChain::getAccountTree(const Hash& root) {
	return accountTree.createInstance(root);
}

AccountTree BlockChain::getAccountTree() {
	return accountTree.createInstance(getBlock(getHeadBlock()).header.accountTreeRoot);
}

ValidatorTree BlockChain::getValidatorTree(const Hash& root) {
	return validatorTree.createInstance(root);
}

void BlockChain::loadBlockList() {
	std::ifstream stream(directory + "/chain.dat");
	blockList.clear();
	blockListStartOffset = 0;
	if (stream.is_open()) {
		std::string line;
		while (std::getline(stream, line)) {
			blockList.push_back(fromHex<Hash>(line));
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

void BlockChain::loadMetaData() {
	metaData.clear();
	std::ifstream stream(directory + "/meta.dat", std::ios::binary);
	if (stream.is_open()) {
		std::string content((std::istreambuf_iterator<char>(stream)), (std::istreambuf_iterator<char>()));
		Serializer serial(content);
		while (serial.hasDataLeft()) {
			Hash hash = serial.read<Hash>();
			BlockMetaData data = serial.read<BlockMetaData>();
			metaData[hash] = data;
		}
	}
}

void BlockChain::saveMetaData(){
	Serializer serial;
	for (auto& i : metaData) {
		serial.write(i.first);
		serial.write(i.second);
	}
	std::ofstream stream(directory + "/meta.dat", std::ios::binary);
	stream.write((char*)serial.data(), serial.size());
}

void BlockChain::savePendingTransactions() {
	Serializer serial;
	for (auto &i : pendingTransactions) {
		serial.write(i);
	}
	std::ofstream stream(directory + "/pending.dat", std::ios::binary);
	stream.write((char*)serial.data(), serial.size());
}

void BlockChain::loadPendingTransactions() {
	pendingTransactions.clear();
	std::ifstream stream(directory + "/pending.dat", std::ios::binary);
	if (stream.is_open()) {
		std::string content((std::istreambuf_iterator<char>(stream)), (std::istreambuf_iterator<char>()));
		Serializer serial(content);
		while (serial.hasDataLeft()) {
			Hash hash = serial.read<Hash>();
			pendingTransactions.insert(hash);
		}
	}
}
