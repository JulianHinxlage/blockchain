#include "BlockChain.h"

BlockChain::BlockChain() {

}

int BlockChain::getBlockCount() {
	return blocks.size();
}

Hash BlockChain::getLatest() {
	if (blocks.size() > 0) {
		return blocks.back();
	}
	else {
		return Hash(0);
	}
}
