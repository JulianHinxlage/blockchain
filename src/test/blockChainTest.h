//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "blockchain/BlockChain.h"
#include "blockchain/BlockCreator.h"
#include "blockchain/BlockValidator.h"
#include "util/hex.h"
#include <filesystem>

class KeyStore {
public:
	EccPrivateKey priv;
	EccPublicKey pub;

	void createKey(const std::string& file) {
		if (std::filesystem::exists(file)) {
			return;
		}

		eccGenerate(priv, pub);

		std::string content;
		content += toHex(pub) + "\n";
		content += toHex(priv) + "\n";

		std::ofstream stream(file);
		stream.write(content.data(), content.size());
	}

	void loadKey(const std::string& file) {
		if (!std::filesystem::exists(file)) {
			return;
		}

		std::ifstream stream(file);
		std::string line;
		std::getline(stream, line);
		pub = fromHex<EccPublicKey>(line);
		std::getline(stream, line);
		priv = fromHex<EccPrivateKey>(line);
	}
};

static void blockChainTest() {
	BlockChain chain;
	chain.init("chain");
	int transactionCount = 0;
	for (auto& h : chain.blockList) {
		transactionCount += chain.getBlock(h).header.transactionCount;
	}
	printf("blockchain loaded with %i blocks and %i transactions\n", (int)chain.blockList.size(), transactionCount);
	printf("tip %s\n", toHex(chain.latestBlock).c_str());

	BlockValidator validator;
	validator.blockChain = &chain;

	bool verifyChain = true;
	bool cutChain = true;
	if (verifyChain) {
		printf("start verifying blockchain (%i blocks)\n", (int)chain.blockList.size());
		bool allValid = true;
		int index = 0;
		int minInvalidIndex = -1;
		for (auto& h : chain.blockList) {
			Block block = chain.getBlock(h);
			BlockError error = validator.validateBlock(block);
			if (error != BlockError::VALID) {
				printf("Block Error: %s, num=%i, hash=%s\n", blockErrorToString(error), (int)block.header.blockNumber, toHex(block.blockHash).c_str());
				allValid = false;
				if (minInvalidIndex == -1) {
					minInvalidIndex = index;
				}
			}
			index++;
		}
		if (!allValid) {
			printf("invalid blocks in blockchain found\n");

			if (cutChain && minInvalidIndex != -1) {
				if (minInvalidIndex > 0) {
					chain.resetTip(chain.blockList[minInvalidIndex - 1]);
				}
				else {
					chain.resetTip(Hash(0));
				}
			}
		}
	}

	KeyStore key1;
	key1.loadKey("key1.txt");

	KeyStore Key2;
	Key2.loadKey("key2.txt");

	BlockCreator creator;
	creator.blockChain = &chain;
	creator.beginBlock(Key2.pub);

	for (int i = 0; i < 1; i++) {
		Transaction tx = creator.createTransaction(key1.pub, Key2.pub, coinToAmount("1.0"));
		tx.header.sign(key1.priv);
		tx.transactionHash = tx.header.caclulateHash();
		creator.addTransaction(tx);
		printf("created transaction %s\n", toHex(tx.transactionHash).c_str());
	}

	Block block = creator.endBlock();
	block.header.sign(Key2.priv);
	block.blockHash = block.header.caclulateHash();
	printf("created block %s\n", toHex(block.blockHash).c_str());


	BlockError error = validator.validateBlock(block);
	if (error != BlockError::VALID) {
		printf("Block Error: %s\n", blockErrorToString(error));
	}
	else {
		chain.addBlock(block);
		chain.addBlockToTip(block.blockHash);
	}
	chain.accountTree.reset(chain.getBlock(chain.latestBlock).header.accountTreeRoot);

	printf("account1: %s\n", toHex(key1.pub).c_str());
	std::string coin1 = amountToCoin(chain.accountTree.get(key1.pub).balance);
	printf("balance1: %s\n", coin1.c_str());

	printf("account2: %s\n", toHex(Key2.pub).c_str());
	std::string coin2 = amountToCoin(chain.accountTree.get(Key2.pub).balance);
	printf("balance2: %s\n", coin2.c_str());
}
