#include "BlockChain.h"
#include "DataBase.h"
#include "cryptography/sha.h"
#include <time.h>

BlockChain::BlockChain() {

}

int BlockChain::getBlockCount() {
	return blocks.size();
}

Hash BlockChain::getLatestBlock() {
	if (blocks.size() > 0) {
		return blocks.back();
	}
	else {
		return Hash(0);
	}
}

void BlockChain::addBlock(const Block& block) {
	int num = block.header.blockNumber;
	if (num >= blocks.size()) {
		//switch to new longer chain
		std::vector<Hash> hashes;
		Hash hash = block.blockHash;
		hashes.push_back(hash);
		hash = block.header.previousBlock;

		Block b;
		while (db->getBlock(hash, b)) {
			if (b.header.blockNumber < blocks.size()) {
				break;
			}
			hashes.push_back(hash);
			hash = b.header.previousBlock;
		}

		for (int i = 0; i < hashes.size(); i++) {
			blocks.push_back(hashes[i]);
		}
	}
	db->addBlock(block);
}
