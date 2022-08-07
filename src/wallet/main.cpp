
#include "core/Node.h"
#include "Wallet.h"
#include "util/hex.h"
#include <iostream>
#include <filesystem>

std::vector<std::string> split(const std::string& string, const std::string& delimiter, bool includeEmpty = true) {
    std::vector<std::string> parts;
    std::string token;
    int delimiterIndex = 0;
    for (char c : string) {
        if ((int)delimiter.size() == 0) {
            parts.push_back({ c, 1 });
        }
        else if (c == delimiter[delimiterIndex]) {
            delimiterIndex++;
            if (delimiterIndex == delimiter.size()) {
                if (includeEmpty || (int)token.size() != 0) {
                    parts.push_back(token);
                }
                token.clear();
                delimiterIndex = 0;
            }
        }
        else {
            token += delimiter.substr(0, delimiterIndex);
            token.push_back(c);
            delimiterIndex = 0;
        }
    }
    token += delimiter.substr(0, delimiterIndex);
    if (includeEmpty || (int)token.size() != 0) {
        parts.push_back(token);
    }
    return parts;
}

bool loadFile(Wallet &wallet) {
    std::vector<std::string> files;
    for (auto& file : std::filesystem::directory_iterator("..\\wallets")) {
        printf("(%i) %s\n", (int)files.size(), file.path().filename().string().c_str());
        files.push_back(file.path().string());
    }
    printf("(%i) new\n", (int)files.size());

    printf("select number> ");
    std::string line;
    if (std::getline(std::cin, line)) {
        if (line == "exit") {
            return false;
        }

        int num = -1;
        try {
            num = std::stoi(line);
        }
        catch (...) {
            printf("invalid number\n");
        }
        if (num != -1) {
            if (num >= 0 && num < files.size()) {
                if (!wallet.init(files[num])) {
                    wallet.createKey();
                }
            }
            else if (num == files.size()) {
                printf("file> ");
                if (std::getline(std::cin, line)) {
                    line = std::string("..\\wallets\\") + line;
                    if (!wallet.init(line)) {
                        wallet.createKey();
                    }
                }
            }
        }
    }

    return true;
}

int main(int argc, char* args[]) {
	Node node;
	node.init("..\\chainData\\", true);
    if (!node.validateChain(true, 2)) {
        node.db.save();
    }

	Wallet wallet;

    if (!loadFile(wallet)) {
        return 0;
    }

	while (true) {
		std::string line;
        printf("> ");
		if (std::getline(std::cin, line)) {
            auto parts = split(line, " ", false);

            if (parts.size() > 0) {
                if (parts[0] == "exit") {
                    break;
                }
                else if (parts[0] == "info") {
                    node.db.load();
                    node.validateChain(true, 1);

                    AccountEntry account = node.chain.state.getAccount(wallet.publicKey);
                    std::string str;
                    toHex(wallet.publicKey, str);
                    printf("address: %s\n", str.c_str());
                    printf("balance: %i\n", (int)account.balance);
                    printf("nonce:   %i\n", (int)account.nonce);
                }
                else if (parts[0] == "send") {
                    if (parts.size() >= 3) {
                        node.db.load();
                        node.validateChain(true, 1);

                        EccPublicKey to;
                        fromHex(parts[1], to);
                        Amount amount = 0;
                        try {
                            amount = std::stoi(parts[2]);
                        }
                        catch (...) {
                            printf("invalid amount\n");
                        }

                        Amount fee = 0;
                        if (parts.size() >= 4) {
                            try {
                                fee = std::stoi(parts[3]);
                            }
                            catch (...) {
                                printf("invalid fee\n");
                            }
                        }

                        Transaction transaction;
                        TransactionError error = node.creator.createTransaction(transaction, wallet.publicKey, to, amount, fee, wallet.privateKey);
                        if (error != TransactionError::NONE) {
                            printf("transaction failed with code %i\n", error);
                        }
                        else {
                            node.creator.addTransaction(transaction);
                            node.creator.createBlock(wallet.publicKey, wallet.publicKey, wallet.privateKey);
                            node.db.save();
                        }
                    }
                    else {
                        printf("usage: send <address> <amount> [fee]\n");
                    }
                }
                else if (parts[0] == "load") {
                    if (!loadFile(wallet)) {
                        return 0;
                    }
                }
                else if (parts[0] == "cutchain") {
                    uint32_t num = -1;
                    if (parts.size() >= 1) {
                        try {
                            num = std::stoi(parts[1]);
                        }
                        catch (...) {
                            printf("invalid number\n");
                        }

                        if (num != -1) {
                            if (num >= 1 && num < node.chain.getBlockCount()) {
                                node.chain.blocks.resize(num);
                                node.db.save();
                            }
                        }
                    }
                }
                else if (parts[0] == "accounts") {
                    node.db.load();
                    node.validateChain(true, 1);

                    Amount sum = 0;
                    for (auto& acc : node.chain.state.getAllAccounts()) {
                        std::string str;
                        toHex(acc.first, str);
                        printf("account: %s, balance: %i\n", str.c_str(), (int)acc.second.balance);
                        sum += acc.second.balance;
                    }
                    printf("sum of balances: %i\n", (int)sum);
                }
                else if (parts[0] == "blocks") {
                    node.db.load();
                    node.validateChain(true, 1);

                    for (int i = 0; i < node.chain.getBlockCount(); i++) {
                        Block block;
                        Hash blockHash = node.chain.blocks[i];
                        node.db.getBlock(blockHash, block);


                        printf("block %i\n", (int)block.header.blockNumber);
                        std::string str;
                        toHex(block.getHash(), str);
                        printf("  hash:     %s\n", str.c_str());
                        printf("  time:     %i\n", (int)block.header.timestamp);
                        printf("  tx count: %i\n", (int)block.transactionTree.transactionHashes.size());
                    }
                }
                else if (parts[0] == "transactions") {
                    node.db.load();
                    node.validateChain(true, 1);

                    int index = 0;
                    for (int i = 0; i < node.chain.getBlockCount(); i++) {
                        Block block;
                        Hash blockHash = node.chain.blocks[i];
                        node.db.getBlock(blockHash, block);

                        for (int j = 0; j < block.transactionTree.transactionHashes.size(); j++) {
                            Hash hash = block.transactionTree.transactionHashes[j];
                            TransactionHeader transaction;
                            if (node.db.getTransactionHeader(hash, transaction)) {
                                printf("transaction %i\n", index++);
                                std::string str;
                                toHex(transaction.from, str);
                                printf("  from:   %s\n", str.c_str());
                                toHex(transaction.to, str);
                                printf("  to:     %s\n", str.c_str());
                                printf("  amount: %i\n", (int)transaction.amount);
                                printf("  fee:    %i\n", (int)transaction.fee);
                                printf("  time:   %i\n", (int)transaction.timestamp);
                                toHex(hash, str);
                                printf("  hash:   %s\n", str.c_str());
                                printf("  block:  %i\n", i);
                            }
                        }
                    }
                }
                else if (parts[0] == "help") {
                    printf("commands: info, send, load, accounts, blocks, transactions\n");
                }
                else {
                    printf("unknown command\n");
                }
            }
            else {
                printf("unknown command\n");
            }
		}
	}

	return 0;
}
