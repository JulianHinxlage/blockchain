//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Transaction.h"
#include "BinaryTree.h"
#include <string>

class Account {
public:
	uint32_t transactionCount = 0;
	Amount balance = 0;
	Amount stakeAmount = 0;
	EccPublicKey stakeOwner = EccPublicKey(0);
	uint64_t validatorNumber = 0;
	uint64_t stakeBlockNumber = 0;
	Hash data = Hash(0);
	Hash code = Hash(0);

	std::string serial() const;
	int deserial(const std::string& str);
};

typedef BinaryTree<EccPublicKey, Account, true> AccountTree;
