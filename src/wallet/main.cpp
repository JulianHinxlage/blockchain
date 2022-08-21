#include "core/Node.h"
#include "Wallet.h"
#include "core/ChainNetwork.h"
#include "util/hex.h"
#include "util/strutil.h"
#include <iostream>
#include <filesystem>

int main(int argc, char* args[]) {
    Node node;
    ChainNetwork network;
    network.db = &node.db;
    network.chain = &node.chain;
    node.chain.config.initDevnet();
    Wallet wallet;

    if (!wallet.init(wallet.selectFile("../wallets"))) {
        wallet.createKey();
    }

    std::string directory = wallet.selectDirectory("../chains");
    if (directory.empty()) {
        return 0;
    }

    node.init(directory, true);
    //if (!node.validateChain(true, 2)) {
    //    node.db.save();
    //}

    network.network.logCallback = [&](const std::string& msg, int level) {
        if (level > 0) {
            printf("log: %s\n", msg.c_str());
        }
    };

    network.init("../entryNodes.txt");

    network.onBlockReceived = [&](const Block& block) {
        if (block.blockHash == block.header.getHash()) {
            std::string str;
            toHex(block.blockHash, str);
            BlockError error = node.validator.validateBlock(block);

            if (error == BlockError::NONE) {
                printf("block: %s\n", str.c_str());
                node.chain.addBlock(block);
                node.db.save();
            }
            else {
                printf("invalid block: %s with code %i\n", str.c_str(), error);
            }
        }
    };
    network.onTransactionReceived = [&](const Transaction& transaction, bool partOfBlock) {
        if (transaction.transactionHash == transaction.header.getHash()) {
            if (!partOfBlock) {
                std::string str;
                toHex(transaction.transactionHash, str);
                printf("tx: %s\n", str.c_str());
            }
            node.db.addTransaction(transaction);
        }
    };




    printf("\n");
    {
        std::string str;
        toHex(node.chain.getLatestBlock(), str);
        printf("chain head: block %i hash %s\n", node.chain.getBlockCount() - 1, str.c_str());
    }
    network.syncChain();
    {
        std::string str;
        toHex(node.chain.getLatestBlock(), str);
        printf("chain head: block %i hash %s\n", node.chain.getBlockCount() - 1, str.c_str());
    }
    printf("\n");


    Account account = node.chain.state.getAccount(wallet.publicKey);


    while (true) {
        std::string line;
        printf("> ");
        if (std::getline(std::cin, line)) {
            auto parts = strSplit(line, " ", false);

            if (parts.size() > 0) {
                if (parts[0] == "exit") {
                    break;
                }
                else if (parts[0] == "info") {

                    Account account = node.chain.state.getAccount(wallet.publicKey);
                    std::string str;
                    toHex(wallet.publicKey, str);
                    printf("address:      %s\n", str.c_str());
                    printf("balance:      %i\n", (int)account.balance);
                    printf("transactions: %i\n", (int)account.nonce);
                }
                else if (parts[0] == "send") {
                    if (parts.size() >= 3) {

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
                        TransactionError error = node.creator.createTransaction(transaction, wallet.publicKey, to, amount, fee, wallet.privateKey, &account);
                        if (error != TransactionError::NONE) {
                            printf("transaction failed with code %i\n", error);
                        }
                        else {
                            account.nonce++;
                            account.balance -= transaction.header.amount + transaction.header.fee;

                            network.broadcastTransaction(transaction);
                            node.db.addTransaction(transaction);
                        }
                    }
                    else {
                        printf("usage: send <address> <amount> [fee]\n");
                    }
                }
                else if (parts[0] == "multisend") {
                    if (parts.size() > 3) {

                        int count = 0;
                        try {
                            count = std::stoi(parts[1]);
                        }
                        catch (...) {
                            printf("invalid count\n");
                        }

                        EccPublicKey to;
                        fromHex(parts[2], to);
                        Amount amount = 0;
                        try {
                            amount = std::stoi(parts[3]);
                        }
                        catch (...) {
                            printf("invalid amount\n");
                        }

                        Amount fee = 0;
                        if (parts.size() > 4) {
                            try {
                                fee = std::stoi(parts[4]);
                            }
                            catch (...) {
                                printf("invalid fee\n");
                            }
                        }

                        for (int i = 0; i < count; i++) {
                            Transaction transaction;
                            TransactionError error = node.creator.createTransaction(transaction, wallet.publicKey, to, amount, fee, wallet.privateKey, &account);
                            if (error != TransactionError::NONE) {
                                printf("transaction failed with code %i\n", error);
                            }
                            else {
                                account.nonce++;
                                account.balance -= transaction.header.amount + transaction.header.fee;

                                network.broadcastTransaction(transaction);
                                node.db.addTransaction(transaction);
                            }
                        }

                    }
                    else {
                        printf("usage: multisend <count> <address> <amount> [fee]\n");
                    }
                }
                else if (parts[0] == "load") {
                    if (!wallet.init(wallet.selectFile("../wallets"))) {
                        wallet.createKey();
                    }
                }
                else if (parts[0] == "sync") {
                    printf("\n");
                    {
                        std::string str;
                        toHex(node.chain.getLatestBlock(), str);
                        printf("chain head: block %i hash %s\n", node.chain.getBlockCount() - 1, str.c_str());
                    }
                    network.syncChain();
                    {
                        std::string str;
                        toHex(node.chain.getLatestBlock(), str);
                        printf("chain head: block %i hash %s\n", node.chain.getBlockCount() - 1, str.c_str());
                    }
                    printf("\n");
                    account = node.chain.state.getAccount(wallet.publicKey);
                }
                else if (parts[0] == "validate") {
                    if (!node.validateChain(true, 2)) {
                        node.db.save();
                    }
                }
                else if (parts[0] == "cutchain") {
                    uint32_t num = -1;
                    if (parts.size() >= 2) {
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

                    for (int i = 0; i < node.chain.getBlockCount(); i++) {
                        Block block;
                        Hash blockHash = node.chain.blocks[i];
                        node.db.getBlock(blockHash, block);


                        printf("block %i\n", (int)block.header.blockNumber);
                        std::string str;
                        toHex(block.getHash(), str);
                        printf("  hash:     %s\n", str.c_str());
                        printf("  time:     %i\n", (int)block.header.timestamp);

                        toHex(block.header.transactionRoot, str);
                        printf("  tx root:  %s\n", str.c_str());
                        toHex(block.header.stateRoot, str);
                        printf("  state:    %s\n", str.c_str());

                        printf("  tx count: %i\n", (int)block.transactionTree.transactionHashes.size());
                    }
                }
                else if (parts[0] == "transactions") {

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
                                toHex(hash, str);
                                printf("  hash:   %s\n", str.c_str());
                                toHex(transaction.from, str);
                                printf("  from:   %s\n", str.c_str());
                                toHex(transaction.to, str);
                                printf("  to:     %s\n", str.c_str());
                                printf("  amount: %i\n", (int)transaction.amount);
                                printf("  fee:    %i\n", (int)transaction.fee);
                                printf("  time:   %i\n", (int)transaction.timestamp);
                                printf("  block:  %i\n", i);
                            }
                        }
                    }
                }
                else if (parts[0] == "help") {
                    printf("commands: info, send, load, sync, accounts, blocks, transactions\n");
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

    network.shutdown();
    return 0;
}
