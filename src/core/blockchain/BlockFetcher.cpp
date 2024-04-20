//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "BlockFetcher.h"
#include "util/log.h"

void BlockFetcher::start() {
	postFetchQueue.onOutput = [&](const Hash& hash) {
		onPostFetch(hash);
	};
	postFetchQueue.start();

	fetchQueue.onOutput = [&](const Block& block) {
		fetchBlock(block);
	};
	fetchQueue.start();

	prefetchQueue.onOutput = [&](const Hash& hash) {
		prefetchOrder.push_back(hash);
		prefetchRequests.insert(hash);
		requestBlock(hash);
	};
	prefetchQueue.start();
}

void BlockFetcher::onBlockIn(const Block& block) {
	fetchQueue.onInput(block);
}

void BlockFetcher::fetchBlock(const Block& block) {
	if (!blockChain->hasBlock(block.blockHash)) {
		blockChain->addBlock(block);
		BlockMetaData meta;
		meta.received = time(nullptr);
		meta.lastCheck = BlockError::NOT_CHECKED;
		blockChain->setMetaData(block.blockHash, meta);

		log(LogLevel::DEBUG, "BlockFetcher", "received block num=%i tx=%i slot=%i hash=%s", block.header.blockNumber, block.header.transactionCount, block.header.slot, toHex(block.blockHash).c_str());
	}

	std::unique_lock<std::mutex> lock(mutex);

	//check if the block was prefetched and ensure the fetch process in the right order
	if (prefetchRequests.contains(block.blockHash)) {
		prefetchRequests.erase(block.blockHash);

		while (!prefetchOrder.empty()) {
			Hash fetch = prefetchOrder.front();
			if (!prefetchRequests.contains(fetch)) {
				prefetchOrder.pop_front();
				onBlockIn(blockChain->getBlock(fetch));
			}
			else {
				break;
			}
		}
		return;
	}

	if (pendingBlocks.contains(block.blockHash)) {
		return;
	}

	Pending& pending = pendingBlocks[block.blockHash];

	for (auto& hash : block.transactionTree.transactionHashes) {
		if (!blockChain->hasTransaction(hash)) {
			pending.pendingTransactionCount++;
			pendingTrasnaction[hash] = block.blockHash;
			requestTransaction(hash);
		}
	}

	Hash prev = block.header.previousBlockHash;
	if (prev != Hash(0)) {
		if (!blockChain->hasBlock(prev)) {
			pending.isPreviousPending = true;
			pendingPrevious[prev] = block.blockHash;
			requestBlock(prev);
		}
		else {
			if (pendingBlocks.contains(prev)) {
				pending.isPreviousPending = true;
				pendingPrevious[prev] = block.blockHash;
			}
		}
	}

	lock.unlock();
	checkPending(block.blockHash);
}

void BlockFetcher::checkPending(const Hash& blockHash){
	std::unique_lock<std::mutex> lock(mutex);
	auto i = pendingBlocks.find(blockHash);
	if (i != pendingBlocks.end()) {
		if (!i->second.isPreviousPending) {
			if (i->second.pendingTransactionCount <= 0) {
				pendingBlocks.erase(i);
				postFetchQueue.onInput(blockHash);
			}
		}
	}
}

void BlockFetcher::onPostFetch(const Hash& blockHash) {
	if (onBlockOut) {
		onBlockOut(blockChain->getBlock(blockHash));
	}

	std::unique_lock<std::mutex> lock(mutex);
	auto i = pendingPrevious.find(blockHash);
	if (i != pendingPrevious.end()) {
		auto j = pendingBlocks.find(i->second);
		if (j != pendingBlocks.end()) {
			j->second.isPreviousPending = false;
			Hash hash = j->first;
			pendingPrevious.erase(i);
			lock.unlock();
			checkPending(hash);
		}
	}

	if (blockHash == synchronizationTarget) {
		if (onSynchronized) {
			onSynchronized();
		}
	}
}

void BlockFetcher::onTransactionIn(const Transaction& transaction) {
	if (!blockChain->hasTransaction(transaction.transactionHash)) {
		blockChain->addTransaction(transaction);
	}

	std::unique_lock<std::mutex> lock(mutex);

	auto i = pendingTrasnaction.find(transaction.transactionHash);
	if (i != pendingTrasnaction.end()) {
		auto j = pendingBlocks.find(i->second);
		if (j != pendingBlocks.end()) {
			j->second.pendingTransactionCount--;
			Hash hash = j->first;
			pendingTrasnaction.erase(i);
			lock.unlock();
			checkPending(hash);
		}
	}
}

void BlockFetcher::synchronize() {
	network->getBlockHashes(blockChain->getBlockCount(), -1, [&](int begin, int end, const std::vector<Hash>& hashes, PeerId peer) {
		std::unique_lock<std::mutex> lock(mutex);
		if (hashes.size() > 0) {
			synchronizationTarget = hashes[hashes.size() - 1];
		}
		else {
			synchronizationTarget = blockChain->getHeadBlock();
			if (onSynchronized) {
				onSynchronized();
			}
		}
		for (auto& hash : hashes) {
			prefetchQueue.onInput(hash);
		}
	});
}

void BlockFetcher::requestTransaction(const Hash& hash, int tryCount) {
	if (tryCount > 3) {
		log(LogLevel::WARNING, "BlockFetcher", "transaction request failed %s", toHex(hash).c_str());
		return;
	}
	network->getTransactions({ hash }, [&, hash, tryCount](const std::vector<Transaction>& transactions, PeerId peer) {
		if (transactions.size() == 1) {
			onTransactionIn(transactions[0]);
		}
		else {
			log(LogLevel::INFO, "BlockFetcher", "retry transaction request %s", toHex(hash).c_str());
			requestTransaction(hash, tryCount + 1);
		}
	});
}

void BlockFetcher::requestBlock(const Hash& hash, int tryCount) {
	if (tryCount > 3) {
		log(LogLevel::WARNING, "BlockFetcher", "block request failed %s", toHex(hash).c_str());
		return;
	}
	network->getBlocks({ hash }, [&, hash, tryCount](const std::vector<Block>& blocks, PeerId peer) {
		if (blocks.size() == 1) {
			onBlockIn(blocks[0]);
		}
		else {
			log(LogLevel::INFO, "BlockFetcher", "retry block request %s", toHex(hash).c_str());
			requestBlock(hash, tryCount + 1);
		}
	});
}

