
#include "Validator.h"
#include "util/random.h"
#include "util/log.h"

Validator::~Validator() {
	if (thread) {
		running = false;
		cv.notify_all();
		if (thread->joinable()) {
			thread->join();
		}
		else {
			thread->detach();
		}
		delete thread;
		thread = nullptr;
	}
}

void Validator::init(const std::string& chainDir, const std::string& keyFile, const std::string& entryNodeFile) {
	node.networkMode = NetworkMode::SERVER;
	node.storageMode = StorageMode::FULL;
	node.verifyMode = VerifyMode::FULL_VERIFY;

	node.onTransactionRecived = [&](const Transaction& transaction) {
		pendingTransactions[transaction.transactionHash] = transaction;
		cv.notify_one();
	};
	node.onBlockRecived = [&](const Block& block) {
		for (auto& hash : block.transactionTree.transactionHashes) {
			pendingTransactions.erase(hash);
		}
	};

	node.init(chainDir, entryNodeFile);
	keyStore.loadOrCreate(keyFile);
	node.verifyChain();
	node.synchronize();
	log(LogLevel::DEBUG, "Validator", "chain tip: num=%i %s", node.blockChain.getBlockCount() - 1, toHex(node.blockChain.getLatestBlock()).c_str());

	running = true;
	thread = new std::thread([&]() {
		while (running) {
			std::unique_lock<std::mutex> lock(mutex);
			if (pendingTransactions.size() == 0) {
				cv.wait(lock);
			}
			if (running) {
				std::this_thread::sleep_for(std::chrono::milliseconds(random<uint32_t>() % 4000));
				createBlock();
			}
		}
	});
}

void Validator::createBlock() {
	BlockHeader tip = node.blockChain.getBlockHeader(node.blockChain.getLatestBlock());
	node.creator.beginBlock(keyStore.getPublicKey());

	std::set<int> numbers;
	for (auto& i : pendingTransactions) {
		numbers.insert(i.second.header.transactionNumber);
	}

	for (auto& number : numbers) {
		for (auto& i : pendingTransactions) {
			auto& transaction = i.second;
			if (i.second.header.transactionNumber == number) {
				TransactionError error = node.verifier.verifyTransaction(transaction);
				if (error == TransactionError::VALID) {
					node.creator.addTransaction(transaction);
				}
				else {
					log(LogLevel::INFO, "Validator", "invalid transaction %s: %s", toHex(transaction.transactionHash).c_str(), transactionErrorToString(error));
				}
			}
		}
	}


	Block block = node.creator.endBlock();
	if (block.header.transactionCount == 0) {
		return;
	}
	block.header.sign(keyStore.getPrivateKey());
	block.blockHash = block.header.caclulateHash();

	BlockError error = node.verifier.verifyBlock(block);
	if (error == BlockError::VALID) {
		node.blockChain.addBlock(block);
		log(LogLevel::INFO, "Validator", "created block %s", toHex(block.blockHash).c_str());
		node.blockChain.addBlockToTip(block.blockHash);
		node.network.sendBlock(block);
		
		for (auto& hash : block.transactionTree.transactionHashes) {
			pendingTransactions.erase(hash);
		}
	}
	else {
		log(LogLevel::INFO, "Validator", "invalid error %s: %s", toHex(block.blockHash).c_str(), blockErrorToString(error));
	}
}
