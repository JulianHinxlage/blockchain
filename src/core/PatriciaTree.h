#pragma once

#include "type.h"
#include "network/Packet.h"
#include "cryptography/sha.h"
#include "PatriciaTreeNode.h"
#include <memory>

class MerkleProof {
public:
	std::vector<std::string> data;
};

//also known as patricia merkle trie
template<typename KeyType, typename ValueType>
class PatriciaTree {
public:
	static const int nibblePerKey = sizeof(KeyType) * 2;
	typedef PatriciaTreeNode<KeyType, ValueType> Node;

	PatriciaTree() {
		rootHash = Hash(0);
	}

	bool set(KeyType key, const ValueType& value) {
		if (rootHistory.size() == 0) {
			rootHistory.push_back(std::make_shared<Node>());
		}
		Node *root = rootHistory[rootHistory.size() - 1].get();

		int nibbleIndex = 0;
		Node* leaf = nullptr;
		std::shared_ptr<Node> newBranchRoot;
		if (root->insert(key, nibbleIndex, &leaf, &newBranchRoot)) {
			if (!leaf) {
				return false;
			}
			leaf->value = value;
			if (newBranchRoot) {
				if (newBranchRoot.get() != root) {
					rootHistory.push_back(newBranchRoot);
				}
				rootHash = newBranchRoot->getHash();
			}
			return true;
		}
		else {
			return false;
		}
	}

	bool get(const KeyType &key, ValueType &value) {
		if (rootHistory.size() == 0) {
			return false;
		}
		Node *root = rootHistory[rootHistory.size() - 1].get();
		
		int nibbleIndex = 0;
		Node *leaf = root->lookupLeaf(key, nibbleIndex);
		if (leaf && nibbleIndex >= nibblePerKey) {
			value = leaf->value;
			return true;
		}
		else {
			return false;
		}
	}

	Hash getRootHash() {
		return rootHash;
	}
	
	Hash recalculateRootHash() {
		if (rootHistory.size() == 0) {
			return Hash(0);
		}
		Node* root = rootHistory[rootHistory.size() - 1].get();
		rootHash = root->recalculateHash();
		return rootHash;
	}

	bool resetToState(Hash targetRoot) {
		for (int i = rootHistory.size() - 1; i >= 0; i--) {
			if (rootHistory[i]->getHash() == targetRoot) {
				rootHistory.resize(i + 1);
				if (rootHistory.size() == 0) {
					rootHash = Hash(0);
				}
				else {
					rootHash = rootHistory[rootHistory.size() - 1]->getHash();
				}
				return true;
			}
		}
		return false;
	}

	//all history up to the point given is deleted
	bool deleteHistory(Hash until) {
		for (int i = rootHistory.size() - 1; i >= 0; i--) {
			if (rootHistory[i]->getHash() == until) {
				rootHistory.erase(rootHistory.begin(), rootHistory.begin() + i);
				return true;
			}
		}
		return false;
	}

	void each(const std::function<void(const KeyType&, const ValueType&)>& callback) {
		if (rootHistory.size() == 0) {
			return;
		}
		Node* root = rootHistory[rootHistory.size() - 1].get();
		KeyType key = KeyType(0);
		int nibbleIndex = 0;
		root->each(key, 0, callback);
	}

	void generateMerkleProof(const KeyType &key, MerkleProof &proof) {
		if (rootHistory.size() == 0) {
			return;
		}
		Node* root = rootHistory[rootHistory.size() - 1].get();
		int nibbleIndex = 0;

		proof.data.clear();

		Node* next = root;
		while (next && nibbleIndex < nibblePerKey) {
			proof.data.push_back(next->serial());
			Node* prev = next;
			next = next->lookupChild(key, nibbleIndex);
			if (next == prev) {
				break;
			}
		}
	}
	
	static bool verifyMerkleProof(const Hash &rootHash, const KeyType& key, const MerkleProof& proof, ValueType &value) {
		int nibbleIndex = nibblePerKey - 1;
		Hash hash = Hash(0);
		for (int i = proof.data.size() - 1; i >= 0; i--) {
			Node node;
			node.deserial(proof.data[i]);
			
			if (node.type != PatriciaTreeNodeType::BRANCH &&
				node.type != PatriciaTreeNodeType::EXTENSION &&
				node.type != PatriciaTreeNodeType::LEAF) {
				return false;
			}

			if (node.type == PatriciaTreeNodeType::LEAF) {
				value = node.value;
			}
			else {
				if (i == proof.data.size() - 1) {
					return false;
				}
			}

			if (node.type == PatriciaTreeNodeType::BRANCH) {
				if (node.nodeHashes[getNibble(key, nibbleIndex)] != hash) {
					return false;
				}
				nibbleIndex--;
			}
			else {
				for (int i = node.path.length - 1; i >= 0; i--) {
					if (node.path.getNibble(i) != getNibble(key, nibbleIndex)) {
						return false;
					}
					nibbleIndex--;
				}
			}
			if (node.type == PatriciaTreeNodeType::EXTENSION) {
				if (node.nodeHashes[0] != hash) {
					return false;
				}
			}

			hash = sha256(proof.data[i]);
		}
		if (nibbleIndex != -1) {
			return false;
		}
		return hash == rootHash;
	}

	const std::vector<std::shared_ptr<Node>> &getHistory() {
		return rootHistory;
	}

private:
	Hash rootHash;
	std::vector<std::shared_ptr<Node>> rootHistory;
};
