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
		onBlock(block);
	};
	network.onTransactionRecived = [&](const Transaction& transaction) {
		onTransaction(transaction);
	};
	network.onStateChanged = [&](NetworkState state) {
		if (state == NetworkState::CONNECTED) {
			if (synchronisationPending) {
				synchronize();
			}
		}
	};

	network.connect("127.0.0.1", 54000);
}

void Node::synchronize() {
	if (network.getState() != NetworkState::CONNECTED) {
		synchronisationPending = true;
		return;
	}
	synchronisationPending = false;

	network.getBlockHashes(-1, -1, [&](int begin, int end, const std::vector<Hash>& hashes, PeerId peer) {
		if (hashes.size() != end - begin) {
			return;
		}

		std::vector<Hash> request;
		for (int i = 0; i < hashes.size(); i++) {
			Hash hash = hashes[i];

			if (blockChain.hasBlock(hash)) {
				prepareBlock(blockChain.getBlock(hash));
				continue;
			}

			request.push_back(hash);

			if (request.size() == 100 || i == hashes.size() - 1) {
				network.getBlocks(request, [&](const std::vector<Block>& blocks, PeerId peer) {
					for (const Block &block : blocks) {
						onBlock(block);
					}
				}, peer);
				request.clear();
			}
		}
	});
}

void Node::verifyChain() {
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

		if (!verifyBlock(block)) {
			break;
		}

		maxValidBlock = i;
	}

	if (maxValidBlock != blockChain.getBlockCount() - 1) {
		log(LogLevel::INFO, "Node", "cut chain to %i block", maxValidBlock + 1);

		if (maxValidBlock == -1) {
			blockChain.setHeadBlock(blockChain.config.genesisBlockHash);
		}
		else {
			blockChain.setHeadBlock(blockChain.getBlockHash(maxValidBlock));
		}
	}
	log(LogLevel::INFO, "Node", "finished verifying blockchain");
}

void Node::onBlock(const Block& block) {
	log(LogLevel::DEBUG, "Node", "received block: num=%i %s", (int)block.header.blockNumber, toHex(block.blockHash).c_str());
	if (verifyMode == VerifyMode::FULL_VERIFY) {
		prepareBlock(block);
	}
	else {
		onVerifiedBlock(block);
	}
}

void Node::onTransaction(const Transaction& transaction) {
	log(LogLevel::DEBUG, "Node", "received transaction: %s", toHex(transaction.transactionHash).c_str());
	blockChain.addTransaction(transaction);

	auto i = blockByTrasnaction.find(transaction.transactionHash);
	if (i != blockByTrasnaction.end()) {
		auto j = pendingBlocks.find(i->second);
		if (j != pendingBlocks.end()) {
			if (prepareBlock(j->second, true)) {
				pendingBlocks.erase(i->second);
				blockByTrasnaction.erase(transaction.transactionHash);
			}
			return;
		}
	}

	log(LogLevel::INFO, "Node", "new transaction: %s", toHex(transaction.transactionHash).c_str());
	if (onTransactionRecived) {
		onTransactionRecived(transaction);
	}
}

bool Node::prepareBlock(const Block& block, bool onlyCheck) {
	bool isPrepared = true;
	Hash prev = block.header.previousBlockHash;
	if (prev != Hash(0) && !blockChain.hasBlock(prev)) {
		isPrepared = false;
		if (!onlyCheck) {
			if (!blockByPrev.contains(prev) && !pendingBlocks.contains(prev)) {
				blockByPrev[prev] = block.blockHash;
				network.getBlocks({prev}, [&](const std::vector<Block>& blocks, PeerId peer) {
					if (blocks.size() == 1) {
						onBlock(blocks[0]);
					}
				});
			}
		}
	}
	for (auto& hash : block.transactionTree.transactionHashes) {
		if (!blockChain.hasTransaction(hash)) {
			isPrepared = false;
			if (!onlyCheck) {
				if (!blockByTrasnaction.contains(hash)) {
					blockByTrasnaction[hash] = block.blockHash;
					network.getTransactions({ hash }, [&](const std::vector<Transaction>& transactions, PeerId peer) {
						if (transactions.size() == 1) {
							onTransaction(transactions[0]);
						}
					});
				}
			}
		}
	}

	if (!isPrepared) {
		if (!onlyCheck) {
			pendingBlocks[block.blockHash] = block;
		}
		return false;
	}
	else {
		if (verifyBlock(block)) {
			onVerifiedBlock(block);

			auto i = blockByPrev.find(block.blockHash);
			if (i != blockByPrev.end()) {
				auto j = pendingBlocks.find(i->second);
				if (j != pendingBlocks.end()) {
					if (prepareBlock(j->second, true)) {
						pendingBlocks.erase(i->second);
						blockByPrev.erase(block.blockHash);
					}
				}
			}

		}
		return true;
	}
}

bool Node::verifyBlock(const Block& block) {
	std::unique_lock<std::mutex> lock(verifyMutex);
	BlockError error = verifier.verifyBlock(block, time(nullptr));
	if (error != BlockError::VALID) {
		log(LogLevel::DEBUG, "Node", "invalid block %s: %s", toHex(block.blockHash).c_str(), blockErrorToString(error));
		return false;
	}
	return true;
}

void Node::onVerifiedBlock(const Block& block) {
	blockChain.addBlock(block);
	if (checkForTip(block)) {
		log(LogLevel::INFO, "Node", "new block: num=%i %s", (int)block.header.blockNumber, toHex(block.blockHash).c_str());
		if (onBlockRecived) {
			onBlockRecived(block);
		}
	}
}

bool Node::checkForTip(const Block& block) {
	BlockHeader head = blockChain.getBlock(blockChain.getHeadBlock()).header;
	if (blockChain.consensus.forkChoice(head, block.header)) {
		blockChain.setHeadBlock(block.blockHash);
		return true;
	}
	return false;
}
