#pragma once

#include "core/ChainNetwork.h"
#include "core/Node.h"
#include "wallet/Wallet.h"

class Validator {
public:
	Node node;
	ChainNetwork network;
	Wallet wallet;
	std::vector<std::pair<Amount, Hash>> unconfirmedTransactionPool;

	void init();
	void run();

private:
	std::mutex mutex;
	std::condition_variable blockCv;
	std::condition_variable transactionCv;
};