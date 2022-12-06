#include "BlockChainState.h"
#include "cryptography/sha.h"
#include <sstream>

void BlockChainState::init(const std::string& directory) {
	stateTree.initStore(directory);
}

void BlockChainState::loadState(const Hash& hash) {
	stateTree.getHistoryState(hash, stateTree);
}

Hash BlockChainState::getHash() {
	return stateTree.root();
}

Account BlockChainState::getAccount(EccPublicKey address) {
	Account account;
	stateTree.get(address, account);
	return account;
}

void BlockChainState::setAccount(EccPublicKey address, Account account) {
	stateTree.set(address, account);
}

std::vector<std::pair<EccPublicKey, Account>> BlockChainState::getAllAccounts() {
	std::vector<std::pair<EccPublicKey, Account>> accounts;
	stateTree.each([&](const EccPublicKey& address, const Account& acc) {
		accounts.push_back({ address, acc });
	}, true);
	return accounts;
}

Account::Account() {
	balance = 0;
	nonce = 0;
	unused = 0;
	code = 0;
	storage = 0;
}

bool Account::serial(std::ostream& stream) {
	stream.write((char*)&balance, sizeof(balance));
	stream.write((char*)&nonce, sizeof(nonce));
	stream.write((char*)&code, sizeof(code));
	stream.write((char*)&storage, sizeof(storage));
	return true;
}

bool Account::deserial(std::istream& stream) {
	stream.read((char*)&balance, sizeof(balance));
	stream.read((char*)&nonce, sizeof(nonce));
	stream.read((char*)&code, sizeof(code));
	stream.read((char*)&storage, sizeof(storage));
	unused = 0;
	return true;
}
