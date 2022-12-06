#pragma once

#include "type.h"
#include "BinaryTreeNode.h"
#include "CachedKeyValueStore.h"
#include <unordered_map>

template<typename KeyType, typename ValueType, typename HashType = Hash>
class BinaryTree {
public:
	typedef BinaryTreeNode<KeyType, ValueType, HashType> Node;
	typedef BinaryTreeKey<KeyType> Key;
	typedef BinaryTreeNodeType Type;

	BinaryTree() {
		saveHistoryEnabled = true;
		rootHash = 0;
		historyIndexOffset = 0;
	}

	void initStore(const std::string& directory, int version = 1, uint64_t maxFileSize = uint64_t(4) * 1024 * 1024 * 1024 - 1) {
		store = std::make_shared<CachedKeyValueStore>();
		store->init(directory, version, maxFileSize);
	}

	HashType root() {
		return rootHash;
	}

	HashType recalculateRootHash() {
		ensureRoot();
		rootHash = rootNode->calculateHash(true);
		rootNode->save();
		return rootHash;
	}

	bool set(const KeyType& key, const ValueType &value) {
		ensureRoot();
		int bitOffset = 0;
		std::shared_ptr<Node> newRoot;
		Node* leaf = rootNode->insert(Key(key), bitOffset, newRoot);
		if (leaf) {
			leaf->value = value;
			rootNode = newRoot;
			rootHash = rootNode->calculateHash();
			rootNode->save();
			if (saveHistoryEnabled) {
				historyRoots.push_back(rootNode);
				historyRootHashes.push_back(rootHash);
				historyIndexMap[rootHash] = historyRoots.size() - 1;
			}
			return true;
		}
		else {
			return false;
		}
	}

	bool get(const KeyType& key, ValueType& value) {
		ensureRoot();
		int bitOffset = 0;
		Node *leaf = rootNode->lookupLeaf(Key(key), bitOffset);
		if (leaf) {
			value = leaf->value;
			return true;
		}
		else {
			return false;
		}
	}

	bool remove(const KeyType& key) {
		ensureRoot();
		int bitOffset = 0;
		std::shared_ptr<Node> newRoot;
		if(rootNode->erase(Key(key), bitOffset, newRoot)) {
			if (newRoot == nullptr) {
				newRoot = std::make_shared<Node>(store.get());
			}
			rootNode = newRoot;
			rootHash = rootNode->calculateHash();
			if (saveHistoryEnabled) {
				historyRoots.push_back(rootNode);
				historyRootHashes.push_back(rootHash);
				historyIndexMap[rootHash] = historyRoots.size() - 1;
			}
			return true;
		}
		else {
			return false;
		}
	}

	bool generateMerkleProof(const KeyType& key, Buffer &buffer) {
		ensureRoot();
		int bitOffset = 0;

		Node* next = rootNode.get();
		while (next) {
			next->serial(buffer);
			Node* prev = next;
			next = next->lookupChild(key, bitOffset);
			if (prev == next) {
				return true;
			}
		}

		return false;
	}

	static bool verifyMerkleProof(const KeyType& key, const HashType &rootHash, ValueType &value, Buffer& buffer) {
		std::vector<Node> nodes;
		while (buffer.size() > 0) {
			nodes.emplace_back();
			nodes.back().deserial(buffer);
		}
		int bitIndex = sizeof(KeyType) * 8 - 1;
		bool hasLeaf = false;

		Key k(key);

		Hash hash = Hash(0);
		for (int i = nodes.size() - 1; i >= 0; i--) {
			Node &node = nodes[i];

			if (node.type == Type::LEAF) {
				hasLeaf = true;
				value = node.value;

				int match = node.path.matchingBitCount(k, bitIndex - node.pathLength + 1);
				if (match >= node.pathLength) {
					bitIndex -= node.pathLength;
				}
				else {
					return false;
				}
			}
			else if (node.type == Type::EXTENSION) {
				if (node.child != hash) {
					return false;
				}
				int match = node.path.matchingBitCount(k, bitIndex - node.pathLength + 1);
				if (match >= node.pathLength) {
					bitIndex -= node.pathLength;
				}
				else {
					return false;
				}
			}
			else if (node.type == Type::BRANCH) {
				if (k.getBit(bitIndex)) {
					if (node.childs[1] != hash) {
						return false;
					}
				}
				else {
					if (node.childs[0] != hash) {
						return false;
					}
				}
				bitIndex--;
			}
			else {
				return false;
			}
			hash = node.calculateHash();
		}

		if (!hasLeaf) {
			return false;
		}
		if (bitIndex != -1) {
			return false;
		}
		return hash == rootHash;
	}

	void enableSaveHistory(bool enable) {
		saveHistoryEnabled = enable;
		if (!saveHistoryEnabled) {
			historyIndexOffset = 0;
			historyRoots.clear();
			historyRootHashes.clear();
			historyIndexMap.clear();
		}
		else {
			historyIndexOffset = 0;
			if (historyRoots.size() == 0 && rootNode != nullptr) {
				historyRoots.push_back(rootNode);
				historyRootHashes.push_back(rootHash);
				historyIndexMap[rootHash] = historyRoots.size() - 1;
			}
		}
	}

	void removeHistory(HashType oldestRootToKeep) {
		auto x = historyIndexMap.find(oldestRootToKeep);
		if (x == historyIndexMap.end()) {
			//given root not in history
			return;
		}

		int index = x->second;
		for (int i = 0; i < index - historyIndexOffset; i++) {
			historyIndexMap.erase(historyRootHashes[i]);
		}
		historyRootHashes.erase(historyRootHashes.begin(), historyRootHashes.begin() + index - historyIndexOffset);
		historyRoots.erase(historyRoots.begin(), historyRoots.begin() + index - historyIndexOffset);
		historyIndexOffset = index;
	}

	bool getHistoryState(HashType targetRoot, BinaryTree &tree) {
		auto x = historyIndexMap.find(targetRoot);
		if (x == historyIndexMap.end()) {
			//given root not in history

			if(targetRoot == Hash(0)) {
				tree.historyIndexOffset = 0;
				tree.historyRoots.clear();
				tree.historyRootHashes.clear();
				tree.historyIndexMap.clear();
				tree.store = store;
				tree.ensureRoot();
				return true;
			}

			if (store) {
				Buffer buffer = store->get(targetRoot).toBuffer();
				if (buffer.size() > 0) {
					tree.historyIndexOffset = 0;
					tree.historyRoots.clear();
					tree.historyRootHashes.clear();
					tree.historyIndexMap.clear();
					tree.store = store;

					tree.rootNode = std::make_shared<Node>(store.get());
					tree.rootNode->deserial(buffer);
					tree.rootHash = tree.rootNode->calculateHash();
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
		int index = x->second;

		tree.rootNode = historyRoots[index - historyIndexOffset];
		tree.rootHash = historyRootHashes[index - historyIndexOffset];

		tree.historyIndexOffset = 0;
		tree.historyRoots.clear();
		tree.historyRootHashes.clear();
		tree.historyIndexMap.clear();
		tree.store = store;

		return true;
	}

	void each(std::function<void(const KeyType&, const ValueType&)> callback, bool loadAll = false) {
		ensureRoot();
		Key key;
		int bitIndex = 0;
		rootNode->each(key, bitIndex, callback, loadAll);
	}

	void clear() {
		historyRoots.clear();
		historyRootHashes.clear();
		historyIndexMap.clear();
		historyIndexOffset = 0;
		rootNode = nullptr;
		rootHash = Hash(0);
	}

private:
	std::shared_ptr<Node> rootNode;
	HashType rootHash;
	std::vector<std::shared_ptr<Node>> historyRoots;
	std::vector<HashType> historyRootHashes;
	std::unordered_map<HashType, int> historyIndexMap;
	std::shared_ptr<CachedKeyValueStore> store;
	int historyIndexOffset;
	bool saveHistoryEnabled;

	void ensureRoot() {
		if (rootNode == nullptr) {
			rootNode = std::make_shared<Node>(store.get());
			rootHash = rootNode->calculateHash();
			if (saveHistoryEnabled) {
				historyRoots.push_back(rootNode);
				historyRootHashes.push_back(rootHash);
				historyIndexMap[rootHash] = historyRoots.size() - 1;
			}
		}
	}
};
