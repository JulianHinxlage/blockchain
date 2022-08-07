#include "Block.h"
#include "cryptography/sha.h"
#include <sstream>

BlockHeader::BlockHeader() {
	version = 0;
	timestamp = 0;
	previousBlock = 0;
	blockNumber = 0;
	beneficiary = 0;
	validator = 0;
	transactionRoot = 0;
	stateRoot = 0;
	signature = 0;
}

bool BlockHeader::serial(std::ostream& stream) const {
	stream.write((char*)&version, sizeof(version));
	stream.write((char*)&timestamp, sizeof(timestamp));
	stream.write((char*)&previousBlock, sizeof(previousBlock));
	stream.write((char*)&blockNumber, sizeof(blockNumber));
	stream.write((char*)&beneficiary, sizeof(beneficiary));
	stream.write((char*)&validator, sizeof(validator));
	stream.write((char*)&transactionRoot, sizeof(transactionRoot));
	stream.write((char*)&stateRoot, sizeof(stateRoot));
	stream.write((char*)&signature, sizeof(signature));
	return true;
}

bool BlockHeader::deserial(std::istream& stream) {
	stream.read((char*)&version, sizeof(version));
	stream.read((char*)&timestamp, sizeof(timestamp));
	stream.read((char*)&previousBlock, sizeof(previousBlock));
	stream.read((char*)&blockNumber, sizeof(blockNumber));
	stream.read((char*)&beneficiary, sizeof(beneficiary));
	stream.read((char*)&validator, sizeof(validator));
	stream.read((char*)&transactionRoot, sizeof(transactionRoot));
	stream.read((char*)&stateRoot, sizeof(stateRoot));
	stream.read((char*)&signature, sizeof(signature));
	return true;
}

bool BlockHeader::signSerial(std::ostream& stream) const {
	stream.write((char*)&version, sizeof(version));
	stream.write((char*)&timestamp, sizeof(timestamp));
	stream.write((char*)&previousBlock, sizeof(previousBlock));
	stream.write((char*)&blockNumber, sizeof(blockNumber));
	stream.write((char*)&beneficiary, sizeof(beneficiary));
	stream.write((char*)&validator, sizeof(validator));
	stream.write((char*)&transactionRoot, sizeof(transactionRoot));
	stream.write((char*)&stateRoot, sizeof(stateRoot));
	return true;
}

Hash BlockHeader::getHash() const {
	std::stringstream stream;
	serial(stream);
	return sha256(stream.str());
}

void BlockHeader::sign(const EccPrivateKey& key) {
	std::stringstream stream;
	signSerial(stream);
	std::string str = stream.str();
	signature = eccCreateSignature(str.data(), str.size(), key);
}

bool BlockHeader::verifySignature() const {
	std::stringstream stream;
	signSerial(stream);
	std::string str = stream.str();
	return eccVerifySignature(str.data(), str.size(), validator, signature);
}


Block::Block() {
	blockHash = 0;
}

bool Block::serial(std::ostream& stream) const {
	header.serial(stream);
	transactionTree.serial(stream);
	return true;
}

bool Block::deserial(std::istream& stream) {
	header.deserial(stream);
	transactionTree.deserial(stream);
	blockHash = header.getHash();
	return true;
}

Hash Block::getHash() const {
	return blockHash;
}


bool TransactionTree::serial(std::ostream& stream) const {
	uint32_t count = transactionHashes.size();
	stream.write((char*)&count, sizeof(count));
	for (int i = 0; i < count; i++) {
		stream.write((char*)&transactionHashes[i], sizeof(transactionHashes[i]));
	}
	return true;
}

bool TransactionTree::deserial(std::istream& stream) {
	uint32_t count = 0;
	stream.read((char*)&count, sizeof(count));
	transactionHashes.resize(count);
	for (int i = 0; i < count; i++) {
		stream.read((char*)&transactionHashes[i], sizeof(transactionHashes[i]));
	}
	return true;
}

Hash TransactionTree::getHash() const {
	//generate merkle tree root hash
	std::vector<Hash> hashes = transactionHashes;
	while (hashes.size() > 1) {
		if (hashes.size() % 2 == 1) {
			hashes.push_back(Hash(0));
		}
		int count = hashes.size() / 2;
		for (int i = 0; i < count; i++) {
			hashes[i] = sha256((char*)&hashes[i * 2], sizeof(Hash) * 2);
		}
		hashes.resize(count);
	}
	if (hashes.size() >= 1) {
		return hashes[0];
	}
	else {
		return Hash(0);
	}
}
