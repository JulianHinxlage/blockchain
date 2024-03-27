//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Transaction.h"
#include "BinaryTree.h"

class Account {
public:
	uint32_t transactionCount = 0;
	uint32_t unused = 0;
	uint64_t balance = 0;
};

typedef BinaryTree<EccPublicKey, Account> AccountTree;
