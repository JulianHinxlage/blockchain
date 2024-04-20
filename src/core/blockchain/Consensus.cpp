//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "Consensus.h"
#include "BlockChain.h"
#include "cryptography/sha.h"

EccPublicKey Consensus::selectNextValidator(const BlockHeader& block, uint32_t slot) {
	Serializer serializer;
	serializer.write(block.seed);
	serializer.write(slot);
	Hash rng = sha256(serializer.toString());

	if (block.totalStakeAmount == 0) {
		return EccPublicKey(0);
	}
	
	double range = ((double)(uint64_t)rng) / ((double)(uint64_t)-1);
	uint64_t numMax = block.totalStakeAmount / blockChain->config.minimumStakeAmount;
	uint64_t num = range * numMax;
	if (num >= numMax) {
		num = numMax - 1;
	}

	return blockChain->getValidatorTree(block.validatorTreeRoot).get(num);
}

bool Consensus::forkChoice(const BlockHeader& a, const BlockHeader& b) {
	if (a.blockNumber > b.blockNumber) {
		return false;
	}
	if (a.blockNumber < b.blockNumber) {
		return true;
	}
	if (a.slot < b.slot) {
		return false;
	}
	if (a.slot > b.slot) {
		return true;
	}
	return false;
}
