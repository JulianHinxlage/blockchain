#pragma once

#include "util/Buffer.h"
#include "cryptography/sha.h"
#include <stdint.h>
#include <memory>
#include <functional>
#include <cassert>

template<typename KeyType>
class BinaryTreeKey {
public:
	KeyType data;

	BinaryTreeKey(KeyType data = KeyType(0)) : data(data) {}

	bool getBit(int index) const {
		int byteIndex = index / 8;
		int bitIndex = index % 8;
		uint8_t byte = *((uint8_t*)&data + byteIndex);
		return byte & (1 << bitIndex);
	}

	void setBit(int index, bool value) {
		int byteIndex = index / 8;
		int bitIndex = index % 8;
		uint8_t* byte = (uint8_t*)&data + byteIndex;
		if (value) {
			*byte |= (1 << bitIndex);
		}
		else {
			*byte &= ~(1 << bitIndex);
		}
	}

	int matchingBitCount(const BinaryTreeKey&key, int offset) {
		int match = 0;
		int bitsLeft = sizeof(KeyType) * 8 - offset;
		for (int i = 0; i < bitsLeft; i++) {
			if (getBit(i) == key.getBit(i + offset)) {
				match++;
			}
			else {
				break;
			}
		}
		return match;
	}
};

enum class BinaryTreeNodeType : uint8_t {
	UNDEFINED,
	BRANCH,
	EXTENSION,
	LEAF,
};

template<typename KeyType, typename ValueType, typename HashType>
class BinaryTreeNode: public std::enable_shared_from_this<BinaryTreeNode<KeyType, ValueType, HashType>> {
public:
	typedef BinaryTreeNode<KeyType, ValueType, HashType> Node;
	typedef BinaryTreeKey<KeyType> Key;
	typedef BinaryTreeNodeType Type;

	Type type;
	uint16_t pathLength;
	union {
		struct { //BRANCH
			HashType childs[2];
		};
		struct { //EXTENSION
			Key path;
			HashType child;
		};
		struct { //LEAF
			Key path;
			ValueType value;
		};
	};
	std::shared_ptr<Node> nodes[2];

	BinaryTreeNode() {
		type = Type::UNDEFINED;
		pathLength = 0;
		path.data = 0;
		child = 0;
	}

	Node *lookupChild(const Key &key, int &bitOffset) {
		switch (type) {
		case Type::BRANCH:
			if (key.getBit(bitOffset)) {
				if (nodes[1]) {
					bitOffset++;
					return nodes[1].get();
				}
				else {
					return nullptr;
				}
			}
			else {
				if (nodes[0]) {
					bitOffset++;
					return nodes[0].get();
				}
				else {
					return nullptr;
				}
			}
			break;
		case Type::EXTENSION: {
			int match = path.matchingBitCount(key, bitOffset);
			match = std::min(match, (int)pathLength);
			if (match == pathLength && nodes[0]) {
				bitOffset += match;
				return nodes[0].get();
			}
			else {
				return nullptr;
			}
			break;
		}
		case Type::LEAF: {
			int match = path.matchingBitCount(key, bitOffset);
			match = std::min(match, (int)pathLength);
			if (match == pathLength) {
				bitOffset += match;
				return this;
			}
			else {
				return nullptr;
			}
			break;
		}
		default:
			return nullptr;
			break;
		}
	}

	Node *lookupLeaf(const Key& key, int &bitOffset) {
		Node* next = lookupChild(key, bitOffset);
		if (next) {
			if (next == this) {
				return this;
			}
			return next->lookupLeaf(key, bitOffset);
		}
		else {
			return nullptr;
		}
	}

	std::shared_ptr<Node> createNewLeaf(const Key& key, int bitOffset) {
		std::shared_ptr<Node> newLeaf = std::make_shared<Node>();
		newLeaf->type = Type::LEAF;
		newLeaf->pathLength = sizeof(Key) * 8 - bitOffset;
		for (int i = 0; i < newLeaf->pathLength; i++) {
			newLeaf->path.setBit(i, key.getBit(i + bitOffset));
		}
		return newLeaf;
	}

	std::shared_ptr<Node> createNewExtension(const Key& key, int bitOffset, int length) {
		std::shared_ptr<Node> newExtension = std::make_shared<Node>();
		newExtension->type = Type::EXTENSION;
		newExtension->pathLength = length;
		for (int i = 0; i < newExtension->pathLength; i++) {
			newExtension->path.setBit(i, key.getBit(i + bitOffset));
		}
		return newExtension;
	}

	Node *insert(const Key& key, int& bitOffset, std::shared_ptr<Node>& newRoot) {
		Node *next = lookupChild(key, bitOffset);
		if (next) {
			if (next == this) {
				//the requested key alrady exists, so copy the node for new root
				std::shared_ptr<Node> newLeaf = createNewLeaf(key, bitOffset);
				newRoot = newLeaf;
				return newLeaf.get();
			}
			else {
				Node *leaf = next->insert(key, bitOffset, newRoot);
				if(leaf){
					if (newRoot.get() != next && type != Type::LEAF) {
						//create a new root to create a new tree branch all the way up
						std::shared_ptr<Node> newNode = std::make_shared<Node>();
						*newNode = *this;
						if (type == Type::BRANCH) {
							for (int i = 0; i < 2; i++) {
								if (newNode->nodes[i].get() == next) {
									newNode->nodes[i] = newRoot;
									newNode->childs[i] = Hash(0);
								}
							}
						}
						else if (type == Type::EXTENSION) {
							if (newNode->nodes[0].get() == next) {
								newNode->nodes[0] = newRoot;
								newNode->child = Hash(0);
							}
						}
						newRoot = newNode;
					}
				}
				return leaf;
			}
		}
		else {
			switch (type) {
			case Type::BRANCH: {
				assert(false);
				break;
			}
			case Type::EXTENSION:
			case Type::LEAF: {
				int match = path.matchingBitCount(key, bitOffset);
				std::shared_ptr<Node> newBranch = std::make_shared<Node>();
				newBranch->type = Type::BRANCH;

				std::shared_ptr<Node> newLeaf = createNewLeaf(key, bitOffset + match + 1);

				if (match > 0) {
					std::shared_ptr<Node> newExtension = createNewExtension(key, bitOffset, match);
					newExtension->nodes[0] = newBranch;
					newRoot = newExtension;
				}
				else {
					newRoot = newBranch;
				}

				std::shared_ptr<Node> oldLeaf;
				if (type == Type::EXTENSION && pathLength <= match + 1) {
					oldLeaf = nodes[0];
				}
				else {
					oldLeaf = std::make_shared<Node>();
					*oldLeaf = *this;
					oldLeaf->path.data = oldLeaf->path.data >> (match + 1);
					oldLeaf->pathLength -= match + 1;
				}

				if (key.getBit(bitOffset + match)) {
					newBranch->nodes[0] = oldLeaf;
					newBranch->nodes[1] = newLeaf;
				}
				else {
					newBranch->nodes[0] = newLeaf;
					newBranch->nodes[1] = oldLeaf;
				}
				return newLeaf.get();
				break;
			}
			case Type::UNDEFINED: {
				std::shared_ptr<Node> newLeaf = createNewLeaf(key, bitOffset);
				newRoot = newLeaf;
				return newLeaf.get();
				break;
			}
			default:
				assert(false);
				return nullptr;
				break;
			}
		}
	}

	bool erase(const Key& key, int& bitOffset, std::shared_ptr<Node>& newRoot) {
		int startBitOffset = bitOffset;
		Node* next = lookupChild(key, bitOffset);
		if (next) {
			if (next == this) {
				newRoot = nullptr;
				return true;
			}

			if (next->erase(key, bitOffset, newRoot)) {
				if (newRoot != nullptr) {
					std::shared_ptr<Node> newNode = std::make_shared<Node>();
					*newNode = *this;
					if (type == Type::BRANCH) {
						for (int i = 0; i < 2; i++) {
							if (newNode->nodes[i].get() == next) {
								newNode->nodes[i] = newRoot;
								newNode->childs[i] = Hash(0);
							}
						}
					}
					else if (type == Type::EXTENSION) {
						if (newRoot->type == Type::BRANCH) {
							if (newNode->nodes[0].get() == next) {
								newNode->nodes[0] = newRoot;
								newNode->child = Hash(0);
							}
						}
						else {
							//remove extension and concat path to child node
							newRoot->path.data = newRoot->path.data << pathLength;
							newRoot->pathLength += pathLength;
							for (int i = 0; i < pathLength; i++) {
								newRoot->path.setBit(i, path.getBit(i));
							}
							return true;
						}
					}
					newRoot = newNode;
				}
				else {
					//child node was erased
					if (type == Type::BRANCH) {
						std::shared_ptr<Node> newNode = std::make_shared<Node>();

						for (int i = 0; i < 2; i++) {
							if (nodes[i].get() == next) {
								*newNode = *nodes[1 - i];
							}
						}

						if (newNode->type == Type::BRANCH) {
							//replace branch node with extension node
							std::shared_ptr<Node> newExtension = std::make_shared<Node>();
							newExtension->type = Type::EXTENSION;
							newExtension->pathLength = 1;
							newExtension->path.setBit(0, !key.getBit(startBitOffset));

							newExtension->nodes[0] = newNode;
							newNode = newExtension;
						}
						else {
							//remove branch and add the bit to  the path off the child node
							newNode->path.data = newNode->path.data << 1;
							newNode->pathLength++;
							newNode->path.setBit(0, !key.getBit(startBitOffset));
						}
						newRoot = newNode;
					}
					else if (type == Type::EXTENSION) {
						newRoot = nullptr;
					}
				}
				return true;
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
	}

	void serial(Buffer& buffer) {
		buffer.add((uint8_t)type);
		buffer.add((uint16_t)pathLength);

		if constexpr (sizeof(HashType) == sizeof(KeyType)) {
			if constexpr (sizeof(HashType) == sizeof(ValueType)) {
				buffer.add(childs[0]);
				buffer.add(childs[1]);
			}
			else {
				if (type == Type::LEAF) {
					buffer.add(path);
					buffer.add(value);
				}else{
					buffer.add(childs[0]);
					buffer.add(childs[1]);
				}
			}
		}
		else {
			switch (type) {
			case Type::BRANCH:
				buffer.add(childs[0]);
				buffer.add(childs[1]);
				break;
			case Type::EXTENSION:
				buffer.add(path);
				buffer.add(child);
				break; 
			case Type::LEAF:
				buffer.add(path);
				buffer.add(value);
				break; 
			default:
				break;
			}
		}
	}

	void deserial(Buffer& buffer) {
		type = (Type)buffer.get<uint8_t>();
		pathLength = buffer.get<uint16_t>();

		if constexpr (sizeof(HashType) == sizeof(KeyType)) {
			if constexpr (sizeof(HashType) == sizeof(ValueType)) {
				childs[0] = buffer.get<HashType>();
				childs[1] = buffer.get<HashType>();
			}
			else {
				if (type == Type::LEAF) {
					path = buffer.get<KeyType>();
					value = buffer.get<ValueType>();
				}
				else {
					childs[0] = buffer.get<HashType>();
					childs[1] = buffer.get<HashType>();
				}
			}
		}
		else {
			switch (type) {
			case Type::BRANCH:
				childs[0] = buffer.get<HashType>();
				childs[1] = buffer.get<HashType>();
				break;
			case Type::EXTENSION:
				path = buffer.get<KeyType>();
				child = buffer.get<HashType>();
				break;
			case Type::LEAF:
				path = buffer.get<KeyType>();
				value = buffer.get<ValueType>();
				break;
			default:
				break;
			}
		}
	}

	HashType calculateHash(bool recalculate = false) {
		if (type == Type::UNDEFINED) {
			return sha256("");
		}

		switch (type) {
		case Type::BRANCH:
			if (childs[0] == Hash(0) || recalculate) {
				childs[0] = nodes[0]->calculateHash(recalculate);
			}
			if (childs[1] == Hash(0) || recalculate) {
				childs[1] = nodes[1]->calculateHash(recalculate);
			}
			break;
		case Type::EXTENSION:
			if (child == Hash(0) || recalculate) {
				child = nodes[0]->calculateHash(recalculate);
			}
			break;
		default:
			break;
		}

		Buffer buffer;
		buffer.reserve(3 + std::max(sizeof(HashType) * 2, sizeof(ValueType) + sizeof(ValueType)));
		serial(buffer);
		return sha256(buffer.data(), buffer.size());
	}

	void each(Key &key, int &bitOffset, std::function<void(const KeyType&, const ValueType&)> callback) {
		for (int i = 0; i < pathLength; i++) {
			key.setBit(i + bitOffset, path.getBit(i));
		}
		bitOffset += pathLength;

		switch (type) {
		case Type::BRANCH:
			bitOffset++;
			key.setBit(bitOffset - 1, 0);
			nodes[0]->each(key, bitOffset, callback);
			key.setBit(bitOffset - 1, 1);
			nodes[1]->each(key, bitOffset, callback);
			bitOffset--;
			break;
		case Type::EXTENSION:
			nodes[0]->each(key, bitOffset, callback);
			break;
		case Type::LEAF:
			callback(key.data, value);
			break;
		default:
			break;
		}

		bitOffset -= pathLength;
	}
};