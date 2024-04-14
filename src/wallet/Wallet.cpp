#include "Wallet.h"
#include "util/log.h"

void Wallet::init(const std::string& chainDir, const std::string& keyFile, const std::string& entryNodeFile) {
	node.networkMode = NetworkMode::SERVER;
	node.storageMode = StorageMode::FULL;
	node.verifyMode = VerifyMode::FULL_VERIFY;

	node.init(chainDir, entryNodeFile);
	keyStore.loadOrCreate(keyFile);	

	node.verifyChain();
	node.synchronize();
	log(LogLevel::DEBUG, "Wallet", "chain tip: num=%i %s", node.blockChain.getBlockCount() - 1, toHex(node.blockChain.getLatestBlock()).c_str());
}

void Wallet::sendTransaction(const std::string &address, const std::string& amount, const std::string& fee) {
	Transaction transaction = node.creator.createTransaction(keyStore.getPublicKey(), fromHex<EccPublicKey>(address), coinToAmount(amount), coinToAmount(fee));
	transaction.header.sign(keyStore.getPrivateKey());
	transaction.transactionHash = transaction.header.caclulateHash();
	TransactionError error = node.verifier.verifyTransaction(transaction, true);
	if (error != TransactionError::VALID) {
		log(LogLevel::INFO, "Wallet", "invalid transaction %s: %s", toHex(transaction.transactionHash).c_str(), transactionErrorToString(error));
	}
	else {
		node.blockChain.addTransaction(transaction);
		log(LogLevel::INFO, "Wallet", "created transaction %s", toHex(transaction.transactionHash).c_str());
		node.network.sendTransaction(transaction);
	}
}
