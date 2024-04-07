//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "BlockChainNode.h"
#include "util/Serializer.h"
#include "BlockChain.h"
#include "util/random.h"
#include <fstream>

void BlockChainNode::init(NodeType type, const std::string& entryNodeFile) {
	state = NodeState::DISCONNECTED;
	this->type = type;

	std::ifstream stream(entryNodeFile);
	if (stream.is_open()) {
		//todo
	}
}

void BlockChainNode::connect(const std::string& address, uint16_t port) {
	for (int i = 0; i < 10; i++) {
		network.addEntryNode(address, port + i);
	}

	network.onStateChanged = [&](PeerNetworkState state) {
		if (state == PeerNetworkState::CONNECTED) {
			changeState(NodeState::CONNECTED);
		}
		else if(state == PeerNetworkState::DISCONNECTED) {
			changeState(NodeState::DISCONNECTED);
		}
	};

	network.logCallback = [&](const std::string& msg, bool error) {
		if (logCalback) {
			logCalback(msg, error);
		}
	};

	network.messageCallback = [&](const std::string& msg, PeerId source, bool broadcast) {
		onMessage(msg, source);
	};

	changeState(NodeState::JOINING);
	network.connect(address, port);
}

void BlockChainNode::disconnect() {
	network.disconnect();
}

void BlockChainNode::sendBlock(const Block& block) {
	Serializer request;
	request.write(NodeOpcode::BLOCK_BROADCAST);
	request.write(random<RequestId>());
	request.writeStr(block.serial());
	network.broadcast(request.toString());
}

void BlockChainNode::sendTransaction(const Transaction& transaction) {
	Serializer request;
	request.write(NodeOpcode::TRANSACTION_BREADCAST);
	request.write(random<RequestId>());
	request.writeStr(transaction.serial());
	network.broadcast(request.toString());
}

void BlockChainNode::synchronize() {
	if (state != NodeState::CONNECTED) {
		if (state != NodeState::SYNCHRONIZING) {
			isSynchronizationPending = true;
		}
		return;
	}
	changeState(NodeState::SYNCHRONIZING);

	PeerId peer = network.getRandomNeighbor();
	blockHashRequest(peer, -1, -1, [&, peer](int blockNumberBegin, const std::vector<Hash>& hashes) {
		if (blockNumberBegin > blockChain->getBlockCount()) {
			blockHashRequest(peer, blockChain->getBlockCount(), blockNumberBegin + 1, [&, peer](int blockNumberBegin, const std::vector<Hash>& hashes) {
				blockRequest(peer, hashes);
			});
		}
		else if (blockChain->getBlockCount() == blockNumberBegin) {
			blockRequest(peer, hashes);
		}
	});
}

void BlockChainNode::changeState(NodeState newState) {
	if (state != newState) {
		state = newState;
		if (isSynchronizationPending) {
			if (state == NodeState::CONNECTED) {
				isSynchronizationPending = false;
				synchronize();
			}
		}
	}
}

void BlockChainNode::blockHashRequest(PeerId peer, int blockNumberBegin, int blockNumberEnd, const std::function<void(int, const std::vector<Hash>&)>& callback) {
	Serializer request;
	RequestId requestId = random<RequestId>();
	request.write(NodeOpcode::BLOCK_HASH_REQUEST);
	request.write(requestId);
	request.write<int>(blockNumberBegin);
	request.write<int>(blockNumberEnd);
	requestContext.add(requestId, [callback](NodeOpcode opcode, Serializer& request) {
		if (opcode == NodeOpcode::BLOCK_HASH_REPLY) {
			int blockNumberBegin = request.read<int>();
			int blockNumberEnd = request.read<int>();

			if (blockNumberBegin >= 0 && blockNumberEnd >= 0) {
				std::vector<Hash> hashes;
				for (int i = blockNumberBegin; i < blockNumberEnd; i++) {
					hashes.push_back(request.read<Hash>());
				}

				if (callback) {
					callback(blockNumberBegin, hashes);
				}
			}
		}
	});
	network.send(peer, request.toString());
}

void BlockChainNode::blockRequest(PeerId peer, const std::vector<Hash>& hashes) {
	Serializer request;
	RequestId requestId = random<RequestId>();
	request.write(NodeOpcode::BLOCK_REQUEST);
	request.write(requestId);
	request.write<int>(hashes.size());
	for (auto& hash : hashes) {
		request.write(hash);
	}
	requestContext.add(requestId, [&](NodeOpcode opcode, Serializer& request) {
		if (opcode == NodeOpcode::BLOCK_REPLY) {
			int count = request.read<int>();
			for (int i = 0; i < count; i++) {
				std::string data;
				request.readStr(data);
				Block block;
				block.deserial(data);
				block.blockHash = block.header.caclulateHash();
				if (onBlockRecived) {
					onBlockRecived(block);
				}
			}
		}
	});
	network.send(peer, request.toString());
}

void BlockChainNode::onMessage(const std::string& msg, PeerId source) {
	Serializer request(msg);

	NodeOpcode opcode = request.read<NodeOpcode>();
	RequestId requestId = request.read<RequestId>();

	if (!requestContext.onRequest(requestId, opcode, request)) {
		if (opcode == NodeOpcode::BLOCK_HASH_REQUEST) {
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
					reply.write(NodeOpcode::BLOCK_HASH_REPLY);
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
		else if (opcode == NodeOpcode::BLOCK_REQUEST) {
			int count = request.read<int>();
			if (count > 0 && count < 100) {

				Serializer reply;
				reply.write(NodeOpcode::BLOCK_REPLY);
				reply.write(requestId);
				reply.write(count);

				for (int i = 0; i < count; i++) {
					Hash hash = request.read<Hash>();
					Block block = blockChain->getBlock(hash);
					std::string data = block.serial();
					reply.writeStr(data);
				}
				network.send(source, reply.toString());
				return;
			}
		}
		else if (opcode == NodeOpcode::BLOCK_BROADCAST) {
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
		else if (opcode == NodeOpcode::TRANSACTION_BREADCAST) {
			std::string data;
			request.readStr(data);
			Transaction transaction;
			transaction.deserial(data);
			if (onTransactionRecived) {
				onTransactionRecived(transaction);
			}
			return;
		}
		else if (opcode == NodeOpcode::REQUEST_ERROR) {
			if (logCalback) {
				logCalback("got error reply", true);
			}
			return;
		}
		else {
			if (logCalback) {
				logCalback("got invalid message: opcode=" + std::to_string((int)opcode), true);
			}
		}

		Serializer reply;
		reply.write(NodeOpcode::REQUEST_ERROR);
		reply.write(requestId);
		network.send(source, reply.toString());
	}
}
