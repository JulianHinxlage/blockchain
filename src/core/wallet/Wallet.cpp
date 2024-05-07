//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "Wallet.h"
#include "util/log.h"

void Wallet::init(const std::string& chainDir, const std::string& keyFile, const std::string& entryNodeFile) {
	node.networkMode = NetworkMode::SERVER;
	node.storageMode = StorageMode::FULL;

	node.init(chainDir, entryNodeFile);
	if (!keyStore.init(keyFile)) {
		keyStore.createFile(keyFile);
		keyStore.generateMasterKey();
		keyStore.generatePrivateKey(0, 0);
		keyStore.setPassword("");
		keyStore.save();
	}
	else {
		keyStore.unlock("");
	}

	node.verifyChain();
	node.synchronize();
	node.synchronizePendingTransactions();
	log(LogLevel::DEBUG, "Wallet", "chain head: num=%i %s", node.blockChain.getBlockCount() - 1, toHex(node.blockChain.getHeadBlock()).c_str());

	initContext = false;
}

void Wallet::sendTransaction(const std::string &address, const std::string& amount, const std::string& fee, TransactionType type) {
	if (!initContext) {
		initContext = true;
		context = node.verifier.createContext(node.blockChain.getHeadBlock());
	}
	
	uint32_t transactionNumber = context.accountTree.get(keyStore.getPublicKey()).transactionCount;
	Transaction transaction = node.creator.createTransaction(keyStore.getPublicKey(), fromHex<EccPublicKey>(address), transactionNumber, coinToAmount(amount), coinToAmount(fee), type);
	transaction.header.sign(keyStore.getPrivateKey());
	transaction.transactionHash = transaction.header.caclulateHash();
	TransactionError error = node.verifier.verifyTransaction(transaction, context);
	if (error != TransactionError::VALID) {
		log(LogLevel::INFO, "Wallet", "invalid transaction %s: %s", toHex(transaction.transactionHash).c_str(), transactionErrorToString(error));
	}
	else {
		node.blockChain.addTransaction(transaction);
		log(LogLevel::INFO, "Wallet", "created transaction %s", toHex(transaction.transactionHash).c_str());
		node.network.sendTransaction(transaction);
	}
}

Account Wallet::getAccount() {
	Account account = node.blockChain.getAccountTree().get(keyStore.getPublicKey());
	return account;
}
