//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Transaction.h"
#include "Block.h"
#include "Account.h"
#include "util/hex.h"

class BlockChainConfig {
public:
	uint32_t transactionVersion;
	uint32_t blockVersion;
	Amount minimumStakeAmount;
	bool uniformStakeSize;
	uint64_t stakeMaturityBlockCount;
	Hash genesisBlockHash;
	Block genesisBlock;
	int slotTime;

	void initDevNet(AccountTree &accountTree) {
		transactionVersion = 1;
		blockVersion = 1;
		minimumStakeAmount = coinToAmount("1000");
		stakeMaturityBlockCount = 10;
		uniformStakeSize = true;
		slotTime = 10;

		EccPublicKey genesisAddress = fromHex<EccPublicKey>("");
		Account genesisAccount;
		genesisAccount.transactionCount = 0;
		genesisAccount.balance = coinToAmount("1000000000");

		accountTree.reset();
		accountTree.set(genesisAddress, genesisAccount);

		genesisBlock = Block();
		genesisBlock.header.version = blockVersion;
		genesisBlock.header.timestamp = 0;
		genesisBlock.header.blockNumber = 0;
		genesisBlock.header.totalStakeAmount = 0;
		genesisBlock.header.previousBlockHash = 0;
		genesisBlock.header.validator = 0;
		genesisBlock.header.beneficiary = 0;
		genesisBlock.header.slot = 0;
		genesisBlock.header.seed = 0;
		genesisBlock.header.transactionTreeRoot = genesisBlock.transactionTree.calculateRoot();
		genesisBlock.header.accountTreeRoot = accountTree.getRoot();
		genesisBlock.header.signature = 0;
		genesisBlock.blockHash = genesisBlock.header.caclulateHash();
		genesisBlockHash = genesisBlock.blockHash;
	}
};
