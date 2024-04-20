//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "blockchain/Node.h"
#include "blockchain/KeyStore.h"

class Wallet {
public:
	Node node;
	KeyStore keyStore;
	bool initContext = false;
	VerifyContext context;

	void init(const std::string &chainDir, const std::string& keyFile, const std::string& entryNodeFile);
	void sendTransaction(const std::string& address, const std::string& amount, const std::string& fee, TransactionType type = TransactionType::TRANSFER);
};
