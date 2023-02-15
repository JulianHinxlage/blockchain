#pragma once

#include "network/PeerNetwork.h"
#include "DataBase.h"
#include <sstream>

class ChainNetwork {
public:
	enum class Opcode {
		NOOP,

		BLOCKS,
		BLOCKS_HEADER,
		TRANSACTIONS,
		TRANSACTIONS_HEADER,
		STATE_DATA,
		BLOCK_HASHES,

		REQUEST_BLOCKS,
		REQUEST_BLOCKS_HEADER,
		REQUEST_TRANSACTIONS,
		REQUEST_TRANSACTIONS_HEADER,
		REQUEST_STATE_DATA,
		REQUEST_BLOCK_HASHES,
	};

	PeerNetwork network;
	DataBase* db;
	BlockChain* chain;
	std::function<void(const Block&)> onBlockReceived;
	std::function<void(const Transaction&, bool partOfBlock)> onTransactionReceived;

	ChainNetwork();
	void init(const std::string &entryNodeFile);
	void shutdown();

	void broadcastBlock(const Block& block);
	void broadcastTransaction(const Transaction& transaction);

	void syncChain();

private:
	enum class Mode {
		NONE,
		SYNCHRONISATION,
		LISTENING,
	};
	Mode mode;
	std::mutex syncMutex;
	std::condition_variable syncCv;

	void onMessage(const std::string &msg, PeerId source);
	void packTransaction(Packet& packet, const Transaction& transaction);
	void packBlock(Packet &packet, const Block& block);
};
