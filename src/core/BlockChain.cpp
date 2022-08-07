#include "BlockChain.h"
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
