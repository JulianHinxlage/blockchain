#pragma once

#include "type.h"
#include "network/Packet.h"
#include "cryptography/sha.h"
#include <memory>
#include <functional>

enum class PatriciaTreeNodeType : uint8_t {
	BRANCH,
	EXTENSION,
	LEAF,
	UNDEFINED = -1,
};

class NibblePath {
public:
	std::vector<uint8_t> data;
	uint8_t length;

	NibblePath() {
		length = 0;
	}

	uint8_t getNibble(int nibbleIndex) {
		int byteIndex = nibbleIndex / 2;
		if (byteIndex < 0 || byteIndex >= data.size()) {
			return 0;
		}
		uint8_t byte = data[byteIndex];
		if (nibbleIndex % 2 == 0) {
			return byte >> 0 & 0xf;
		}
		else {
			return byte >> 4 & 0xf;
		}
	}

	//removes front most nibbles
	void popNibble(int count = 1) {
		int byteShift = count / 2;
		for (int i = 0; i < byteShift; i++) {
			data.erase(data.begin());
		}

		if (count % 2 == 1) {
			int i = 0;
			for (i = 0; i < data.size() - 1; i++) {
				data[i] = (data[i] >> 4 & 0x0f) | (data[i + 1] << 4 & 0xf0);
			}
			data[i] = (data[i] >> 4 & 0x0f);
		}
		length -= count;
		data.resize((length - 1) / 2 + 1);
	}

	//adds a nibble to the end
	void pushNibble(uint8_t nibble) {
		int byteIndex = length / 2;
		if (data.size() <= byteIndex) {
			data.resize(byteIndex + 1, 0);
		}

		if (length % 2 == 0) {
			data[byteIndex] |= nibble & 0xf;
		}
		else {
			data[byteIndex] |= (nibble << 4) & 0xf0;
		}
		length++;
	}
};

template<typename T>
static uint8_t getNibble(const T& key, int index) {
	int byteIndex = index / 2;
	uint8_t byte = *(((uint8_t*)&key) + byteIndex);
	if (index % 2 == 0) {
		return byte >> 4 & 0xf;
	}
	else {
		return byte >> 0 & 0xf;
	}
}

template<typename T>
static void setNibble(T& key, int index, uint8_t nibble) {
	int byteIndex = index / 2;
	uint8_t *byte = ((uint8_t*)&key) + byteIndex;
	if (index % 2 == 0) {
		*byte = (nibble << 4 & 0xf0) | *byte & 0x0f;
	}
	else {
		*byte = (nibble << 0 & 0x0f) | *byte & 0xf0;
	}
}

template<typename KeyType, typename ValueType>
class PatriciaTreeNode : public std::enable_shared_from_this<PatriciaTreeNode<KeyType, ValueType>> {
public:
	typedef PatriciaTreeNode<KeyType, ValueType> Node;
	static const int nibblePerKey = sizeof(KeyType) * 2;

	PatriciaTreeNodeType type;
	NibblePath path;
	ValueType value;
	bool dirty;
	Hash hash;
	std::vector<std::shared_ptr<Node>> nodes;

	//is only data from deserialisation
	std::vector<Hash> nodeHashes;

	PatriciaTreeNode(PatriciaTreeNodeType type = PatriciaTreeNodeType::UNDEFINED) {
		this->type = type;
		dirty = true;
		hash = Hash(0);
		value = ValueType(0);

		switch (type) {
		case PatriciaTreeNodeType::BRANCH:
			nodes.resize(16);
			break;
		case PatriciaTreeNodeType::EXTENSION:
			nodes.resize(1);
			break;
		case PatriciaTreeNodeType::LEAF:
			nodes.resize(0);
			break;
		default:
			break;
		}
	}

	Hash getHash() {
		if (dirty) {
			hash = sha256(serial());
			dirty = false;
		}
		return hash;
	}

	Hash recalculateHash() {
		for (auto& n : nodes) {
			if (n) {
				n->recalculateHash();
			}
		}
		hash = sha256(serial());
		dirty = false;
		return hash;
	}

	std::string serial() {
		Packet packet;
		packet.add((uint8_t)type);

		switch (type) {
		case PatriciaTreeNodeType::BRANCH: {
			uint16_t bitField = 0;
			for (int i = 0; i < 16; i++) {
				if (nodes[i]) {
					bitField |= (1 << i);
				}
			}
			packet.add(bitField);
			for (int i = 0; i < 16; i++) {
				if (nodes[i]) {
					packet.add(nodes[i]->getHash());
				}
			}
			break;
		}
		case PatriciaTreeNodeType::EXTENSION:
			packet.add((uint8_t)path.length);
			packet.add((char*)path.data.data(), path.data.size());
			if (nodes[0]) {
				packet.add(nodes[0]->getHash());
			}
			else {
				packet.add(Hash(0));
			}
			break;
		case PatriciaTreeNodeType::LEAF:
			packet.add((uint8_t)path.length);
			packet.add((char*)path.data.data(), path.data.size());
			packet.add(value);
			break;
		default:
			break;
		}

		return packet.remaining();
	}

	void deserial(const std::string &str) {
		Packet packet;
		packet.add(str.data(), str.size());
		type = (PatriciaTreeNodeType)packet.get<uint8_t>();
		dirty = true;

		switch (type) {
		case PatriciaTreeNodeType::BRANCH: {
			uint16_t bitField = packet.get<uint16_t>();
			nodeHashes.resize(16);
			for (int i = 0; i < 16; i++) {
				if (bitField & (1 << i)) {
					Hash hash = packet.get<Hash>();
					nodeHashes[i] = hash;
				}
				else {
					nodeHashes[i] = Hash(0);
				}
			}
			break;
		}
		case PatriciaTreeNodeType::EXTENSION:
			path.length = packet.get<uint8_t>();
			path.data.resize((path.length - 1) / 2 + 1);
			packet.get((char*)path.data.data(), path.data.size());
			nodeHashes.resize(1);
			nodeHashes[0] = packet.get<Hash>();
			break;
		case PatriciaTreeNodeType::LEAF:
			path.length = packet.get<uint8_t>();
			path.data.resize((path.length - 1) / 2 + 1);
			packet.get((char*)path.data.data(), path.data.size());
			nodeHashes.resize(0);
			value = packet.get<ValueType>();
			break;
		default:
			break;
		}
	}

	Node* lookupChild(const KeyType& key, int& nibbleIndex) {
		switch (type) {
		case PatriciaTreeNodeType::BRANCH: {
			uint8_t nibble = getNibble(key, nibbleIndex);
			nibbleIndex++;
			return nodes[nibble].get();
		}
		case PatriciaTreeNodeType::EXTENSION:
			for (int i = 0; i < path.length; i++) {
				uint8_t nibble = getNibble(key, nibbleIndex);
				if (nibble != path.getNibble(i)) {
					return nullptr;
				}
				nibbleIndex++;
			}
			return nodes[0].get();
		case PatriciaTreeNodeType::LEAF:
			for (int i = 0; i < path.length; i++) {
				uint8_t nibble = getNibble(key, nibbleIndex);
				if (nibble != path.getNibble(i)) {
					return nullptr;
				}
				nibbleIndex++;
			}
			return this;
		default:
			return nullptr;
		}
	}

	Node* lookupLeaf(const KeyType& key, int& nibbleIndex) {
		Node* next = this;
		while (next && nibbleIndex < nibblePerKey) {
			Node* prev = next;
			next = next->lookupChild(key, nibbleIndex);
			if (next == prev) {
				break;
			}
		}
		return next;
	}

	//inserts a new leaf and creates a new tree branch
	bool insert(const KeyType& key, int& nibbleIndex, Node** leaf, std::shared_ptr<Node>* newBranchRoot) {
		int nibbleStart = nibbleIndex;
		Node* next = lookupChild(key, nibbleIndex);
		if (next) {
			if (next == this) {
				if (nibbleIndex >= nibblePerKey) {
					//leaf node alrady exists, but we need to create a new tree branch anyway
					std::shared_ptr<Node> newNode = std::make_shared<Node>();
					*newNode = *this;
					newNode->dirty = true;
					*newBranchRoot = newNode;
					*leaf = newNode.get();
					return true;
				}
				else {
					return false;
				}
			}
			else {
				if (next->insert(key, nibbleIndex, leaf, newBranchRoot)) {
					if (newBranchRoot->get() != next) {
						if (type != PatriciaTreeNodeType::LEAF) {
							//create next node for new tree branch
							std::shared_ptr<Node> newNode = std::make_shared<Node>();
							*newNode = *this;
							newNode->dirty = true;

							for (int i = 0; i < newNode->nodes.size(); i++) {
								if (newNode->nodes[i].get() == next) {
									newNode->nodes[i] = *newBranchRoot;
								}
							}

							*newBranchRoot = newNode;
						}
					}
					return true;
				}
				else {
					return false;
				}
			}
		}
		else {
			nibbleIndex = nibbleStart;
			return branch(key, nibbleIndex, leaf, newBranchRoot);
		}
	}

	void each(KeyType key, int nibbleIndex, const std::function<void(const KeyType &, const ValueType &)> &callback) {
		switch (type) {
		case PatriciaTreeNodeType::BRANCH: {
			for (int i = 0; i < nodes.size(); i++) {
				if (nodes[i]) {
					setNibble(key, nibbleIndex, i);
					nodes[i]->each(key, nibbleIndex + 1, callback);
				}
			}
			break;
		}
		case PatriciaTreeNodeType::EXTENSION:
			for (int i = 0; i < path.length; i++) {
				setNibble(key, nibbleIndex + i, path.getNibble(i));
			}
			nodes[0]->each(key, nibbleIndex + path.length, callback);
			break;
		case PatriciaTreeNodeType::LEAF:
			for (int i = 0; i < path.length; i++) {
				setNibble(key, nibbleIndex + i, path.getNibble(i));
			}
			callback(key, value);
			break;
		default:
			break;
		}
	}

private:
	bool branch(const KeyType& key, int& nibbleIndex, Node** leaf, std::shared_ptr<Node>* newBranchRoot) {
		switch (type) {
		case PatriciaTreeNodeType::BRANCH: {
			uint8_t nibble = getNibble(key, nibbleIndex);
			std::shared_ptr<Node> newNode = std::make_shared<Node>();
			*newNode = *this;
			newNode->nodes[nibble] = std::make_shared<Node>();
			newNode->dirty = true;
			nibbleIndex++;

			if (!newNode->nodes[nibble]->branch(key, nibbleIndex, leaf, newBranchRoot)) {
				return false;
			}
			*newBranchRoot = newNode;
			return true;
			break;
		}
		case PatriciaTreeNodeType::EXTENSION:
			//handle the same as leaf
		case PatriciaTreeNodeType::LEAF: {
			uint8_t nibble = getNibble(key, nibbleIndex);
			uint8_t leafNibble = path.getNibble(0);

			if (nibble != leafNibble) {
				std::shared_ptr<Node> newNode = std::make_shared<Node>(PatriciaTreeNodeType::BRANCH);
				std::shared_ptr<Node> newLeaf = std::make_shared<Node>();
				*newLeaf = *this;
				newLeaf->dirty = true;
				newLeaf->path.popNibble(1);

				if (newLeaf->type == PatriciaTreeNodeType::EXTENSION) {
					if (newLeaf->path.length <= 0) {
						//remove extension node, if not longer needed
						newLeaf = newLeaf->nodes[0];
					}
				}

				newNode->nodes[leafNibble] = newLeaf;
				newNode->nodes[nibble] = std::make_shared<Node>();
				nibbleIndex++;

				if (!newNode->nodes[nibble]->branch(key, nibbleIndex, leaf, newBranchRoot)) {
					return false;
				}
				*newBranchRoot = newNode;
				return true;
			}
			else {

				std::shared_ptr<Node> extNode = std::make_shared<Node>(PatriciaTreeNodeType::EXTENSION);

				int matchCount = 0;
				while (nibble == leafNibble) {
					extNode->path.pushNibble(nibble);
					matchCount++;
					nibble = getNibble(key, nibbleIndex + matchCount);
					leafNibble = path.getNibble(matchCount);
				}
				matchCount++;

				std::shared_ptr<Node> newNode = std::make_shared<Node>(PatriciaTreeNodeType::BRANCH);
				std::shared_ptr<Node> newLeaf = std::make_shared<Node>();
				*newLeaf = *this;
				newLeaf->dirty = true;
				newLeaf->path.popNibble(matchCount);

				if (newLeaf->type == PatriciaTreeNodeType::EXTENSION) {
					if (newLeaf->path.length <= 0) {
						//remove extension node, if not longer needed
						newLeaf = newLeaf->nodes[0];
					}
				}

				newNode->nodes[leafNibble] = newLeaf;
				newNode->nodes[nibble] = std::make_shared<Node>();
				nibbleIndex += matchCount;

				extNode->nodes[0] = newNode;
				if (!newNode->nodes[nibble]->branch(key, nibbleIndex, leaf, newBranchRoot)) {
					return false;
				}
				*newBranchRoot = extNode;
				return true;
			}
			break;
		}
		case PatriciaTreeNodeType::UNDEFINED: {
			*this = Node(PatriciaTreeNodeType::LEAF);
			for (int i = nibbleIndex; i < nibblePerKey; i++) {
				uint8_t nibble = getNibble(key, i);
				path.pushNibble(nibble);
				nibbleIndex++;
			}
			*leaf = this;
			*newBranchRoot = this->shared_from_this();
			return true;
			break;
		}
		default:
			break;
		}

		return false;
	}
};
