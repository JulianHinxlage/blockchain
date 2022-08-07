#include "Transaction.h"
#include "cryptography/sha.h"
#include <sstream>

TransactionHeader::TransactionHeader() {
	version = 0;
	timestamp = 0;
	from = 0;
	to = 0;
	amount = 0;
	fee = 0;
	dataHash = 0;
	nonce = 0;
	signature = 0;
}

TransactionHeader::TransactionHeader(const TransactionHeader& transaction) {
	operator=(transaction);
}

TransactionHeader& TransactionHeader::operator=(const TransactionHeader& transaction) {
	version = transaction.version;
	timestamp = transaction.timestamp;
	from = transaction.from;
	to = transaction.to;
	amount = transaction.amount;
	fee = transaction.fee;
	dataHash = transaction.dataHash;
	nonce = transaction.nonce;
	signature = transaction.signature;
	return *this;
}

bool TransactionHeader::serial(std::ostream& stream) const {
	stream.write((char*)&version, sizeof(version));
	stream.write((char*)&timestamp, sizeof(timestamp));
	stream.write((char*)&from, sizeof(from));
	stream.write((char*)&to, sizeof(to));
	stream.write((char*)&amount, sizeof(amount));
	stream.write((char*)&fee, sizeof(fee));
	stream.write((char*)&dataHash, sizeof(dataHash));
	stream.write((char*)&nonce, sizeof(nonce));
	stream.write((char*)&signature, sizeof(signature));
	return true;
}

bool TransactionHeader::deserial(std::istream& stream) {
	stream.read((char*)&version, sizeof(version));
	stream.read((char*)&timestamp, sizeof(timestamp));
	stream.read((char*)&from, sizeof(from));
	stream.read((char*)&to, sizeof(to));
	stream.read((char*)&amount, sizeof(amount));
	stream.read((char*)&fee, sizeof(fee));
	stream.read((char*)&dataHash, sizeof(dataHash));
	stream.read((char*)&nonce, sizeof(nonce));
	stream.read((char*)&signature, sizeof(signature));
	return true;
}

bool TransactionHeader::signSerial(std::ostream& stream) const {
	stream.write((char*)&version, sizeof(version));
	stream.write((char*)&timestamp, sizeof(timestamp));
	stream.write((char*)&from, sizeof(from));
	stream.write((char*)&to, sizeof(to));
	stream.write((char*)&amount, sizeof(amount));
	stream.write((char*)&fee, sizeof(fee));
	stream.write((char*)&dataHash, sizeof(dataHash));
	stream.write((char*)&nonce, sizeof(nonce));
	return true;
}

Hash TransactionHeader::getHash() const {
	std::stringstream stream;
	serial(stream);
	return sha256(stream.str());
}

void TransactionHeader::sign(const EccPrivateKey& key) {
	std::stringstream stream;
	signSerial(stream);
	std::string str = stream.str();
	signature = eccCreateSignature(str.data(), str.size(), key);
}

bool TransactionHeader::verifySignature() const {
	std::stringstream stream;
	signSerial(stream);
	std::string str = stream.str();
	return eccVerifySignature(str.data(), str.size(), from, signature);
}


Transaction::Transaction() {
	transactionHash = 0;
}

bool Transaction::serial(std::ostream& stream) const {
	header.serial(stream);
	uint32_t bytes = data.size();
	stream.write((char*)&bytes, sizeof(bytes));
	if (bytes > 0) {
		stream.write((char*)&data[0], data.size());
	}
	return true;
}

bool Transaction::deserial(std::istream& stream) {
	header.deserial(stream);
	uint32_t bytes = 0;
	stream.read((char*)&bytes, sizeof(bytes));
	data.resize(bytes);
	if (bytes > 0) {
		stream.read((char*)&data[0], data.size());
	}
	transactionHash = header.getHash();
	return true;
}

Hash Transaction::getHash() const {
	return transactionHash;
}
