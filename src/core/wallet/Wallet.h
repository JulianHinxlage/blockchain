//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "blockchain/FullNode.h"
#include "storage/KeyStore.h"

class Wallet {
public:
	FullNode node;
	KeyStore keyStore;
	bool initContext = false;
	VerifyContext context;
	bool initialized = false;

	void init(const std::string &chainDir, const std::string& keyFile, const std::string& entryNodeFile);
	void sendTransaction(const std::string& address, const std::string& amount, const std::string& fee, TransactionType type = TransactionType::TRANSFER);
	Account getAccount();
	Amount getPendingBalance();
};
