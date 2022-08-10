#include "BlockCreator.h"
#include "DataBase.h"
#include "util/hex.h"
#include "cryptography/sha.h"
#include <time.h>

BlockCreateor::BlockCreateor() {
	chain = nullptr;
	reset();
}

void BlockCreateor::reset() {
	totalFees = 0;
	block = Block();
}

TransactionError BlockCreateor::createTransaction(Transaction& transaction, const EccPublicKey& from, const EccPublicKey& to, Amount amount, Amount fee, const EccPrivateKey& key) {
	AccountEntry account = chain->state.getAccount(from);
	if (account.balance < amount + fee) {
		return TransactionError::INVALID_BALANCE;
	}
	if (eccToPublicKey(key) != from) {
		return TransactionError::INVALID_KEY;
	}

	transaction.header.version = chain->config.version;
	transaction.header.timestamp = time(nullptr);
	transaction.header.from = from;
	transaction.header.to = to;
	transaction.header.amount = amount;
	transaction.header.fee = fee;
	transaction.header.nonce = account.nonce;
	transaction.header.dataHash = sha256(transaction.data.data(), transaction.data.size());
	transaction.header.sign(key);
	transaction.transactionHash = transaction.header.getHash();
	return TransactionError::NONE;
}

TransactionError BlockCreateor::addTransaction(const Transaction& transaction) {
	if (transaction.getHash() != transaction.header.getHash()) {
		return TransactionError::INVALID_HASH;
	}
	
	if (transaction.header.version != chain->config.version) {
		return TransactionError::INVALID_VERSION;
	}

	if (!eccValidPublicKey(transaction.header.from)) {
		return TransactionError::INVALID_FROM;
	}
	if (!eccValidPublicKey(transaction.header.from)) {
		return TransactionError::INVALID_TO;
	}

	if (transaction.header.fee + transaction.header.amount < transaction.header.fee) {
		return TransactionError::INVALID_FEE;
	}
	if (transaction.header.fee + transaction.header.amount < transaction.header.amount) {
		return TransactionError::INVALID_FEE;
	}

	AccountEntry fromAccount = chain->state.getAccount(transaction.header.from);
	AccountEntry toAccount = chain->state.getAccount(transaction.header.to);
	if (fromAccount.nonce != transaction.header.nonce) {
		return TransactionError::INVALID_NONCE;
	}

	if (transaction.header.amount > fromAccount.balance) {
		return TransactionError::INVALID_BALANCE;
	}
	if (transaction.header.fee > fromAccount.balance) {
		return TransactionError::INVALID_BALANCE;
	}
	if (transaction.header.amount + transaction.header.fee > fromAccount.balance) {
		return TransactionError::INVALID_BALANCE;
	}

	if (totalFees + transaction.header.fee < totalFees) {
		return TransactionError::INVALID_FEE;
	}
	if (toAccount.balance + transaction.header.amount < toAccount.balance) {
		return TransactionError::INVALID_BALANCE;
	}

	if (!transaction.header.verifySignature()) {
		return TransactionError::INVALID_SIGNATURE;
	}

	block.transactionTree.transactionHashes.push_back(transaction.getHash());
	totalFees += transaction.header.fee;
	fromAccount.balance -= transaction.header.amount + transaction.header.fee;
	fromAccount.nonce++;
	chain->state.setAccount(transaction.header.from, fromAccount);
	toAccount = chain->state.getAccount(transaction.header.to);
	toAccount.balance += transaction.header.amount;
	chain->state.setAccount(transaction.header.to, toAccount);
	chain->db->addTransaction(transaction);
	return TransactionError::NONE;
}

BlockError BlockCreateor::createBlock(const EccPublicKey& beneficiary, const EccPublicKey& validator, const EccPrivateKey& key, Block* blockOutput) {
	if (!eccValidPublicKey(beneficiary)) {
		return BlockError::INVALID_BENEFICIARY;
	}
	if (!eccValidPublicKey(validator)) {
		return BlockError::INVALID_VALIDATOR;
	}
	if (eccToPublicKey(key) != validator) {
		return BlockError::INVALID_KEY;
	}

	//add fees to beneficiary
	AccountEntry beneficiaryAccount = chain->state.getAccount(beneficiary);
	beneficiaryAccount.balance += totalFees;

	if (beneficiaryAccount.balance + totalFees < beneficiaryAccount.balance) {
		return BlockError::INVALID_FEE;
	}

	block.header.version = chain->config.version;
	block.header.timestamp = time(nullptr);
	block.header.blockNumber = chain->getBlockCount();
	block.header.previousBlock = chain->getLatestBlock();
	block.header.transactionRoot = block.transactionTree.getHash();
	block.header.beneficiary = beneficiary;
	block.header.validator = validator;


	chain->state.setAccount(beneficiary, beneficiaryAccount);
	block.header.stateRoot = chain->state.getHash();

	block.header.sign(key);
	block.blockHash = block.header.getHash();

	chain->addBlock(block);
	if (blockOutput) {
		*blockOutput = block;
	}

	reset();
	return BlockError::NONE;
}

BlockError BlockCreateor::validateBlock(const Block& block) {
	reset();

	if (block.blockHash != block.header.getHash()) {
		return BlockError::INVALID_HASH;
	}

	Hash genesisBlockHash;
	fromHex("03d81123767aa201dd1a412ff1f425f54fa4d58f9aca908d61b2bc5cc15a3e18", genesisBlockHash);
	if (block.blockHash == genesisBlockHash) {
		if (chain->state.getHash() != Hash(0)) {
			return BlockError::INVALID_STATE;
		}
		return BlockError::NONE;
	}

	if (block.header.version != chain->config.version) {
		return BlockError::INVALID_VERSION;
	}

	BlockHeader previous;
	if (!chain->db->getBlockHeader(block.header.previousBlock, previous)) {
		return BlockError::INVALID_PREVIOUS;
	}
	if (block.header.blockNumber != previous.blockNumber + 1) {
		return BlockError::INVALID_BLOCK_NUMBER;
	}

	if (!eccValidPublicKey(block.header.beneficiary)) {
		return BlockError::INVALID_BENEFICIARY;
	}
	if (!eccValidPublicKey(block.header.validator)) {
		return BlockError::INVALID_VALIDATOR;
	}

	if (block.header.transactionRoot != block.transactionTree.getHash()) {
		return BlockError::INVALID_TRANSACTION_ROOT;
	}

	if (chain->state.getHash() != previous.stateRoot) {
		if (!getBlockChainState(chain->state, block.header.previousBlock)) {
			return BlockError::INVALID_STATE;
		}
		if (chain->state.getHash() != previous.stateRoot) {
			return BlockError::INVALID_STATE;
		}
	}

	if (!block.header.verifySignature()) {
		return BlockError::INVALID_SIGNATURE;
	}

	//recreate block state
	for (int i = 0; i < block.transactionTree.transactionHashes.size(); i++) {
		Hash hash = block.transactionTree.transactionHashes[i];
		Transaction transaction;
		if (!chain->db->getTransaction(hash, transaction)) {
			return BlockError::INVALID_TRANSACTION;
		}
		addTransaction(transaction);
	}

	AccountEntry beneficiaryAccount = chain->state.getAccount(block.header.beneficiary);
	beneficiaryAccount.balance += totalFees;
	if (beneficiaryAccount.balance + totalFees < beneficiaryAccount.balance) {
		return BlockError::INVALID_FEE;
	}
	chain->state.setAccount(block.header.beneficiary, beneficiaryAccount);

	if (chain->state.getHash() != block.header.stateRoot) {
		return BlockError::INVALID_STATE;
	}

	return BlockError::NONE;
}

bool BlockCreateor::getBlockChainState(BlockChainState& state, const Hash& blockHash) {
	Block block;
	chain->config.createGenesis(block, state);
	chain->config.genesisBlockHash;

	if (chain->db->getBlock(blockHash, block)) {
		int num = block.header.blockNumber;
		if (num >= 0 && num < chain->blocks.size() && chain->blocks[num] == blockHash) {
			
			for (int i = 1; i <= num; i++) {
				if (!chain->db->getBlock(chain->blocks[i], block)) {
					return false;
				}
				if (processBlock(block, state, chain->db) != BlockError::NONE) {
					return false;
				}
			}
		}
		else {
			Hash hash = blockHash;
			std::vector<Hash> blockList;

			while (hash != chain->config.genesisBlockHash) {
				blockList.push_back(hash);
				if (!chain->db->getBlock(hash, block)) {
					return false;
				}
				hash = block.header.previousBlock;
			}

			for (int i = 0; i < blockList.size(); i++) {
				if (!chain->db->getBlock(blockList[i], block)) {
					return false;
				}
				if (processBlock(block, state, chain->db) != BlockError::NONE) {
					return false;
				}
			}
		}
	}

	return true;
}

void BlockCreateor::processTransaction(const Transaction& transaction, BlockChainState& state) {
	AccountEntry fromAccount = state.getAccount(transaction.header.from);
	fromAccount.balance -= transaction.header.amount + transaction.header.fee;
	fromAccount.nonce++;
	chain->state.setAccount(transaction.header.from, fromAccount);
	
	AccountEntry toAccount = state.getAccount(transaction.header.to);
	toAccount = chain->state.getAccount(transaction.header.to);
	toAccount.balance += transaction.header.amount;
	chain->state.setAccount(transaction.header.to, toAccount);
}

BlockError BlockCreateor::processBlock(const Block& block, BlockChainState& state, DataBase* db) {
	Amount totalFees = 0;

	for (int i = 0; i < block.transactionTree.transactionHashes.size(); i++) {
		Hash hash = block.transactionTree.transactionHashes[i];
		Transaction transaction;
		if (!db->getTransaction(hash, transaction)) {
			return BlockError::INVALID_TRANSACTION;
		}
		totalFees += transaction.header.fee;
		processTransaction(transaction, state);
	}

	AccountEntry beneficiaryAccount = state.getAccount(block.header.beneficiary);
	beneficiaryAccount.balance += totalFees;
	chain->state.setAccount(block.header.beneficiary, beneficiaryAccount);

	return BlockError::NONE;
}
