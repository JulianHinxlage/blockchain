#include "Node.h"

Node::Node() {

}

bool Node::init(const std::string& dataDirectory, bool verbose) {
	db.init(dataDirectory, &chain);
	validator.chain = &chain;
	creator.chain = &chain;

	if (chain.blocks.size() == 0) {
		//create chain with genesis block
		Block block = chain.config.getGenesisBlock();

		if (block.blockHash != chain.config.getGenesisBlockHash()) {
			if (verbose) {
				printf("genesis block hash mismatch\n");
			}
			return false;
		}

		auto error = validator.validateBlock(block);
		if (error != BlockError::NONE) {
			if (verbose) {
				printf("invalid genesis block\n");
			}
			return false;
		}

		chain.db->addBlock(block);
		chain.blocks.push_back(block.getHash());
		chain.config.getGenesisState(chain.state);
	}
	return true;
}

bool Node::validateChain(bool reduceChainToValid, int verbose) {
	bool valid = true;
	BlockChainState startState = chain.state;
	chain.state = BlockChainState();

	if (verbose > 1) {
		printf("validating chain started\n");
	}

	int validBlockCount = 0;
	for (int i = 0; i < chain.getBlockCount(); i++) {

		Hash hash = chain.blocks[i];
		Block block;
		if (!chain.db->getBlock(hash, block)) {
			if (verbose > 0) {
				printf("block %i not found\n", i);
			}
			valid = false;
		}

		BlockError error = validator.validateBlock(block);
		if (error != BlockError::NONE) {
			if (verbose > 0) {
				printf("block error %i in block %i\n", error, i);
			}
			valid = false;
		}

		if (block.getHash() == chain.config.getGenesisBlockHash()) {
			chain.config.getGenesisState(chain.state);
		}

		if (!valid) {
			break;
		}
		else {
			validBlockCount++;
		}
	}

	if (validBlockCount == 0) {
		Block genesis = chain.config.getGenesisBlock();
		chain.addBlock(genesis);
	}

	if (valid) {
		if (chain.state.getHash() != startState.getHash()) {
			if (verbose > 0) {
				printf("chain state mismatch\n");
				valid = false;
			}
		}
		if (!reduceChainToValid) {
			chain.state = startState;
		}
	}
	else {
		if (reduceChainToValid) {
			chain.blocks.resize(validBlockCount);
			if (verbose > 0) {
				printf("chain reduced to %i blocks\n", validBlockCount);
			}
		}
	}

	if (verbose > 1) {
		printf("validating chain finished\n");
	}
	return valid;
}
