#include "BlockChainConfig.h"
#include "util/hex.h"

BlockChainConfig::BlockChainConfig() {
	version = 1;
	fromHex("03d81123767aa201dd1a412ff1f425f54fa4d58f9aca908d61b2bc5cc15a3e18", genesisBlockHash);
}

void BlockChainConfig::createGenesis(Block& block, BlockChainState& state) {
	//development genesis block
	block = Block();
	state = BlockChainState();
	EccPublicKey genesisAddress;
	fromHex("304086d46f6203cd63408700ddc6158e0cc693249c45ae9a06f0fefce274793e44", genesisAddress);
	fromHex("0354200246e7afb3ac34f8db0f3e03e7e432f82fc813a464191e6e5cbc3eed57b1a22d4b201200cb520d59d32d9632691e2d34d7be7562f78e82cfd7dea08e6eac22688a75554500", block.header.signature);

	block.header.version = 1;
	block.header.timestamp = 1659848723;
	block.header.blockNumber = 0;
	block.header.previousBlock = 0;
	block.header.transactionRoot = block.transactionTree.getHash();
	block.header.beneficiary = 0;
	block.header.validator = 0;

	AccountEntry genesisAccount;
	genesisAccount.balance = 1'000'000'000;
	genesisAccount.nonce = 0;
	state.setAccount(genesisAddress, genesisAccount);
	block.header.stateRoot = state.getHash();

	block.blockHash = block.header.getHash();
}
