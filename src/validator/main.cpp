#include "Validator.h"
#include "util/hex.h"
#include <filesystem>
#include <iostream>

int main(int argc, char* args[]) {
	Validator validator;
    validator.network.db = &validator.node.db;
    validator.network.chain = &validator.node.chain;
    validator.node.chain.config.initDevnet();

    if (!validator.wallet.init(validator.wallet.selectFile("../wallets"))) {
        validator.wallet.createKey();
    }

	std::string directory = validator.wallet.selectDirectory("../chains");
    if (directory.empty()) {
        return 0;
    }
	validator.node.init(directory, true);
    //if (!validator.node.validateChain(true, 2)) {
    //    validator.node.db.save();
    //}

    validator.network.network.logCallback = [&](const std::string& msg, int level) {
        if (level > 0) {
            //printf("log: %s\n", msg.c_str());
        }
    };

	validator.network.init("../entryNodes.txt");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    validator.init();

    printf("\n");
    {
        std::string str;
        toHex(validator.node.chain.getLatestBlock(), str);
        printf("chain head: block %i hash %s\n", validator.node.chain.getBlockCount() - 1, str.c_str());
    }
    validator.network.syncChain();
    {
        std::string str;
        toHex(validator.node.chain.getLatestBlock(), str);
        printf("chain head: block %i hash %s\n", validator.node.chain.getBlockCount() - 1, str.c_str());
    }
    printf("\n");

    validator.run();
    validator.network.shutdown();
	return 0;
}
