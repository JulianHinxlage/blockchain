#pragma once

#include "type.h"
#include "BinaryTree.h"
#include "CachedKeyValueStore.h"
#include <unordered_map>

class Account {
public:
	Amount balance;
	uint32_t nonce;
	uint32_t unused;
	Hash code;
	Hash storage;

	Account();
	bool serial(std::ostream& stream);
	bool deserial(std::istream& stream);
};

class BlockChainState {
public:
	void init(const std::string &directory);
	void loadState(const Hash &hash);
	Hash getHash();

	Account getAccount(EccPublicKey address);
	void setAccount(EccPublicKey address, Account account);

	std::vector<std::pair<EccPublicKey, Account>> getAllAccounts();

private:
	BinaryTree<EccPublicKey, Account> stateTree;
};
