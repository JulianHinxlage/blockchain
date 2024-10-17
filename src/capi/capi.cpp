//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "capi.h"

#include "wallet/Wallet.h"
#include "util/log.h"

Wallet wallet;

extern "C" {
	
	void init(const char *chainDir, const char* keyFile, const char* entryNodeFile) {
		wallet.init(chainDir, keyFile, entryNodeFile);
	}

	const char* getBalance(const char* address) {
		Account account = wallet.node.blockChain.getAccountTree().get(fromHex<EccPublicKey>(address));
		static thread_local std::string str = "";
		str = amountToCoin(account.balance);
		return str.c_str();
	}

	const char* getPendingBalance(const char* addressStr) { 
		EccPublicKey address = fromHex<EccPublicKey>(addressStr);

		Amount in = 0;
		Amount out = 0;
		for (auto& hash : wallet.node.blockChain.getPendingTransactions()) {
			Transaction tx = wallet.node.blockChain.getTransaction(hash);
			if (tx.header.recipient == address) {
				in += tx.header.amount;
			}
			if (tx.header.sender == address) {
				out += tx.header.amount;
			}
		}

		static thread_local std::string str = "";
		if (out > in) {
			str = "-" + amountToCoin(out - in);
		}
		else {
			str = amountToCoin(in - out);
		}
		return str.c_str();
	}

	const char* getTransactionAmount(const char* transactionHash) {
		TransactionHeader tx = wallet.node.blockChain.getTransactionHeader(fromHex<Hash>(transactionHash));
		static thread_local std::string str = "";
		str = amountToCoin(tx.amount);
		return str.c_str();
	}

	std::string dateAndTime(uint32_t time) {
		time_t rawtime = time;
		struct tm* timeinfo;
		char buffer[80];

		timeinfo = localtime(&rawtime);

		strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", timeinfo);
		return std::string(buffer);
	}

	const char* getTransactionTime(const char* transactionHash) {
		TransactionHeader tx = wallet.node.blockChain.getTransactionHeader(fromHex<Hash>(transactionHash));
		static thread_local std::string str = "";
		str = dateAndTime(tx.timestamp);
		return str.c_str();
	}

	const char* getTransactionSender(const char* transactionHash) {
		TransactionHeader tx = wallet.node.blockChain.getTransactionHeader(fromHex<Hash>(transactionHash));
		static thread_local std::string str = "";
		str = toHex(tx.sender);
		return str.c_str();
	}

	const char* getTransactionRecipient(const char* transactionHash) {
		TransactionHeader tx = wallet.node.blockChain.getTransactionHeader(fromHex<Hash>(transactionHash));
		static thread_local std::string str = "";
		str = toHex(tx.recipient);
		return str.c_str();
	}

	const char* getTransactions(const char* addressStr){
		EccPublicKey address = fromHex<EccPublicKey>(addressStr);

		static thread_local std::string data = "";
		data = "";

		int count = wallet.node.blockChain.getBlockCount();
		for (int i = 0; i < count; i++) {
			Hash hash = wallet.node.blockChain.getBlockHash(i);
			Block block = wallet.node.blockChain.getBlock(hash);
			for (auto& txHash : block.transactionTree.transactionHashes) {
				Transaction tx = wallet.node.blockChain.getTransaction(txHash);

				bool isSender = tx.header.sender == address;
				bool isRecipient = tx.header.recipient == address;
				if (isSender || isRecipient) {

					if (!data.empty()) {
						data += "\n";
					}
					data += toHex(txHash);
				}
			}
		}

		return data.c_str();
	}

	const char* getPendingTransactions() {
		static std::string data = "";
		data = "";

		for (auto& hash : wallet.node.blockChain.getPendingTransactions()) {
			if (!data.empty()) {
				data += "\n";
			}
			data += toHex(hash);
		}
		return data.c_str();
	}

	const char* getPendingTransactionsForAddress(const char* addressStr) {
		EccPublicKey address = fromHex<EccPublicKey>(addressStr);

		static thread_local std::string data = "";
		data = "";

		for (auto& hash : wallet.node.blockChain.getPendingTransactions()) {
			Transaction tx = wallet.node.blockChain.getTransaction(hash);
			if (tx.header.recipient == address || tx.header.sender == address) {
				if (!data.empty()) {
					data += "\n";
				}
				data += toHex(hash);
			}
		}
		return data.c_str();
	}

	const char* createTransaction(const char* senderStr, const char* recipientStr, const char* amount, const char* fee, const char* typeStr) {
		EccPublicKey sender = fromHex<EccPublicKey>(senderStr);
		EccPublicKey recipient = fromHex<EccPublicKey>(recipientStr);

		TransactionType type = TransactionType::TRANSFER;
		if (typeStr == "transfer") {
			type = TransactionType::TRANSFER;
		}
		else if (typeStr == "stake") {
			type = TransactionType::STAKE;
		}
		else if (typeStr == "unstake") {
			type = TransactionType::UNSTAKE;
		}

		uint32_t transactionNumber = wallet.node.blockChain.getAccountTree().get(sender).transactionCount;
		Transaction transaction = wallet.node.creator.createTransaction(sender, recipient, transactionNumber, coinToAmount(amount), coinToAmount(fee), type);
		
		static thread_local std::string data = "";
		data = stringToHex(transaction.serial());

		return data.c_str();
	}

	const char* sendTransaction(const char* data) {
		Transaction transaction;
		transaction.deserial(stringfromHex(data));

		VerifyContext context = wallet.node.verifier.createContext(wallet.node.blockChain.getHeadBlock());

		transaction.transactionHash = transaction.header.caclulateHash();
		TransactionError error = wallet.node.verifier.verifyTransaction(transaction, context);


		if (error != TransactionError::VALID) {
			log(LogLevel::INFO, "Wallet", "invalid transaction %s: %s", toHex(transaction.transactionHash).c_str(), transactionErrorToString(error));
		}
		else {
			wallet.node.blockChain.addTransaction(transaction);
			wallet.node.blockChain.addPendingTransaction(transaction.transactionHash);
			log(LogLevel::INFO, "Wallet", "created transaction %s", toHex(transaction.transactionHash).c_str());
			wallet.node.network.sendTransaction(transaction);
		}

		static thread_local std::string str = "";
		str = transactionErrorToString(error);
		return str.c_str();
	}

}
