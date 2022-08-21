#include "ChainNetwork.h"
#include "util/strutil.h"
#include <fstream>

ChainNetwork::ChainNetwork() {
	mode = Mode::NONE;
	chain = nullptr;
	db = nullptr;
}

void ChainNetwork::init(const std::string& entryNodeFile) {
	std::ifstream stream;
	stream.open(entryNodeFile);
	std::string line;
	while (std::getline(stream, line)) {
		std::string ip = "127.0.0.1";
		uint16_t port = 6000;
		auto parts = strSplit(line, ":", false);
		if (parts.size() > 0) {
			ip = parts[0];
		}
		if (parts.size() > 1) {
			try {
				port = std::stoi(parts[1]);
			}
			catch (...) {}
		}
		network.addEntryNode(Endpoint(ip, port));
	}

	network.msgCallback = [&](const std::string &msg, PeerId source) {
		onMessage(msg, source);
	};

	mode = Mode::LISTENING;
	network.start(6000, "127.0.0.1", 32);
	network.connect();
}

void ChainNetwork::shutdown() {
	network.disconnect();
	network.stop();
}

void ChainNetwork::broadcastBlock(const Block& block) {
	Packet packet;
	packet.add(Opcode::BLOCKS);
	packet.add((int)1);
	packBlock(packet, block);
	network.broadcast(packet.remaining());
}

void ChainNetwork::broadcastTransaction(const Transaction& transaction) {
	Packet packet;
	packet.add(Opcode::TRANSACTIONS);
	packet.add((int)1);
	packTransaction(packet, transaction);
	network.broadcast(packet.remaining());
}

void ChainNetwork::packTransaction(Packet& packet, const Transaction& transaction) {
	std::stringstream stream;
	transaction.serial(stream);
	std::string data = stream.str();
	packet.add((int)data.size());
	packet.add(data.data(), data.size());
}

void ChainNetwork::packBlock(Packet& packet, const Block& block) {
	std::stringstream stream;
	block.serial(stream);
	std::string data = stream.str();
	packet.add((int)data.size());
	packet.add(data.data(), data.size());

	int txCount = block.transactionTree.transactionHashes.size();
	packet.add((int)txCount);
	Transaction transaction;
	for (int i = 0; i < txCount; i++) {
		db->getTransaction(block.transactionTree.transactionHashes[i], transaction);
		packTransaction(packet, transaction);
	}
}

void ChainNetwork::onMessage(const std::string& msg, PeerId source) {
	Packet packet;
	packet.add((char*)msg.data(), msg.size());

	Opcode opcode = packet.get<Opcode>();

	switch (opcode) {
	case Opcode::BLOCKS: {
		int count = packet.get<int>();
		for (int i = 0; i < count; i++) {
			int size = packet.get<int>();
			Block block;
			std::stringstream stream;
			stream.write(packet.data(), size);
			packet.skip(size);
			block.deserial(stream);

			int txCount = packet.get<int>();
			Transaction transaction;
			for (int i = 0; i < txCount; i++) {
				int size = packet.get<int>();
				std::stringstream stream;
				stream.write(packet.data(), size);
				packet.skip(size);
				transaction.deserial(stream);

				if (onTransactionReceived) {
					onTransactionReceived(transaction, true);
				}
			}

			if (onBlockReceived) {
				onBlockReceived(block);
			}
		}
		if (mode == Mode::SYNCHRONISATION) {
			syncCv.notify_one();
		}
		break;
	}
	case Opcode::TRANSACTIONS: {
		int count = packet.get<int>();
		for (int i = 0; i < count; i++) {
			int size = packet.get<int>();
			Transaction transaction;
			std::stringstream stream;
			stream.write(packet.data(), size);
			packet.skip(size);
			transaction.deserial(stream);

			if (onTransactionReceived) {
				onTransactionReceived(transaction, false);
			}
		}
		break;
	}
	case Opcode::BLOCK_HASHES: {
		int count = packet.get<int>();
		int blockNumber = packet.get<int>();
		
		std::vector<Hash> hashes;
		for (int i = 0; i < count; i++) {
			Hash hash = packet.get<Hash>();
			hashes.push_back(hash);
		}

		int newCount = 0;
		for (int num = blockNumber; num >= 0; num--) {
			if (newCount >= hashes.size()) {
				break;
			}
			if (chain->blocks.size() > num) {
				if (chain->blocks[num] == hashes[hashes.size() - 1 -  newCount]) {
					break;
				}
			}
			newCount++;
		}

		if (mode == Mode::SYNCHRONISATION) {
			Packet response;
			response.add(Opcode::REQUEST_BLOCKS);
			response.add(newCount);
			if (hashes.size() > 0) {
				response.add(hashes[hashes.size() - 1]);
			}
			network.send(response.remaining(), source);
		}

		break;
	}
	case Opcode::REQUEST_BLOCKS: {
		int count = packet.get<int>();
		Hash lastBlock = packet.get<Hash>();

		Hash hash = lastBlock;
		Block block;



		std::vector<Hash> hashes;
		for (int i = 0; i < count; i++) {
			if (db->getBlock(hash, block)) {
				hashes.push_back(hash);
				hash = block.header.previousBlock;
				if (block.header.blockNumber == 0) {
					break;
				}
			}
		}

		Packet response;
		response.add(Opcode::BLOCKS);
		response.add((int)hashes.size());

		for (int i = hashes.size() - 1; i >= 0; i--) {
			hash = hashes[i];
			if (db->getBlock(hash, block)) {
				packBlock(response, block);
				if (block.header.blockNumber == 0) {
					break;
				}
			}
		}

		network.send(response.remaining(), source);
		break;
	}
	case Opcode::REQUEST_TRANSACTIONS: {
		int count = packet.get<int>();
		Hash hash = packet.get<Hash>();

		Packet response;
		response.add(Opcode::TRANSACTIONS);
		response.add((int)1);

		Transaction transaction;
		if (db->getTransaction(hash, transaction)) {
			packTransaction(packet, transaction);
		}
		network.send(response.remaining(), source);
		break;
	}
	case Opcode::REQUEST_BLOCK_HASHES: {
		int count = packet.get<int>();
		int blockNumber = packet.get<int>();
		Hash hash = packet.get<Hash>();


		if (blockNumber != -1) {
			if (blockNumber >= 0 && blockNumber < chain->blocks.size()) {
				hash = chain->blocks[blockNumber];
			}
		}
		else {
			if (hash == Hash(0)) {
				hash = chain->getLatestBlock();
			}
		}

		int index = -1;
		for (int i = chain->blocks.size() - 1; i >= 0; i--) {
			if (chain->blocks[i] == hash) {
				index = i;
				break;
			}
		}

		if (count == -1) {
			count = index + 1;
		}

		if (index != -1) {

			count = std::min(count, index + 1);

			Packet response;
			response.add(Opcode::BLOCK_HASHES);
			response.add((int)count);
			response.add((int)index);//block number

			for (int i = 0; i < count; i++) {
				response.add(chain->blocks[index - (count - 1) + i]);
			}

			network.send(response.remaining(), source);
		}
		else {
			Packet response;
			response.add(Opcode::BLOCK_HASHES);
			response.add((int)0);
			response.add((int)-1);
			network.send(response.remaining(), source);
		}
		break;
	}
	default:
		break;
	}
}

void ChainNetwork::syncChain() {
	if (network.isConnected()) {
		std::unique_lock<std::mutex> lock(syncMutex);
		mode = Mode::SYNCHRONISATION;

		Packet packet;
		packet.add(Opcode::REQUEST_BLOCK_HASHES);
		packet.add((int)-1);//count
		packet.add((int)-1);//block number
		packet.add(Hash(0));//block hash

		PeerId neighbor = network.getRandomNeighbor();
		network.send(packet.remaining(), neighbor);

		syncCv.wait_for(lock, std::chrono::milliseconds(5000));

		mode = Mode::LISTENING;
	}
}
