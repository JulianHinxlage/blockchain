//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "Node.h"
#include "util/log.h"

void Node::init(const std::string& chainDir, const std::string& entryNodeFile) {
	network.blockChain = &blockChain;
	creator.blockChain = &blockChain;
	verifier.blockChain = &blockChain;
	blockChain.init(chainDir);
	log(LogLevel::INFO, "Node", "chain loaded with %i blocks from directory %s", blockChain.getBlockCount(), chainDir.c_str());

	if (networkMode == NetworkMode::CLIENT) {
		network.init(PeerType::CLIENT, entryNodeFile);
	}
	else if (networkMode == NetworkMode::MULTI_CLIENT) {
		network.init(PeerType::MULTI_CLIENT, entryNodeFile);
	}
	else if (networkMode == NetworkMode::SERVER) {
		network.init(PeerType::SERVER, entryNodeFile);
	}

	network.onBlockRecived = [&](const Block& block) {
		fetcher.onBlockIn(block);
	};
	network.onTransactionRecived = [&](const Transaction& transaction) {
		if (!blockChain.hasTransaction(transaction.transactionHash)) {
			blockChain.addTransaction(transaction);
			blockChain.addPendingTransaction(transaction.transactionHash);
			if (onNewTransaction) {
				onNewTransaction(transaction);
			}
		}
	};
	network.onStateChanged = [&](NetworkState state) {
		if (state == NetworkState::CONNECTED) {
			if (synchronisationPending) {
				synchronize();
			}
		}
	};

	network.connect("127.0.0.1", 54000);

	fetcher.blockChain = &blockChain;
	fetcher.network = &network;
	fetcher.onBlockOut = [&](const Block &block) {
		verifyQueue.onInput(block);
	};
	verifyQueue.onOutput = [&](const Block& block) {
		verify(block);
	};
	verifyQueue.start();
	fetcher.start();
	state = NodeState::INIT;
}

void Node::verify(const Block& block) {
	BlockError result = BlockError::NOT_CHECKED;;
	BlockMetaData prevMeta = blockChain.getMetaData(block.header.previousBlockHash);
	if (prevMeta.lastCheck == BlockError::NOT_CHECKED) {
		if (blockChain.hasBlock(block.header.previousBlockHash)) {
			pendingVerifies[block.header.previousBlockHash] = block.blockHash;
			verifyQueue.onInput(blockChain.getBlock(block.header.previousBlockHash));
			return;
		}
		else {
			fetcher.onBlockIn(block);
			return;
		}
	}
	else if (prevMeta.lastCheck != BlockError::VALID) {
		result = BlockError::INVALID_PREVIOUS;
	}
	else {
		result = verifier.verifyBlock(block, time(nullptr));
	}

	BlockMetaData meta = blockChain.getMetaData(block.blockHash);
	meta.lastCheck = result;
	blockChain.setMetaData(block.blockHash, meta);

	if (result == BlockError::VALID) {
		BlockHeader head = blockChain.getBlockHeader(blockChain.getHeadBlock());
		if (blockChain.consensus.forkChoice(head, block.header)) {
			blockChain.setHeadBlock(block.blockHash);
			log(LogLevel::INFO, "Node", "new cain head num=%i tx=%i slot=%i hash=%s", block.header.blockNumber, block.header.transactionCount, block.header.slot, toHex(block.blockHash).c_str());
			for (auto& hash : block.transactionTree.transactionHashes) {
				blockChain.removePendingTransaction(hash);
			}
			if (onNewBlock) {
				onNewBlock(block);
			}
		}
		else {
			log(LogLevel::INFO, "Node", "valid block num=%i tx=%i slot=%i hash=%s", block.header.blockNumber, block.header.transactionCount, block.header.slot, toHex(block.blockHash).c_str());
			for (auto& hash : block.transactionTree.transactionHashes) {
				blockChain.removePendingTransaction(hash);
			}
		}
	}
	else {
		log(LogLevel::INFO, "Node", "invalid block error=%s num=%i slot=%i hash=%s", blockErrorToString(result), block.header.blockNumber, block.header.slot, toHex(block.blockHash).c_str());
	}

	auto i = pendingVerifies.find(block.blockHash);
	if (i != pendingVerifies.end()) {
		Hash hash = i->second;
		pendingVerifies.erase(i);
		verifyQueue.onInput(blockChain.getBlock(hash));
	}

	if (state == SYNCHRONISING_VERIFY) {
		if (verifyQueue.empty()) {
			state = NodeState::RUNNING;
			log(LogLevel::INFO, "Node", "chain synchronisation finished");
		}
	}
}

void Node::synchronize() {
	if (network.getState() != NetworkState::CONNECTED) {
		synchronisationPending = true;
		return;
	}
	synchronisationPending = false;
	state = NodeState::SYNCHRONISING_FETCH;
	log(LogLevel::INFO, "Node", "chain synchronisation started");

	fetcher.onSynchronized = [&]() {
		if (state == NodeState::SYNCHRONISING_FETCH) {
			state = NodeState::SYNCHRONISING_VERIFY;
			if (verifyQueue.empty()) {
				state = NodeState::RUNNING;
				log(LogLevel::INFO, "Node", "chain synchronisation finished");
			}
		}
	};
	fetcher.synchronize();
}

void Node::synchronizePendingTransactions() {
	network.getPendingTransactions([&](const std::vector<Hash>& hashes, PeerId peer) {
		for (auto& hash : hashes) {
			if (!blockChain.hasTransaction(hash)) {
				network.getTransactions({ hash }, [&](const std::vector<Transaction>& transactions, PeerId peer) {
					for (auto &transaction : transactions) {
						if (network.onTransactionRecived) {
							network.onTransactionRecived(transaction);
						}
					}
				});
			}
			else {
				blockChain.addPendingTransaction(hash);
			}
		}
	});
}

void Node::verifyChain() {
	state = NodeState::VERIFY_CHAIN;
	log(LogLevel::INFO, "Node", "start verifying blockchain");
	int count = blockChain.getBlockCount();
	int maxValidBlock = -1;
	for (int i = 0; i < count; i++) {
		Hash hash = blockChain.getBlockHash(i);
		Block block = blockChain.getBlock(hash);

		if (block.header.caclulateHash() != hash) {
			log(LogLevel::INFO, "Node", "hash error");
			break;
		}

		BlockError result = verifier.verifyBlock(block, time(nullptr));
		if (result != BlockError::VALID) {
			log(LogLevel::INFO, "Node", "invalid block error=%s num=%i slot=%i hash=%s", blockErrorToString(result), block.header.blockNumber, block.header.slot, toHex(block.blockHash).c_str());
			break;
		}

		maxValidBlock = i;
	}

	if (maxValidBlock != count - 1) {
		log(LogLevel::INFO, "Node", "cut chain to %i block", maxValidBlock + 1);

		for (int i = maxValidBlock + 1; i < blockChain.getBlockCount(); i++) {
			Hash hash = blockChain.getBlockHash(i);
			BlockMetaData meta = blockChain.getMetaData(hash);
			meta.lastCheck = BlockError::INVALID_PREVIOUS;
			blockChain.setMetaData(hash, meta);
		}

		if (maxValidBlock == -1) {
			blockChain.setHeadBlock(blockChain.config.genesisBlockHash);
		}
		else {
			blockChain.setHeadBlock(blockChain.getBlockHash(maxValidBlock));
		}
	}
	log(LogLevel::INFO, "Node", "finished verifying blockchain");
	state = NodeState::INIT;
}

NodeState Node::getState() {
	return state;
}
