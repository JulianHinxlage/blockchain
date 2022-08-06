#include "BlockChainState.h"
#include "cryptography/sha.h"
#include <sstream>

bool BlockChainState::serial(std::ostream& stream) {
	uint32_t count = entries.size();
	stream.write((char*)&count, sizeof(count));
	for (auto& i : entries) {
		stream.write((char*)&i.first, sizeof(i.first));
		i.second.serial(stream);
	}
	return true;
}

bool BlockChainState::deserial(std::istream& stream) {
	uint32_t count = 0;
	stream.read((char*)&count, sizeof(count));
	entries.reserve(count);
	for (int i = 0; i < count; i++) {
		EccPublicKey address;
		AccountEntry entry;
		stream.read((char*)&address, sizeof(address));
		entry.deserial(stream);
		entries.push_back({ address, entry });
	}
	indexMap.clear();
	for (int i = 0; i < entries.size(); i++) {
		indexMap[entries[i].first] = i;
	}
	return true;
}

Hash BlockChainState::getHash() {
	//generate merkle tree root hash
	std::vector<Hash> hashes;

	for (int i = 0; i < entries.size(); i++) {
		std::stringstream stream;
		stream.write((char*)&entries[i].first, sizeof(entries[i].first));
		entries[i].second.serial(stream);
		hashes.push_back(sha256(stream.str()));
	}

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

AccountEntry BlockChainState::getAccount(EccPublicKey address) {
	auto entry = indexMap.find(address);
	if (entry != indexMap.end()) {
		int index = entry->second;
		if (index >= 0 && index < entries.size()) {
			return entries[index].second;
		}
	}
	return AccountEntry();
}

void BlockChainState::setAccount(EccPublicKey address, AccountEntry account) {
	auto entry = indexMap.find(address);
	if (entry != indexMap.end()) {
		int index = entry->second;
		entries[index].second = account;
		return;
	}
	entries.push_back({ address, account });
	indexMap[address] = entries.size() - 1;
}

AccountEntry::AccountEntry() {
	balance = 0;
	nonce = 0;
}

bool AccountEntry::serial(std::ostream& stream) {
	stream.write((char*)&balance, sizeof(balance));
	stream.write((char*)&nonce, sizeof(nonce));
	return true;
}

bool AccountEntry::deserial(std::istream& stream) {
	stream.read((char*)&balance, sizeof(balance));
	stream.read((char*)&nonce, sizeof(nonce));
	return true;
}
