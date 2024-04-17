//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "storage/KeyValueStorage.h"
#include "BinaryTreeNode.h"

template<typename KeyType, typename ValueType, bool useSerial>
class BinaryTree {
public:
	KeyValueStorage* storage;

	typedef BinaryTreeNode<KeyType, ValueType, useSerial> Node;
	typedef BinaryTreeNodeType Type;

	void init(const std::string& directory, const Hash &root = Hash()) {
		reset(root);
	}

	Hash getRoot() {
		if (rootHash == Hash(0)) {
			rootHash = rootNode->calculateHash();
		}
		return rootHash;
	}

	void set(const KeyType &key, const ValueType &value) {
		std::shared_ptr<Node> newRoot;
		Node* node = rootNode->insert(key, 0, newRoot);
		if (node && node->type == Type::LEAF) {
			node->value = value;
			rootNode = newRoot;
			rootHash = Hash(0);
		}
	}

	bool has(const KeyType &key) {
		Node* node = rootNode->getLeaf(key);
		if (node && node->type == Type::LEAF) {
			return true;
		}
		return false;
	}

	ValueType get(const KeyType &key) {
		Node *node = rootNode->getLeaf(key);
		if (node && node->type == Type::LEAF) {
			return node->value;
		}
		return ValueType();
	}

	bool reset(const Hash& root = Hash()) {
		rootNode = std::make_shared<Node>();
		rootNode->storage = storage;
		rootHash = Hash();
		if (root != Hash()) {
			if (storage->has(root)) {
				rootNode->load(root);
				rootHash = root;
				return true;
			}
		}
		return false;
	}

private:
	std::shared_ptr<Node> rootNode;
	Hash rootHash;
};
