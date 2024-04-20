//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "Block.h"
#include "cryptography/sha.h"
#include "cryptography/ecc.h"
#include "util/Serializer.h"

Hash BlockHeader::caclulateHash() const {
	return sha256(serial());
}

void BlockHeader::sign(const EccPrivateKey& privateKey) {
	Serializer serial;
	serial.write(version);
	serial.write(transactionCount);
	serial.write(timestamp);
	serial.write(blockNumber);
	serial.write(totalStakeAmount);
	serial.write(previousBlockHash);
	serial.write(validator);
	serial.write(beneficiary);
	serial.write(slot);
	serial.write(seed);
	serial.write(transactionTreeRoot);
	serial.write(accountTreeRoot);
	serial.write(validatorTreeRoot);
	std::string data = serial.toString();
	signature = eccCreateSignature(data.data(), data.size(), privateKey);
}

bool BlockHeader::verifySignature() const {
	Serializer serial;
	serial.write(version);
	serial.write(transactionCount);
	serial.write(timestamp);
	serial.write(blockNumber);
	serial.write(totalStakeAmount);
	serial.write(previousBlockHash);
	serial.write(validator);
	serial.write(beneficiary);
	serial.write(slot);
	serial.write(seed);
	serial.write(transactionTreeRoot);
	serial.write(accountTreeRoot);
	serial.write(validatorTreeRoot);
	std::string data = serial.toString();
	return eccVerifySignature(data.data(), data.size(), validator, signature);
}

std::string BlockHeader::serial() const {
	Serializer serial;
	serial.write(version);
	serial.write(transactionCount);
	serial.write(timestamp);
	serial.write(blockNumber);
	serial.write(totalStakeAmount);
	serial.write(previousBlockHash);
	serial.write(validator);
	serial.write(beneficiary);
	serial.write(slot);
	serial.write(seed);
	serial.write(transactionTreeRoot);
	serial.write(accountTreeRoot);
	serial.write(validatorTreeRoot);
	serial.write(signature);
	return serial.toString();
}

int BlockHeader::deserial(const std::string& str) {
	Serializer serial(str);
	serial.read(version);
	serial.read(transactionCount);
	serial.read(timestamp);
	serial.read(blockNumber);
	serial.read(totalStakeAmount);
	serial.read(previousBlockHash);
	serial.read(validator);
	serial.read(beneficiary);
	serial.read(slot);
	serial.read(seed);
	serial.read(transactionTreeRoot);
	serial.read(accountTreeRoot);
	serial.read(validatorTreeRoot);
	serial.read(signature);
	return serial.getReadIndex();
}

Hash TransactionTree::calculateRoot() const {
	std::vector<Hash> hashes = transactionHashes;
	while (hashes.size() > 1) {
		if (hashes.size() % 2 == 1) {
			hashes.push_back(Hash());
		}

		int count = hashes.size() / 2;
		for (int i = 0; i < count; i++) {
			hashes[i] = sha256((char*)&hashes[i * 2], sizeof(Hash) * 2);
		}
		hashes.resize(count);
	}
	if (hashes.size() > 0) {
		return hashes[0];
	}
	else {
		return Hash();
	}
}

std::string Block::serial() const {
	Serializer serial(header.serial());
	serial.writeBytes((uint8_t*)transactionTree.transactionHashes.data(), transactionTree.transactionHashes.size() * sizeof(Hash));
	return serial.toString();
}

int Block::deserial(const std::string& str) {
	Serializer serial(str);
	serial.skip(header.deserial(str));
	int size = serial.size() - serial.getReadIndex();
	transactionTree.transactionHashes.resize(size / sizeof(Hash));
	serial.readBytes((uint8_t*)transactionTree.transactionHashes.data(), transactionTree.transactionHashes.size() * sizeof(Hash));
	return serial.getReadIndex();
}

