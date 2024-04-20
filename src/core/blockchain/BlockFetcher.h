//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "BlockChain.h"
#include "Network.h"
#include "util/ThreadedQueue.h"
#include <deque>

class BlockFetcher {
public:
	BlockChain* blockChain;
	Network* network;

	std::function<void(const Block&)> onBlockOut;
	std::function<void()> onSynchronized;

	void start();
	void onBlockIn(const Block& block);
	void onTransactionIn(const Transaction &transaction);
	void synchronize();

private:
	class Pending {
	public:
		bool isPreviousPending = false;
		int pendingTransactionCount = 0;
	};
	std::map<Hash, Pending> pendingBlocks;
	std::map<Hash, Hash> pendingTrasnaction;
	std::map<Hash, Hash> pendingPrevious;
	std::set<Hash> prefetchRequests;
	std::deque<Hash> prefetchOrder;
	std::mutex mutex;
	ThreadedQueue<Hash> postFetchQueue;
	ThreadedQueue<Block> fetchQueue;
	ThreadedQueue<Hash> prefetchQueue;
	Hash synchronizationTarget;

	void requestTransaction(const Hash& hash, int tryCount = 0);
	void requestBlock(const Hash& hash, int tryCount = 0);

	void fetchBlock(const Block& block);
	void checkPending(const Hash& blockHash);
	void onPostFetch(const Hash& blockHash);
};
