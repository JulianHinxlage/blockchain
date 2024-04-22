//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "Validator.h"
#include "util/random.h"
#include "util/log.h"
#include <map>

Validator::~Validator() {
	if (thread) {
		running = false;
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

	node.init(chainDir, entryNodeFile);
	if (!keyStore.init(keyFile)) {
		keyStore.createFile(keyFile);
		keyStore.generateMasterKey();
		keyStore.generatePrivateKey(0, 0);
		keyStore.setPassword("");
	}
	else {
		keyStore.unlock("");
	}

	node.verifyChain();
	node.synchronize();
	node.synchronizePendingTransactions();
	log(LogLevel::DEBUG, "Validator", "chain head: num=%i %s", node.blockChain.getBlockCount() - 1, toHex(node.blockChain.getHeadBlock()).c_str());

	running = true;
	thread = new std::thread([&]() {
		while (running) {
			if (node.getState() != NodeState::RUNNING) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
			else {
				epoch();
			}
		}
	});
}

uint64_t nowMilli() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void Validator::epoch() {
	uint64_t number = node.blockChain.getBlockCount();
	uint64_t slotTime = node.blockChain.config.slotTime;

	while (number == 1 && node.blockChain.getPendingTransactions().size() == 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		number = node.blockChain.getBlockCount();
	}

	Block prev = node.blockChain.getBlock(node.blockChain.getHeadBlock());

	uint64_t epochBeginTime = ((uint64_t)prev.header.timestamp / slotTime) * slotTime + slotTime;
	int64_t timePastInEpochMilli = (nowMilli() - epochBeginTime * 1000);
	int startSlot = timePastInEpochMilli / 1000 / slotTime;

	if (number == 1) {
		epochBeginTime = ((uint64_t)time(nullptr) / slotTime) * slotTime + slotTime;
		timePastInEpochMilli = (nowMilli() - epochBeginTime * 1000);
		startSlot = timePastInEpochMilli / 1000 / slotTime;
	}
	

	if (timePastInEpochMilli < 0) {
		log(LogLevel::INFO, "Validator", "wait for epoch %i to begin", number);
		std::this_thread::sleep_for(std::chrono::milliseconds(-timePastInEpochMilli));
		timePastInEpochMilli = (nowMilli() - epochBeginTime * 1000);
		startSlot = timePastInEpochMilli / 1000 / slotTime;
	}

	log(LogLevel::INFO, "Validator", "begin epoch %i", number);



	for (int slot = startSlot;; slot++) {
		uint64_t slotBeginTime = epochBeginTime + slot * slotTime;
		int64_t timePastInSlotMillis = (nowMilli() - slotBeginTime * 1000);

		if (!running) {
			return;
		}

		if (timePastInSlotMillis < 0) {
			log(LogLevel::INFO, "Validator", "wait for slot %i to begin", slot);
			std::this_thread::sleep_for(std::chrono::milliseconds(-timePastInSlotMillis));
			timePastInSlotMillis = (nowMilli() - slotBeginTime * 1000);
		}

		if (!running) {
			return;
		}

		EccPublicKey validator = node.blockChain.consensus.selectNextValidator(prev.header, slot);
		log(LogLevel::INFO, "Validator", "begin slot %i for epoch %i", slot, number);
		log(LogLevel::INFO, "Validator", "pending transaction count: %i", (int)node.blockChain.getPendingTransactions().size());
		log(LogLevel::INFO, "Validator", "for slot %i validator %s was selected", slot, toHex(validator).c_str());
		if (validator == keyStore.getPublicKey() || validator == EccPublicKey(0)) {
			log(LogLevel::INFO, "Validator", "###### local key selected as validator for block %i slot %i ######", number, slot);
		}

		bool slotWasSuccessfull = false;
		bool creadedBlock = false;
		for (int i = timePastInSlotMillis / 1000; i < slotTime; i++) {
			if (!creadedBlock && (validator == keyStore.getPublicKey() || validator == EccPublicKey(0))) {
				createBlock(slot, slotBeginTime);
				creadedBlock = true;
			}

			if (node.blockChain.getBlockCount() > number) {
				slotWasSuccessfull = true;
				break;
			}

			if (!running) {
				return;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		
			if (!running) {
				return;
			}
		}

		if (slotWasSuccessfull) {
			log(LogLevel::INFO, "Validator", "end epoch %i", number);
			break;
		}
	}

	for (auto& hash : invalidPendingTransactions) {
		node.blockChain.removePendingTransaction(hash);
	}
	invalidPendingTransactions.clear();

}

void Validator::createBlock(uint32_t slot, uint64_t slotBeginTime) {
	uint64_t timestamp = time(nullptr);

	timestamp = std::max(slotBeginTime, timestamp);
	timestamp = std::min(slotBeginTime + node.blockChain.config.slotTime - 1, timestamp);

	if (beneficiary == EccPublicKey(0)) {
		node.creator.beginBlock(keyStore.getPublicKey(), keyStore.getPublicKey(), slot, timestamp);
	}
	else {
		node.creator.beginBlock(keyStore.getPublicKey(), beneficiary, slot, timestamp);
	}

	VerifyContext context = node.verifier.createContext(node.blockChain.getHeadBlock());
	uint64_t startTime = nowMilli();
	int transactionCount = 0;

	std::set<int> numbers;
	for (auto& i : node.blockChain.getPendingTransactions()) {
		Transaction transaction = node.blockChain.getTransaction(i);
		numbers.insert(transaction.header.transactionNumber);
	}

	for (auto& number : numbers) {
		for (auto& i : node.blockChain.getPendingTransactions()) {
			Transaction transaction = node.blockChain.getTransaction(i);
			if (transaction.header.transactionNumber == number) {
				VerifyContext tmp = context;
				TransactionError error = node.verifier.verifyTransaction(transaction, context);
				if (error == TransactionError::VALID) {
					node.creator.addTransaction(transaction);
					transactionCount++;
				}
				else {
					log(LogLevel::INFO, "Validator", "invalid transaction %s: %s", toHex(transaction.transactionHash).c_str(), transactionErrorToString(error));
					context = tmp;
					checkPendingTransaction(transaction, error);
				}

				if (transactionCount >= maxTrasnactionCount) {
					break;
				}

				if (nowMilli() - startTime > 2000) {
					break;
				}
			}
		}

		if (transactionCount >= maxTrasnactionCount) {
			break;
		}

		if (nowMilli() - startTime > 2000) {
			break;
		}
	}

	Block block = node.creator.endBlock();
	if (block.header.transactionCount == 0 && !createEmptyBlocks) {
		return;
	}

	block.header.sign(keyStore.getPrivateKey());
	block.blockHash = block.header.caclulateHash();

	BlockError error = node.verifier.verifyBlock(block, time(nullptr));
	if (error == BlockError::VALID) {
		node.blockChain.addBlock(block);
		BlockMetaData meta;
		meta.received = time(nullptr);
		meta.lastCheck = error;
		node.blockChain.setMetaData(block.blockHash, meta);

		log(LogLevel::INFO, "Validator", "created block num=%i slot=%i txCount=%i hash=%s", block.header.blockNumber, block.header.slot, block.header.transactionCount, toHex(block.blockHash).c_str());
		
		for (auto& hash : block.transactionTree.transactionHashes) {
			node.blockChain.removePendingTransaction(hash);
		}
		if (block.header.previousBlockHash == node.blockChain.getHeadBlock()) {
			node.blockChain.setHeadBlock(block.blockHash);
		}
		node.network.sendBlock(block);
		
	}
	else {
		log(LogLevel::INFO, "Validator", "invalid block %s: %s", toHex(block.blockHash).c_str(), blockErrorToString(error));
	}
}

void Validator::checkPendingTransaction(const Transaction& transaction, TransactionError result) {
	bool keep = false;
	if (result == TransactionError::INVALID_TRANSACTION_NUMBER) {
		uint32_t transactionCount = node.blockChain.getAccountTree().get(transaction.header.sender).transactionCount;
		if (transaction.header.transactionNumber >= transactionCount) {
			keep = true;
		}
	}
	if (result == TransactionError::INVALID_BALANCE) {
		keep = true;
	}
	if (!keep) {
		invalidPendingTransactions.push_back(transaction.transactionHash);
	}
}
