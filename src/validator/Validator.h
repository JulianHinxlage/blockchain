//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "blockchain/Node.h"
#include "blockchain/KeyStore.h"

class Validator {
public:
	Node node;
	KeyStore keyStore;
	std::map<Hash, Transaction> pendingTransactions;
	std::thread* thread;
	std::atomic_bool running;
	bool createEmptyBlocks = true;

	~Validator();
	void init(const std::string& chainDir, const std::string& keyFile, const std::string& entryNodeFile);
	void epoch();
	void createBlock(uint32_t slot, uint64_t slotBeginTime);
};
