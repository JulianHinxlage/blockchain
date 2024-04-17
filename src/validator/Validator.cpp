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

	node.onTransactionRecived = [&](const Transaction& transaction) {
		pendingTransactions[transaction.transactionHash] = transaction;
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
	log(LogLevel::DEBUG, "Validator", "chain head: num=%i %s", node.blockChain.getBlockCount() - 1, toHex(node.blockChain.getHeadBlock()).c_str());

	running = true;
	thread = new std::thread([&]() {
		while (running) {
			epoch();
		}
	});
}

uint64_t nowMilli() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void Validator::epoch() {
	uint64_t number = node.blockChain.getBlockCount();
	uint64_t slotTime = node.blockChain.config.slotTime;

	while (number == 1 && pendingTransactions.size() == 0) {
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
		log(LogLevel::INFO, "Validator", "for slot %i validator %s was selected", slot, toHex(validator).c_str());
		if (validator == keyStore.getPublicKey() || validator == EccPublicKey(0)) {
			log(LogLevel::INFO, "Validator", "###### local key selected as validator for block %i slot %i ######", number, slot);
		}

		bool slotWasSuccessfull = false;
		bool creadedBlock = false;
		for (int i = timePastInSlotMillis / 1000; i < slotTime; i++) {

			if (!running) {
				return;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			if (!running) {
				return;
			}

			if (node.blockChain.getBlockCount() > number) {
				slotWasSuccessfull = true;
				break;
			}

			if (!creadedBlock && (validator == keyStore.getPublicKey() || validator == EccPublicKey(0))) {
				createBlock(slot, slotBeginTime);
				creadedBlock = true;
			}
		}

		if (slotWasSuccessfull) {
			log(LogLevel::INFO, "Validator", "end epoch %i", number);
			break;
		}
	}
}

void Validator::createBlock(uint32_t slot, uint64_t slotBeginTime) {
	uint64_t timestamp = time(nullptr);

	timestamp = std::max(slotBeginTime, timestamp);
	timestamp = std::min(slotBeginTime + node.blockChain.config.slotTime - 1, timestamp);

	node.creator.beginBlock(keyStore.getPublicKey(), slot, timestamp);

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
	if (block.header.transactionCount == 0 && !createEmptyBlocks) {
		return;
	}

	block.header.sign(keyStore.getPrivateKey());
	block.blockHash = block.header.caclulateHash();

	BlockError error = node.verifier.verifyBlock(block, time(nullptr));
	if (error == BlockError::VALID) {
		node.blockChain.addBlock(block);
		log(LogLevel::INFO, "Validator", "created block num=%i slot=%i %s", block.header.blockNumber, block.header.slot, toHex(block.blockHash).c_str());
		
		if (block.header.previousBlockHash == node.blockChain.getHeadBlock()) {
			node.blockChain.setHeadBlock(block.blockHash);
		}
		node.network.sendBlock(block);
		
		for (auto& hash : block.transactionTree.transactionHashes) {
			pendingTransactions.erase(hash);
		}
	}
	else {
		log(LogLevel::INFO, "Validator", "invalid block %s: %s", toHex(block.blockHash).c_str(), blockErrorToString(error));
	}
}
