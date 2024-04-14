//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "Network.h"
#include "util/Serializer.h"
#include "BlockChain.h"
#include "util/random.h"
#include "util/util.h"
#include "util/log.h"
#include <fstream>

void RequestContenxt::add(RequestId id, const std::function<void(NetworkOpcode, Serializer&)>& callback) {
	std::unique_lock<std::mutex> lock(mutex);
	requests[id] = { id, (uint32_t)time(nullptr), callback };
	cv.notify_one();
}

bool RequestContenxt::onRequest(RequestId id, NetworkOpcode opcode, Serializer& serial) {
	std::unique_lock<std::mutex> lock(mutex);
	auto i = requests.find(id);
	if (i != requests.end()) {
		if (i->second.callback) {
			lock.unlock();
			i->second.callback(opcode, serial);
			lock.lock();
		}
		requests.erase(id);
		return true;
	}
	return false;
}

void RequestContenxt::start() {
	if (thread) {
		stop();
	}

	running = true;
	thread = new std::thread([&]() {
		while (running) {
			std::unique_lock<std::mutex> lock(mutex);
			uint32_t now = time(nullptr);
			uint32_t min = -1;
			bool exectuedTimout = false;
			for (auto& request : requests) {
				uint32_t value = request.second.createTime + timeoutTime;
				if (value <= now) {
					if (request.second.callback) {
						Serializer serial;
						lock.unlock();
						request.second.callback(NetworkOpcode::TIMEOUT, serial);
						lock.lock();
						requests.erase(request.first);
						exectuedTimout = true;
						break;
					}
				}

				if (min == -1 || min < value) {
					min = value;
				}
			}

			if (!exectuedTimout) {
				if (min != -1) {
					cv.wait_for(lock, std::chrono::seconds(min - now));
				}
				else {
					cv.wait(lock);
				}
			}
		}
	});
}

void RequestContenxt::stop() {
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

Network::~Network() {
	disconnect();
}

void Network::init(PeerType type, const std::string& entryNodeFile) {
	state = NetworkState::DISCONNECTED;
	this->type = type;

	std::ifstream stream(entryNodeFile);
	if (stream.is_open()) {
		std::string line;
		while (std::getline(stream, line)) {
			auto parts = split(line, " ", false);
			if (parts.size() > 0) {
				std::string address = parts[0];
				int port = 54000;
				if (parts.size() > 1) {
					try {
						port = std::stoi(parts[1]);
					}
					catch (...) {}
				}
				network.addEntryNode(address, port);
			}
		}
	}
}

void Network::connect(const std::string& address, uint16_t port) {
	for (int i = 0; i < 10; i++) {
		network.addEntryNode(address, port + i);
	}

	network.onStateChanged = [&](PeerNetworkState state) {
		if (state == PeerNetworkState::CONNECTED) {
			changeState(NetworkState::CONNECTED);
		}
		else if(state == PeerNetworkState::DISCONNECTED) {
			changeState(NetworkState::DISCONNECTED);
		}
	};

	network.messageCallback = [&](const std::string& msg, PeerId source, bool broadcast) {
		onMessage(msg, source);
	};


	changeState(NetworkState::JOINING);
	requestContext.start();
	if (connectThread) {
		disconnect();
	}
	connectThread = new std::thread([&, address, port]() {
		network.connect(address, port, type);
	});
}

void Network::disconnect() {
	requestContext.stop();
	network.disconnect();
	if (connectThread) {
		if (connectThread->joinable()) {
			connectThread->join();
		}
		else {
			connectThread->detach();
		}
		delete connectThread;
		connectThread = nullptr;
	}
}

void Network::sendBlock(const Block& block) {
	Serializer request;
	request.write(NetworkOpcode::BLOCK_BROADCAST);
	request.write(random<RequestId>());
	request.writeStr(block.serial());
	network.broadcast(request.toString());
}

void Network::sendTransaction(const Transaction& transaction) {
	Serializer request;
	request.write(NetworkOpcode::TRANSACTION_BROADCAST);
	request.write(random<RequestId>());
	request.writeStr(transaction.serial());
	network.broadcast(request.toString());
}

NetworkState Network::getState() {
	return state;
}

void Network::changeState(NetworkState newState) {
	if (state != newState) {
		state = newState;
		if (onStateChanged) {
			onStateChanged(state);
		}
	}
}

void Network::onMessage(const std::string& msg, PeerId source) {
	Serializer request(msg);
	NetworkOpcode opcode = request.read<NetworkOpcode>();
	RequestId requestId = request.read<RequestId>();

	if (!requestContext.onRequest(requestId, opcode, request)) {
		if (opcode == NetworkOpcode::BLOCK_HASH_REQUEST) {
			int blockNumberBegin = request.read<int>();
			int blockNumberEnd = request.read<int>();

			if (blockNumberBegin == -1) {
				blockNumberBegin = blockChain->getBlockCount() - 1;
			}
			if (blockNumberEnd == -1) {
				blockNumberEnd = blockChain->getBlockCount();
			}

			if (blockNumberBegin >= 0 && blockNumberBegin < blockChain->getBlockCount()) {
				if (blockNumberEnd > 0 && blockNumberEnd >= blockNumberBegin) {
					Serializer reply;
					reply.write(NetworkOpcode::BLOCK_HASH_REPLY);
					reply.write(requestId);
					reply.write(blockNumberBegin);
					reply.write(blockNumberEnd);
					for (int i = blockNumberBegin; i < blockNumberEnd; i++) {
						reply.write(blockChain->getBlockHash(i));
					}
					network.send(source, reply.toString());
					return;
				}
			}
		}
		else if (opcode == NetworkOpcode::BLOCK_REQUEST) {
			int count = request.read<int>();
			if (count > 0 && count < 100) {

				Serializer reply;
				reply.write(NetworkOpcode::BLOCK_REPLY);
				reply.write(requestId);
				reply.write(count);

				bool fail = false;
				for (int i = 0; i < count; i++) {
					Hash hash = request.read<Hash>();
					if (!blockChain->hasBlock(hash)) {
						fail = true;
						break;
					}
					Block block = blockChain->getBlock(hash);
					std::string data = block.serial();
					reply.writeStr(data);
				}
				if (!fail) {
					network.send(source, reply.toString());
					return;
				}
			}
		}
		else if (opcode == NetworkOpcode::TRANSACTION_REQUEST) {
			int count = request.read<int>();
			if (count > 0 && count < 100) {

				Serializer reply;
				reply.write(NetworkOpcode::TRANSACTION_REPLY);
				reply.write(requestId);
				reply.write(count);

				bool fail = false;
				for (int i = 0; i < count; i++) {
					Hash hash = request.read<Hash>();
					if (!blockChain->hasTransaction(hash)) {
						fail = true;
						break;
					}
					Transaction transaction = blockChain->getTransaction(hash);
					std::string data = transaction.serial();
					reply.writeStr(data);
				}
				if (!fail) {
					network.send(source, reply.toString());
					return;
				}
			}
		}
		else if (opcode == NetworkOpcode::ACCOUNT_REQUEST) {
			Hash treeRoot = request.read<Hash>();
			EccPublicKey address = request.read<EccPublicKey>();
			if (blockChain->accountTree.reset(treeRoot)) {
				Account account = blockChain->accountTree.get(address);
				Serializer reply;
				reply.write(NetworkOpcode::ACCOUNT_REPLY);
				reply.write(requestId);
				reply.write(account);
				network.send(source, reply.toString());
				return;
			}
		}
		else if (opcode == NetworkOpcode::BLOCK_BROADCAST) {
			std::string data;
			request.readStr(data);
			Block block;
			block.deserial(data);
			block.blockHash = block.header.caclulateHash();
			if (onBlockRecived) {
				onBlockRecived(block);
			}
			return;
		}
		else if (opcode == NetworkOpcode::TRANSACTION_BROADCAST) {
			std::string data;
			request.readStr(data);
			Transaction transaction;
			transaction.deserial(data);
			transaction.transactionHash = transaction.header.caclulateHash();
			if (onTransactionRecived) {
				onTransactionRecived(transaction);
			}
			return;
		}
		else if (opcode == NetworkOpcode::REQUEST_ERROR) {
			log(LogLevel::TRACE, "Network", "REQUEST_ERROR");
			return;
		}
		else {
			log(LogLevel::TRACE, "Network", "invalid opcode %i", (int)opcode);
		}

		Serializer reply;
		reply.write(NetworkOpcode::REQUEST_ERROR);
		reply.write(requestId);
		network.send(source, reply.toString());
	}
}

void Network::getBlockHashes(int begin, int end, const std::function<void(int, int, const std::vector<Hash>&, PeerId)>& callback, PeerId peer) {
	if (peer == PeerId(0)) {
		peer = network.getRandomNeighbor();
	}
	Serializer request;
	RequestId requestId = random<RequestId>();
	request.write(NetworkOpcode::BLOCK_HASH_REQUEST);
	request.write(requestId);
	request.write<int>(begin);
	request.write<int>(end);
	requestContext.add(requestId, [callback, peer](NetworkOpcode opcode, Serializer& request) {
		if (opcode == NetworkOpcode::BLOCK_HASH_REPLY) {
			int begin = request.read<int>();
			int end = request.read<int>();

			if (begin >= 0 && end >= 0) {
				std::vector<Hash> hashes;
				for (int i = begin; i < end; i++) {
					hashes.push_back(request.read<Hash>());
				}

				if (callback) {
					callback(begin, end, hashes, peer);
				}
				return;
			}
		}
		std::vector<Hash> hashes;
		if (callback) {
			callback(0, 0, hashes, peer);
		}
	});
	network.send(peer, request.toString());
}

void Network::getBlocks(const std::vector<Hash>& blockHashes, const std::function<void(const std::vector<Block>&, PeerId)>& callback, PeerId peer) {
	if (peer == PeerId(0)) {
		peer = network.getRandomNeighbor();
	}
	Serializer request;
	RequestId requestId = random<RequestId>();
	request.write(NetworkOpcode::BLOCK_REQUEST);
	request.write(requestId);
	request.write<int>(blockHashes.size());
	for (auto& hash : blockHashes) {
		request.write<Hash>(hash);
	}
	requestContext.add(requestId, [callback, peer](NetworkOpcode opcode, Serializer& request) {
		if (opcode == NetworkOpcode::BLOCK_REPLY) {
			int count = request.read<int>();

			if (count >= 0) {
				std::vector<Block> blocks;
				for (int i = 0; i < count; i++) {
					std::string data;
					request.readStr(data);
					Block block;
					block.deserial(data);
					block.blockHash = block.header.caclulateHash();
					blocks.push_back(block);
				}
				
				if (callback) {
					callback(blocks, peer);
				}
				return;
			}
		}
		std::vector<Block> blocks;
		if (callback) {
			callback(blocks, peer);
		}
	});
	network.send(peer, request.toString());
}

void Network::getTransactions(const std::vector<Hash>& transactionHashs, const std::function<void(const std::vector<Transaction>&, PeerId)>& callback, PeerId peer) {
	if (peer == PeerId(0)) {
		peer = network.getRandomNeighbor();
	}
	Serializer request;
	RequestId requestId = random<RequestId>();
	request.write(NetworkOpcode::TRANSACTION_REQUEST);
	request.write(requestId);
	request.write<int>(transactionHashs.size());
	for (auto& hash : transactionHashs) {
		request.write<Hash>(hash);
	}
	requestContext.add(requestId, [callback, peer](NetworkOpcode opcode, Serializer& request) {
		if (opcode == NetworkOpcode::TRANSACTION_REPLY) {
			int count = request.read<int>();

			if (count >= 0) {
				std::vector<Transaction> transactions;
				for (int i = 0; i < count; i++) {
					std::string data;
					request.readStr(data);
					Transaction transaction;
					transaction.deserial(data);
					transaction.transactionHash = transaction.header.caclulateHash();
					transactions.push_back(transaction);
				}

				if (callback) {
					callback(transactions, peer);
				}
				return;
			}
		}
		std::vector<Transaction> transactions;
		if (callback) {
			callback(transactions, peer);
		}
	});
	network.send(peer, request.toString());
}

void Network::getAccount(Hash treeRoot, EccPublicKey address, const std::function<void(const Account&, PeerId)>& callback, PeerId peer) {
	if (peer == PeerId(0)) {
		peer = network.getRandomNeighbor();
	}
	Serializer request;
	RequestId requestId = random<RequestId>();
	request.write(NetworkOpcode::ACCOUNT_REQUEST);
	request.write(requestId);
	request.write(treeRoot);
	request.write(address);
	requestContext.add(requestId, [callback, peer](NetworkOpcode opcode, Serializer& request) {
		if (opcode == NetworkOpcode::ACCOUNT_REPLY) {
			Account account = request.read<Account>();
			if (callback) {
				callback(account, peer);
			}
			return;
		}
		Account account;
		if (callback) {
			callback(account, peer);
		}
	});
	network.send(peer, request.toString());
}
