//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Block.h"

class Consensus {
public:
	class BlockChain* blockChain = nullptr;

	EccPublicKey selectNextValidator(const BlockHeader& block, uint32_t slot);

	//returns true if block b should be chosen over a
	bool forkChoice(const BlockHeader& a, const BlockHeader& b);
};
