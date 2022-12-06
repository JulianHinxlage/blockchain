#include "BlockChainConfig.h"
#include "util/hex.h"

BlockChainConfig::BlockChainConfig() {
	version = 0;
	genesisBlockHash = 0;
}

void BlockChainConfig::initDevnet() {
	version = 1;
	genesisBlock = Block();
	BlockChainState genesisState;
	getGenesisState(genesisState);

	genesisBlock.header.version = 1;
	genesisBlock.header.timestamp = 1659848723;
	genesisBlock.header.blockNumber = 0;
	genesisBlock.header.previousBlock = 0;
	genesisBlock.header.transactionRoot = genesisBlock.transactionTree.getHash();
	genesisBlock.header.beneficiary = 0;
	genesisBlock.header.validator = 0;
	genesisBlock.header.signature = 0;
	
	genesisBlock.header.stateRoot = genesisState.getHash();
	
	genesisBlock.blockHash = genesisBlock.header.getHash();
	genesisBlockHash = genesisBlock.blockHash;
}

Hash BlockChainConfig::getGenesisBlockHash() {
	return genesisBlockHash;
}

const Block& BlockChainConfig::getGenesisBlock() {
	return genesisBlock;
}

void BlockChainConfig::getGenesisState(BlockChainState& state) {
	EccPublicKey genesisAddress;
	fromHex("304086d46f6203cd63408700ddc6158e0cc693249c45ae9a06f0fefce274793e44", genesisAddress); 
	
	Account genesisAccount;
	genesisAccount.balance = 1'000'000'000;
	genesisAccount.nonce = 0;
	state.loadState(Hash(0));
	state.setAccount(genesisAddress, genesisAccount);
}

int BlockChainConfig::getVersion() {
	return version;
}
