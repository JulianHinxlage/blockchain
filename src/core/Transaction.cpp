#include "Transaction.h"
#include "cryptography/sha.h"
#include <sstream>

Transaction::Transaction() {
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

Transaction::Transaction(Transaction& transaction) {
	operator=(transaction);
}

Transaction& Transaction::operator=(Transaction& transaction) {
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

bool Transaction::serial(std::ostream& stream) {
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

bool Transaction::deserial(std::istream& stream) {
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

bool Transaction::signSerial(std::ostream& stream) {
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

Hash Transaction::getHash() {
	std::stringstream stream;
	serial(stream);
	return sha256(stream.str());
}


bool FullTransaction::serial(std::ostream& stream) {
	transaction.serial(stream);
	uint32_t bytes = data.size();
	stream.write((char*)&bytes, sizeof(bytes));
	if (bytes > 0) {
		stream.write((char*)&data[0], data.size());
	}
	return true;
}

bool FullTransaction::deserial(std::istream& stream) {
	transaction.deserial(stream);
	uint32_t bytes = 0;
	stream.read((char*)&bytes, sizeof(bytes));
	data.resize(bytes);
	if (bytes > 0) {
		stream.read((char*)&data[0], data.size());
	}
	transactionHash = transaction.getHash();
	return true;
}

Hash FullTransaction::getHash() {
	return transactionHash;
}
