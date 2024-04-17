//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "type.h"
#include "util/Serializer.h"
#include "cryptography/sha.h"
#include <memory>

enum class BinaryTreeNodeType : uint8_t {
	NONE,
	BRANCH,
	EXTENSION,
	LEAF,
};

template<typename KeyType>
class BinaryTreeKey {
public:
	typedef BinaryTreeKey<KeyType> Key;
	KeyType key;

	BinaryTreeKey(const KeyType& key) {
		this->key = key;
	}

	BinaryTreeKey(){}

	bool getBit(int bitIndex) const {
		int byte = bitIndex / 8;
		int bit = bitIndex % 8;
		uint8_t v = ((uint8_t*)&key)[byte];
		return (v & (1 << bit)) != 0;
	}

	void setBit(int bitIndex, bool value) {
		int byte = bitIndex / 8;
		int bit = bitIndex % 8;
		uint8_t &v = ((uint8_t*)&key)[byte];

		if (value) {
			v |= (1 << bit);
		}
		else {
			v &= ~(1 << bit);
		}
	}

	int bitMatch(const Key& value, int bitOffset) {
		int left = (sizeof(KeyType) * 8) - bitOffset;
		for (int i = 0; i < left; i++) {
			if (getBit(i) != value.getBit(i + bitOffset)) {
				return i;
			}
		}
		return left;
	}
};

template<typename KeyType, typename ValueType, bool useSerial>
class BinaryTreeNode {
public:
	typedef BinaryTreeNode<KeyType, ValueType, useSerial> Node;
	typedef BinaryTreeKey<KeyType> Key;
	typedef BinaryTreeNodeType Type;

	BinaryTreeNodeType type;
	uint16_t pathLength;
	
	Hash childs[2];
	Key path;
	ValueType value;

	std::shared_ptr<Node> nodes[2];
	KeyValueStorage* storage;

	BinaryTreeNode() {
		type = BinaryTreeNodeType::NONE;
		pathLength = 0;
		path = Key();
		value = ValueType();
		childs[0] = 0;
		childs[1] = 0;
	}

	std::string serial() {
		Serializer serial;
		serial.write(type);
		serial.write(pathLength);
		if (type == BinaryTreeNodeType::BRANCH) {
			serial.write(childs[0]);
			serial.write(childs[1]);
		}
		else if (type == BinaryTreeNodeType::EXTENSION) {
			serial.write(path);
			serial.write(childs[0]);
		}
		else if (type == BinaryTreeNodeType::LEAF) {
			serial.write(path);
			if constexpr (useSerial) {
				std::string str = value.serial();
				serial.writeBytes((uint8_t*)str.data(), str.size());
			}
			else {
				serial.write(value);
			}
		}
		return serial.toString();
	}

	int deserial(const std::string& str) {
		Serializer serial(str);
		serial.read(type);
		serial.read(pathLength);
		if (type == BinaryTreeNodeType::BRANCH) {
			serial.read(childs[0]);
			serial.read(childs[1]);
		}
		else if (type == BinaryTreeNodeType::EXTENSION) {
			serial.read(path);
			serial.read(childs[0]);
		}
		else if (type == BinaryTreeNodeType::LEAF) {
			serial.read(path);
			if constexpr (useSerial) {
				value.deserial(serial.readAll());
			}
			else {
				serial.read(value);
			}
		}
		return serial.getReadIndex();
	}

	Hash calculateHash() {
		if (type == Type::NONE) {
			return Hash(0);
		}
		else if (type == BinaryTreeNodeType::BRANCH) {
			if (childs[0] == Hash(0)) {
				childs[0] = nodes[0]->calculateHash();
			}
			if (childs[1] == Hash(0)) {
				childs[1] = nodes[1]->calculateHash();
			}
		}
		else if (type == BinaryTreeNodeType::EXTENSION) {
			if (childs[0] == Hash(0)) {
				childs[0] = nodes[0]->calculateHash();
			}
		}
		Hash hash = sha256(serial());
		if (!storage->has(hash)) {
			storage->set(hash, serial());
		}
		return hash;
	}

	void load(const Hash& hash) {
		if (storage->has(hash)) {
			deserial(storage->get(hash));
		}
	}

	Node* getChild(const Key& key, int &bitOffset) {
		if (type == Type::BRANCH) {
			bool bit = key.getBit(bitOffset);
			if (!nodes[bit]) {
				if (childs[bit] == Hash(0)) {
					return nullptr;
				}
				else {
					auto node = std::make_shared<Node>();
					node->storage = storage;
					node->load(childs[bit]);
					nodes[bit] = node;
				}
			}
			bitOffset++;
			return nodes[bit].get();
		}
		else {
			int match = path.bitMatch(key, bitOffset);
			if (match >= pathLength) {
				match = pathLength;
				if (type == Type::LEAF) {
					bitOffset += match;
					return this;
				}
				else if (type == Type::EXTENSION) {
					if (!nodes[0]) {
						if (childs[0] == Hash(0)) {
							return nullptr;
						}
						else {
							auto node = std::make_shared<Node>();
							node->storage = storage;
							node->load(childs[0]);
							nodes[0] = node;
						}
					}
					bitOffset += match;
					return nodes[0].get();
				}
				else {
					return nullptr;
				}
			}
			else {
				return nullptr;
			}
		}
	}
	
	Node* getLeaf(const Key &key, int bitOffset = 0) {
		Node* child = getChild(key, bitOffset);
		if (child) {
			if (child == this) {
				return child;
			}
			return child->getLeaf(key, bitOffset);
		}
		else {
			return nullptr;
		}
	}

	std::shared_ptr<Node> createLeaf(const Key& key, int bitOffset) {
		std::shared_ptr<Node> node = std::make_shared<Node>();
		node->storage = storage;
		node->type = Type::LEAF;
		node->pathLength = (sizeof(key) * 8) - bitOffset;
		for (int i = 0; i < node->pathLength; i++) {
			node->path.setBit(i, key.getBit(i + bitOffset));
		}
		return node;
	}
	
	std::shared_ptr<Node> createExtension(const Key& key, int bitOffset, int pathLength) {
		std::shared_ptr<Node> node = std::make_shared<Node>();
		node->storage = storage;
		node->type = Type::EXTENSION;
		node->pathLength = pathLength;
		for (int i = 0; i < node->pathLength; i++) {
			node->path.setBit(i, key.getBit(i + bitOffset));
		}
		return node;
	}

	std::shared_ptr<Node> createBranch() {
		std::shared_ptr<Node> node = std::make_shared<Node>();
		node->storage = storage;
		node->type = Type::BRANCH;
		node->pathLength = 0;
		return node;
	}

	Node* insert(const Key& key, int bitOffset, std::shared_ptr<Node> &newRoot) {
		Node* next = getChild(key, bitOffset);
		if (next) {
			if (next == this) {
				std::shared_ptr<Node> node = std::make_shared<Node>();
				node->storage = storage;
				*node = *this;
				newRoot = node;
				return node.get();
			}
			else {
				Node *leaf = next->insert(key, bitOffset, newRoot);
				if (leaf) {
					if (type == Type::BRANCH) {
						std::shared_ptr<Node> node = createBranch();
						if (nodes[0].get() == next) {
							node->nodes[0] = newRoot;
							node->nodes[1] = nodes[1];
							node->childs[1] = childs[1];
						}
						else if (nodes[1].get() == next) {
							node->nodes[1] = newRoot;
							node->nodes[0] = nodes[0];
							node->childs[0] = childs[0];
						}
						newRoot = node;
					}
					else if(type == Type::EXTENSION){
						std::shared_ptr<Node> node = std::make_shared<Node>();
						node->storage = storage;
						*node = *this;
						node->childs[0] = Hash(0);
						node->nodes[0] = newRoot;
						newRoot = node;
					}
				}
				return leaf;
			}
		}
		else {
			if (type == Type::BRANCH) {
				return nullptr;
			}
			else if(type == Type::NONE) {
				newRoot = createLeaf(key, bitOffset);
				return newRoot.get();
			}
			else if (type == Type::EXTENSION || type == Type::LEAF) {
				int match = path.bitMatch(key, bitOffset);

				std::shared_ptr<Node> newBranch = createBranch();

				if (match > 0) {
					std::shared_ptr<Node> newExtension = createExtension(key, bitOffset, match);
					newExtension->nodes[0] = newBranch;
					newRoot = newExtension;
				}
				else {
					newRoot = newBranch;
				}

				std::shared_ptr<Node> oldLeaf = std::make_shared<Node>();
				oldLeaf->storage = storage;
				if (type == Type::LEAF) {
					oldLeaf->type = Type::LEAF;
					oldLeaf->pathLength = pathLength - match - 1;
					for (int i = 0; i < oldLeaf->pathLength; i++) {
						oldLeaf->path.setBit(i, path.getBit(i + match + 1));
					}
					oldLeaf->value = value;
				}
				else {
					if (pathLength <= match + 1) {
						oldLeaf = nodes[0];
					}
					else {
						oldLeaf->type = Type::EXTENSION;
						oldLeaf->pathLength = pathLength - match - 1;
						for (int i = 0; i < oldLeaf->pathLength; i++) {
							oldLeaf->path.setBit(i, path.getBit(i + match + 1));
						}
						oldLeaf->childs[0] = childs[0];
						oldLeaf->nodes[0] = nodes[0];
					}
				}

				std::shared_ptr<Node> newLeaf = createLeaf(key, bitOffset + match + 1);
				if (key.getBit(bitOffset + match)) {
					newBranch->nodes[1] = newLeaf;
					newBranch->nodes[0] = oldLeaf;
				}
				else {
					newBranch->nodes[0] = newLeaf;
					newBranch->nodes[1] = oldLeaf;
				}

				return newLeaf.get();
			}
		}
	}
};
