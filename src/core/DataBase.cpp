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

	loadChain();
	loadTransactions();
	loadBlocks();
	loadState();
}

const std::string& DataBase::getDataPath() {
	return dataPath;
}

bool DataBase::getTransactionHeader(Hash transactionHash, TransactionHeader &transaction) {
	auto entry = transactions.find(transactionHash);
	if (entry != transactions.end()) {
		transaction = entry->second.header;
		return true;
	}
	else {
		return false;
	}
}

bool DataBase::getTransaction(Hash transactionHash, Transaction& transaction) {
	auto entry = transactions.find(transactionHash);
	if (entry != transactions.end()) {
		transaction = entry->second;
		return true;
	}
	else {
		return false;
	}
}

bool DataBase::getBlockHeader(Hash blockHash, BlockHeader& block) {
	auto entry = blocks.find(blockHash);
	if (entry != blocks.end()) {
		block = entry->second.header;
		return true;
	}
	else {
		return false;
	}
}

bool DataBase::getBlock(Hash blockHash, Block& block) {
	auto entry = blocks.find(blockHash);
	if (entry != blocks.end()) {
		block = entry->second;
		return true;
	}
	else {
		return false;
	}
}

void DataBase::addTransaction(const Transaction& transaction) {
	auto entry = transactions.find(transaction.transactionHash);
	if (entry == transactions.end()) {
		transactions[transaction.transactionHash] = transaction;
	}
}

void DataBase::addBlock(const Block& block) {
	auto entry = blocks.find(block.blockHash);
	if (entry == blocks.end()) {
		blocks[block.blockHash] = block;
	}
}

void DataBase::load() {
	loadChain();
	loadTransactions();
	loadBlocks();
	loadState();
}

void DataBase::save() {
	saveChain();
	saveTransactions();
	saveBlocks();
	saveState();
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

void DataBase::loadBlocks() {
	std::string file = dataPath + "blocks.txt";
	if (std::filesystem::exists(file)) {
		std::ifstream stream;
		stream.open(file, std::ifstream::binary);

		blocks.clear();

		uint32_t count = 0;
		stream.read((char*)&count, sizeof(count));
		blocks.reserve(count);

		for (int i = 0; i < count; i++) {
			Block block;
			block.deserial(stream);
			blocks[block.getHash()] = block;
		}

		stream.close();
	}
}

void DataBase::saveBlocks() {
	std::string file = dataPath + "blocks.txt";
	std::ofstream stream;
	stream.open(file, std::ofstream::binary);

	uint32_t count = blocks.size();
	stream.write((char*)&count, sizeof(count));

	for (auto &block : blocks) {
		block.second.serial(stream);
	}

	stream.close();
}

void DataBase::loadTransactions() {
	std::string file = dataPath + "transactions.txt";
	if (std::filesystem::exists(file)) {
		std::ifstream stream;
		stream.open(file, std::ifstream::binary);

		transactions.clear();

		uint32_t count = 0;
		stream.read((char*)&count, sizeof(count));

		for (int i = 0; i < count; i++) {
			Transaction transaction;
			transaction.deserial(stream);
			transactions[transaction.getHash()] = transaction;
		}

		stream.close();
	}
}

void DataBase::saveTransactions() {
	std::string file = dataPath + "transactions.txt";
	std::ofstream stream;
	stream.open(file, std::ofstream::binary);

	uint32_t count = transactions.size();
	stream.write((char*)&count, sizeof(count));

	for (auto& transaction : transactions) {
		transaction.second.serial(stream);
	}

	stream.close();
}

void DataBase::loadState() {
	std::string file = dataPath + "state.txt";
	if (std::filesystem::exists(file)) {
		std::ifstream stream;
		stream.open(file, std::ifstream::binary);

		chain->state.deserial(stream);
		stream.close();
	}
}

void DataBase::saveState() {
	std::string file = dataPath + "state.txt";
	std::ofstream stream;
	stream.open(file, std::ofstream::binary);

	chain->state.serial(stream);
	stream.close();
}
