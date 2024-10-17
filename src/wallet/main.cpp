//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "wallet/Wallet.h"
#include "util/util.h"
#include "util/log.h"
#include "util/Terminal.h"

#include <filesystem>
#include <iostream>

#if WIN32
#include <Windows.h>
#include <Psapi.h>
#include <Winternl.h>
#undef max

int processCount(const std::string& name) {
	int count = 0;
	DWORD processesArray[1024];
	DWORD bytesReturned;
	if (EnumProcesses(processesArray, sizeof(processesArray), &bytesReturned)) {
		int numProcesses = bytesReturned / sizeof(DWORD);

		for (int i = 0; i < numProcesses; ++i) {
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processesArray[i]);
			if (hProcess != NULL) {
				char processName[MAX_PATH];
				if (GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName))) {
					std::string pname = processName;
					if (pname == name) {
						count++;
					}
				}

				CloseHandle(hProcess);
			}
		}
	}

	return count;
}
#else
#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>
#include <fstream>
#include <cstring>


int processCount(const std::string& name) {
    int count = 0;
    struct dirent* entry;
    DIR* procDir = opendir("/proc");

    if (!procDir) {
        std::cerr << "Failed to open /proc directory" << std::endl;
        return 0;
    }

    while ((entry = readdir(procDir)) != NULL) {
        if (isdigit(entry->d_name[0])) {
            std::string pidDir = std::string("/proc/") + entry->d_name;
            std::ifstream cmdlineFile(pidDir + "/comm");

            if (cmdlineFile.is_open()) {
                std::string processName;
                std::getline(cmdlineFile, processName);
                cmdlineFile.close();

                if (processName == name) {
                    count++;
                }
            }
        }
    }

    closedir(procDir);
    return count;
}
#endif

std::string search(std::string path) {
	std::string orig = path;
	for (int i = 0; i < 3; i++) {
		if (std::filesystem::exists(path)) {
			return path;
		}
		path = "../" + path;
	}
	if (std::filesystem::is_directory(orig)) {
		std::filesystem::create_directories(orig);
	}
	else {
		std::string parent = std::filesystem::path(orig).parent_path().string();
		if (!parent.empty()) {
			std::filesystem::create_directories(std::filesystem::path(orig).parent_path());
		}
	}
	return orig;
}

int main(int argc, char* argv[]) {
	Terminal terminal;
	terminal.init();
	setLogCallback([&](LogLevel level, const std::string& category, const std::string& msg) {
		terminal.println(msg.c_str());
	});

	int num = 0;
	std::string name = "name";
	if (argc > 0) {
		std::string exe = argv[0];
		name = std::filesystem::path(exe).stem().string();
		num = processCount(std::filesystem::path(exe).filename().string());
	}
	std::string chainDir = search("chains") + "/" + name + "/chain_" + std::to_string(num);
	std::string keyFile = search("keys") + "/" + name + "/key_" + std::to_string(num) + ".txt";


	Wallet wallet;
	wallet.init(chainDir, keyFile, search("entry.txt"));


	while (true) {
		std::string input = terminal.input("> ");

		auto args = split(input, " ", false);
		if (args.size() <= 0) {
			terminal.log("invalid input\n");
			continue;
		}
		std::string cmd = args[0];
		args.erase(args.begin());


		if (cmd == "exit") {
			break;
		}
		else if (cmd == "info") {
			EccPublicKey address = wallet.keyStore.getPublicKey();
			Account account = wallet.getAccount();

			terminal.log("block count:   %i\n", wallet.node.blockChain.getBlockCount());
			terminal.log("chain tip:     %s\n", toHex(wallet.node.blockChain.getHeadBlock()).c_str());
			terminal.log("address:       %s\n", toHex(address).c_str());
			terminal.log("balance:       %s\n", amountToCoin(account.balance).c_str());
			if (account.stakeAmount != 0) {
				terminal.log("stake:         %s\n", amountToCoin(account.stakeAmount).c_str());
				terminal.log("stake block:   %llu\n", account.stakeBlockNumber);
				terminal.log("validator num: %llu\n", account.validatorNumber);
				terminal.log("stake owner:   %s\n", toHex(account.stakeOwner).c_str());
			}
			terminal.log("transactions:  %u\n", account.transactionCount);
		}
		else if (cmd == "blocks") {

			int showCount = 5;
			if (args.size() > 0) {
				try {
					showCount = std::stoi(args[0]);
				}
				catch (...) {}
			}

			int count = wallet.node.blockChain.getBlockCount();
			for (int i = std::max(0, count - showCount); i < count; i++) {
				Hash hash = wallet.node.blockChain.getBlockHash(i);
				Block block = wallet.node.blockChain.getBlock(hash);

				if (i != 0) {
					terminal.log("\n");
				}
				terminal.log("block number:       %i\n", i);
				terminal.log("block slot:         %i\n", block.header.slot);
				terminal.log("transaction count:  %i\n", block.header.transactionCount);
				terminal.log("total stake amount: %s\n", amountToCoin(block.header.totalStakeAmount).c_str());
				terminal.log("block hash:         %s\n", toHex(hash).c_str());
				terminal.log("block rng:          %s\n", toHex(block.header.rng).c_str());
				terminal.log("block validator:    %s\n", toHex(block.header.validator).c_str());
				terminal.log("block beneficiary:  %s\n", toHex(block.header.beneficiary).c_str());
				terminal.log("account tree:       %s\n", toHex(block.header.accountTreeRoot).c_str());
				terminal.log("validator tree:     %s\n", toHex(block.header.validatorTreeRoot).c_str());
			}
		}
		else if (cmd == "transactions") {
			int count = wallet.node.blockChain.getBlockCount();
			for (int i = 0; i < count; i++) {
				Hash hash = wallet.node.blockChain.getBlockHash(i);
				Block block = wallet.node.blockChain.getBlock(hash);
				for (auto& txHash : block.transactionTree.transactionHashes) {
					Transaction tx = wallet.node.blockChain.getTransaction(txHash);

					bool isSender = tx.header.sender == wallet.keyStore.getPublicKey();
					bool isRecipient = tx.header.recipient == wallet.keyStore.getPublicKey();
					if (isSender || isRecipient) {
						terminal.log("\n");
						terminal.log("tx number:      %i\n", (int)tx.header.transactionNumber);
						terminal.log("block number:   %i\n", (int)block.header.blockNumber);
						terminal.log("type:           %i\n", (int)tx.header.type);
						terminal.log("amount:         %s\n", amountToCoin(tx.header.amount).c_str());
						if (isSender) {
							terminal.log("-> to:      %s\n", toHex(tx.header.recipient).c_str());
						}
						if (isRecipient) {
							terminal.log("<- from:    %s\n", toHex(tx.header.sender).c_str());
						}
					}
				}
			}
		}
		else if (cmd == "send") {
			if (args.size() < 2) {
				terminal.log("usage: send <address> <amount> [fee] [type]\n");
			}
			else {
				std::string address = args[0];
				std::string amount = args[1];
				std::string fee = "0";
				if (args.size() > 2) {
					fee = args[2];
				}
				TransactionType type = TransactionType::TRANSFER;
				if (args.size() > 3) {
					std::string str = args[3];
					if (str == "transfer") {
						type = TransactionType::TRANSFER;
					}else if (str == "stake") {
						type = TransactionType::STAKE;
					}
					else if (str == "unstake") {
						type = TransactionType::UNSTAKE;
					}
					else {
						terminal.log("invalid type");
						continue;
					}
				}

				if (wallet.keyStore.isLocked()) {
					auto pass = terminal.passwordInput("password: ");
					if (!wallet.keyStore.unlock(pass)) {
						terminal.log("invalid password\n");
						continue;
					}
				}

				wallet.sendTransaction(address, amount, fee, type);

				wallet.keyStore.lock();
			}
		}
		else if (cmd == "password") {
			if (wallet.keyStore.isLocked()) {
				auto pass = terminal.passwordInput("password: ");
				if (!wallet.keyStore.unlock(pass)) {
					terminal.log("invalid password\n");
					continue;
				}
			}

			auto pass1 = terminal.passwordInput("new password: ");
			auto pass2 = terminal.passwordInput("confirm new password: ");
			if (pass1 == pass2) {
				if (wallet.keyStore.setPassword(pass1)) {
					terminal.log("new password set\n");
				}
			}
			else {
				terminal.log("password mismatch\n");
			}

			wallet.keyStore.lock();
		}
		else if (cmd == "test") {
			wallet.sendTransaction(toHex(wallet.keyStore.getPublicKey()), "0", "0");
		}
		else if (cmd == "test2") {
			while (true) {
				wallet.sendTransaction(toHex(wallet.keyStore.getPublicKey()), "0", "0.001");
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		else if (cmd == "sync") {
			wallet.node.synchronize();
		}
		else if (cmd == "verify") {
			wallet.node.verifyChain();
		}
		else if (cmd == "reset") {
			if (args.size() < 1) {
				terminal.log("usage: reset <block num>\n");
			}
			else {
				try{
					int num = std::stoi(args[0]);
					if(num >= 0 && num < wallet.node.blockChain.getBlockCount()){
						for(int i = wallet.node.blockChain.getBlockCount() - 1; i > num; i--){
							Hash hash = wallet.node.blockChain.getBlockHash(i);
							wallet.node.blockChain.removeBlock(hash);
						}
						wallet.node.blockChain.setHeadBlock(wallet.node.blockChain.getBlockHash(num));
					}else{
						terminal.log("invalid number\n");
					}
				} catch(...){
					terminal.log("invalid number\n");
				}
			}
		}
		else if (cmd == "stats") {
			int count = wallet.node.blockChain.getBlockCount();
			int start = std::max(1, count - 100);

			std::map<int, int> slotCount;
			std::map<EccPublicKey, int> validatorCount;
			int slotSum = 0;
			int slotSumCount = 0;
			int statsCount = 0;

			for (int i = start; i < count; i++) {
				Hash hash = wallet.node.blockChain.getBlockHash(i);
				Block block = wallet.node.blockChain.getBlock(hash);

				if (block.header.slot < 10) {
					slotSum += block.header.slot;
					slotSumCount++;
				}
				slotCount[block.header.slot]++;
				validatorCount[block.header.validator]++;
				statsCount++;
			}

			uint32_t beginTime = wallet.node.blockChain.getBlock(wallet.node.blockChain.getBlockHash(start)).header.timestamp;
			uint32_t endTime = wallet.node.blockChain.getBlock(wallet.node.blockChain.getBlockHash(count-1)).header.timestamp;

			terminal.log("statistics for the last %i blocks\n", count - start);
			terminal.log("avg slot num:   %f\n", (float)slotSum / (float)slotSumCount);
			terminal.log("avg block time: %f\n", (float)(endTime - beginTime) / (float)statsCount);
			for (auto& i : slotCount) {
				terminal.log("slot %i occured %i times\n", i.first, i.second);
			}
			for (auto& i : validatorCount) {
				terminal.log("validator %s occured %i times\n", toHex(i.first).c_str(), i.second);
			}
		}
		else {
			terminal.log("invalid command\n");
		}
	}
	return 0;
}
