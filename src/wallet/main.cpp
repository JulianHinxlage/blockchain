//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "wallet/Wallet.h"
#include "util/util.h"
#include "util/log.h"

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
int processCount(const std::string& name) {
	return 1;
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


	std::string line;
	while (std::getline(std::cin, line)) {
		auto args = split(line, " ", false);
		if (args.size() <= 0) {
			printf("invalid input\n");
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

			printf("block count:   %i\n", wallet.node.blockChain.getBlockCount());
			printf("chain tip:     %s\n", toHex(wallet.node.blockChain.getHeadBlock()).c_str());
			printf("address:       %s\n", toHex(address).c_str());
			printf("balance:       %s\n", amountToCoin(account.balance).c_str());
			if (account.stakeAmount != 0) {
				printf("stake:         %s\n", amountToCoin(account.stakeAmount).c_str());
				printf("stake block:   %llu\n", account.stakeBlockNumber);
				printf("validator num: %llu\n", account.validatorNumber);
				printf("stake owner:   %s\n", toHex(account.stakeOwner).c_str());
			}
			printf("transactions:  %u\n", account.transactionCount);
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
					printf("\n");
				}
				printf("block number:       %i\n", i);
				printf("block slot:         %i\n", block.header.slot);
				printf("transaction count:  %i\n", block.header.transactionCount);
				printf("total stake amount: %s\n", amountToCoin(block.header.totalStakeAmount).c_str());
				printf("block hash:         %s\n", toHex(hash).c_str());
				printf("block seed:         %s\n", toHex(block.header.seed).c_str());
				printf("block validator:    %s\n", toHex(block.header.validator).c_str());
				printf("block beneficiary:  %s\n", toHex(block.header.beneficiary).c_str());
				printf("account tree:       %s\n", toHex(block.header.accountTreeRoot).c_str());
				printf("validator tree:     %s\n", toHex(block.header.validatorTreeRoot).c_str());
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
						printf("\n");
						printf("tx number:      %i\n", (int)tx.header.transactionNumber);
						printf("block number:   %i\n", (int)block.header.blockNumber);
						printf("type:           %i\n", (int)tx.header.type);
						printf("amount:         %s\n", amountToCoin(tx.header.amount).c_str());
						if (isSender) {
							printf("-> to:      %s\n", toHex(tx.header.recipient).c_str());
						}
						if (isRecipient) {
							printf("<- from:    %s\n", toHex(tx.header.sender).c_str());
						}
					}
				}
			}
		}
		else if (cmd == "send") {
			if (args.size() < 2) {
				printf("usage: send <address> <amount> [fee] [type]");
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
						printf("invalid type");
						continue;
					}
				}

				wallet.sendTransaction(address, amount, fee, type);
			}
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
			wallet.node.blockChain.setHeadBlock(wallet.node.blockChain.getBlockHash(0));
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

			printf("statistics for the last %i blocks\n", count - start);
			printf("avg slot num:   %f\n", (float)slotSum / (float)slotSumCount);
			printf("avg block time: %f\n", (float)(endTime - beginTime) / (float)statsCount);
			for (auto& i : slotCount) {
				printf("slot %i occured %i times\n", i.first, i.second);
			}
			for (auto& i : validatorCount) {
				printf("validator %s occured %i times\n", toHex(i.first).c_str(), i.second);
			}
		}
		else {
			printf("invalid command\n");
		}
	}
	return 0;
}
