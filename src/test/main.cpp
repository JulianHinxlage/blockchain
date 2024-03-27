//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "blockchain/BlockChain.h"
#include "blockchain/BlockCreator.h"
#include "blockchain/BlockValidator.h"
#include "util/hex.h"
#include <filesystem>

class Wallet {
public:
	EccPrivateKey priv;
	EccPublicKey pub;

	void createKey(const std::string &file) {
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


int main(int argc, char* argv[]) {
	BlockChain chain;
	chain.init("chain");
	printf("blockchain loaded with %i blocks\n", (int)chain.blockList.size());
	int transactionCount = 0;
	for (auto& h : chain.blockList) {
		transactionCount += chain.getBlock(h).header.transactionCount;
	}
	printf("%i transactions\n", transactionCount);
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
				minInvalidIndex = index;
			}
			index++;
		}
		if (!allValid) {
			printf("invalid blocks in blockchain found\n");

			if (cutChain) {
				chain.blockList.resize(minInvalidIndex);
				chain.blockCount = chain.blockList.size();
				if (chain.blockList.size() > 0) {
					chain.latestBlock = chain.blockList.back();
				}
				else {
					chain.latestBlock = Hash(0);
				}
				chain.saveBlockList();
				chain.accountTree.reset(chain.getBlock(chain.latestBlock).header.accountTreeRoot);
			}
		}
	}

	Wallet wallet1;
	wallet1.loadKey("key1.txt");

	Wallet wallet2;
	wallet2.loadKey("key2.txt");

	BlockCreator creator;
	creator.blockChain = &chain;
	creator.beginBlock(wallet2.pub);

	for (int i = 0; i < 1; i++) {
		Transaction tx = creator.createTransaction(wallet1.pub, wallet2.pub, 0, 0);
		tx.header.sign(wallet1.priv);
		tx.transactionHash = tx.header.caclulateHash();
		creator.addTransaction(tx);
	}

	Block block = creator.endBlock();
	block.header.sign(wallet2.priv);
	block.blockHash = block.header.caclulateHash();
	printf("block %s\n", toHex(block.blockHash).c_str());


	BlockError error = validator.validateBlock(block);
	if (error != BlockError::VALID) {
		printf("Block Error: %s\n", blockErrorToString(error));
		system("pause");
	}
	else {
		chain.addBlock(block);
		chain.addBlockToTip(block.blockHash);
	}

	printf("account1: %s\n", toHex(wallet1.pub).c_str());
	printf("balance1: %i\n",  (int)chain.accountTree.get(wallet1.pub).balance);
	
	printf("account2: %s\n", toHex(wallet2.pub).c_str());
	printf("balance2: %i\n", (int)chain.accountTree.get(wallet2.pub).balance);

	return 0;
}
