#pragma once

#include "type.h"
#include <unordered_map>

class AccountEntry {
public:
	Amount balance;
	uint32_t nonce;

	AccountEntry();
	bool serial(std::ostream& stream);
	bool deserial(std::istream& stream);
};

class BlockChainState {
public:

	bool serial(std::ostream& stream);
	bool deserial(std::istream& stream);
	Hash getHash();

	AccountEntry getAccount(EccPublicKey address);
	void setAccount(EccPublicKey address, AccountEntry account);

private:
	std::vector<std::pair<EccPublicKey, AccountEntry>> entries;
	std::unordered_map<EccPublicKey, int> indexMap;
};
