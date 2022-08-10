#include "Validator.h"
#include "util/hex.h"
#include "util/random.h"

void Validator::init() {
	network.onBlockReceived = [&](const Block& block) {
		if (block.blockHash == block.header.getHash()) {
			std::string str;
			toHex(block.blockHash, str);
			BlockError error = node.validator.validateBlock(block);

			if (error == BlockError::NONE) {
				printf("valid block: %s\n", str.c_str());
				node.chain.addBlock(block);

				for (auto& tx1 : block.transactionTree.transactionHashes) {
					for (int i = 0; i < unconfirmedTransactionPool.size(); i++) {
						auto& tx2 = unconfirmedTransactionPool[i];
						if (tx1 == tx2.second) {
							unconfirmedTransactionPool.erase(unconfirmedTransactionPool.begin() + i);
							break;
						}
					}
				}

				node.db.save();
				blockCv.notify_one();
			}
			else {
				printf("invalid block: %s with code %i\n", str.c_str(), error);
			}
		}
	};
	network.onTransactionReceived = [&](const Transaction& transaction, bool partOfBlock) {
		if (transaction.transactionHash == transaction.header.getHash()) {
			if (!partOfBlock) {
				std::string str;
				toHex(transaction.transactionHash, str);
				printf("tx: %s\n", str.c_str());
			}
			node.db.addTransaction(transaction);

			int index = 0;
			for (index = 0; index < unconfirmedTransactionPool.size(); index++) {
				auto& tx = unconfirmedTransactionPool[index];
				if (transaction.transactionHash == tx.second) {
					index = -1;
					break;
				}
				if (transaction.header.fee > tx.first) {
					break;
				}
			}
			if (index != -1) {
				unconfirmedTransactionPool.insert(unconfirmedTransactionPool.begin() + index, { transaction.header.fee, transaction.transactionHash });
			}

			if (!partOfBlock) {
				transactionCv.notify_one();
			}
		}
	};
}

void Validator::run() {
	while (true) {

		if (unconfirmedTransactionPool.size() == 0) {
			std::unique_lock<std::mutex> lock(mutex);
			transactionCv.wait(lock);
		}

		int millis = randomBytes<uint32_t>() % 10000;
		std::this_thread::sleep_for(std::chrono::milliseconds(millis));


		node.creator.reset();
		node.creator.getBlockChainState(node.chain.state, node.chain.getLatestBlock());

		while (unconfirmedTransactionPool.size() > 0) {
			auto& tx = unconfirmedTransactionPool.front();
			Transaction transaction;
			if (node.db.getTransaction(tx.second, transaction)) {
				TransactionError error = node.creator.addTransaction(transaction);
				std::string str;
				toHex(transaction.transactionHash, str); 
				if (error == TransactionError::NONE) {
					printf("valid tx: %s\n", str.c_str());
				}
				else {
					printf("invalid tx: %s with code %i\n", str.c_str(), error);
				}
			}
			unconfirmedTransactionPool.erase(unconfirmedTransactionPool.begin());
		}

		if (node.creator.block.transactionTree.transactionHashes.size() > 0) {
			Block block;
			BlockError error = node.creator.createBlock(wallet.publicKey, wallet.publicKey, wallet.privateKey, &block);
			if (error == BlockError::NONE) {
				error = node.validator.validateBlock(block);
				if (error == BlockError::NONE) {
					std::string str;
					toHex(block.blockHash, str);
					printf("created block: %s\n", str.c_str());
					network.broadcastBlock(block);
					node.db.save();
				}
			}
		}

	}
}
