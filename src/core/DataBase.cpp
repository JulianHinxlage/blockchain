#include "DataBase.h"
#include "util/hex.h"
#include <filesystem>
#include <fstream>

DataBase::DataBase() {
	chain = nullptr;
}

void DataBase::init(const std::string& path, BlockChain* chain) {
	this->chain = chain;
	chain->db = this;
	dataPath = std::filesystem::path(path).string();
	if (!dataPath.empty()) {
		if (dataPath.back() != '/' && dataPath.back() != '\\') {
			dataPath += "/";
		}
	}
	std::filesystem::create_directories(path);

	transactionStore.init(dataPath + "transactions");
	blockStore.init(dataPath + "blocks");
	
	loadChain();

	//load state
	chain->state.init(dataPath + "state");
	Block block;
	getBlock(chain->getLatestBlock(), block);
	chain->state.loadState(block.header.stateRoot);
}

const std::string& DataBase::getDataPath() {
	return dataPath;
}

bool DataBase::getTransactionHeader(Hash transactionHash, TransactionHeader& transaction) {
	Transaction fullTransaction;
	if (getTransaction(transactionHash, fullTransaction)) {
		transaction = fullTransaction.header;
		return true;
	}
	else {
		return false;
	}
}

bool DataBase::getTransaction(Hash transactionHash, Transaction& transaction) {
	auto value = transactionStore.get(transactionHash);
	if (value.ptr) {
		std::stringstream stream;
		stream.write((char*)value.ptr, value.size);
		transaction.deserial(stream);
		return true;
	}
	else {
		return false;
	}
}

bool DataBase::getBlockHeader(Hash blockHash, BlockHeader& block) {
	Block fullBlock;
	if (getBlock(blockHash, fullBlock)) {
		block = fullBlock.header;
		return true;
	}
	else {
		return false;
	}
}

bool DataBase::getBlock(Hash blockHash, Block& block) {
	auto value = blockStore.get(blockHash);
	if (value.ptr) {
		std::stringstream stream;
		stream.write((char*)value.ptr, value.size);
		block.deserial(stream);
		return true;
	}
	else {
		return false;
	}
}

void DataBase::addTransaction(const Transaction& transaction) {
	std::stringstream stream;
	transaction.serial(stream);
	transactionStore.set(transaction.transactionHash, stream.str());
}

void DataBase::addBlock(const Block& block) {
	std::stringstream stream;
	block.serial(stream);
	blockStore.set(block.blockHash, stream.str());
}

void DataBase::load() {
	loadChain();
}

void DataBase::save() {
	saveChain();
}

void DataBase::loadChain() {
	std::string file = dataPath + "chain.txt";

	if (std::filesystem::exists(file)) {
		std::ifstream stream;
		stream.open(file);

		chain->blocks.clear();

		std::string line;
		while (std::getline(stream, line)) {
			Hash hash;
			if (fromHex(line, hash)) {
				chain->blocks.push_back(hash);
			}
			else {
				hash = 0;
				chain->blocks.push_back(hash);
			}
		}

		stream.close();
	}
}

void DataBase::saveChain() {
	std::string file = dataPath + "chain.txt";
	std::ofstream stream;
	stream.open(file);

	for (int i = 0; i < chain->blocks.size(); i++) {
		std::string hash;
		toHex(chain->blocks[i], hash);
		stream.write(hash.c_str(), hash.size());
		stream.write("\n", 1);
	}

	stream.close();
}
