//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "Wallet.h"
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
			wallet.node.blockChain.accountTree.reset(wallet.node.blockChain.getBlockHeader(wallet.node.blockChain.getLatestBlock()).accountTreeRoot);
			EccPublicKey address = wallet.keyStore.getPublicKey();
			Account account = wallet.node.blockChain.accountTree.get(address);

			printf("block count:  %i\n", wallet.node.blockChain.getBlockCount());
			printf("chain tip:    %s\n", toHex(wallet.node.blockChain.getLatestBlock()).c_str());
			printf("address:      %s\n", toHex(address).c_str());
			printf("balance:      %s\n", amountToCoin(account.balance).c_str());
			printf("transactions: %u\n", account.transactionCount);
		}
		else if (cmd == "blocks") {
			int count = wallet.node.blockChain.getBlockCount();
			for (int i = std::max(0, count - 5); i < count; i++) {
				Hash hash = wallet.node.blockChain.getBlockHash(i);
				Block block = wallet.node.blockChain.getBlock(hash);

				if (i != 0) {
					printf("\n");
				}
				printf("block number:      %i\n", i);
				printf("block hash:        %s\n", toHex(hash).c_str());
				printf("account tree:      %s\n", toHex(block.header.accountTreeRoot).c_str());
				printf("transaction count: %i\n", block.header.transactionCount);
			}
		}
		else if (cmd == "send") {
			if (args.size() < 2) {
				printf("usage: send <address> <amount> [fee]");
			}
			else {
				std::string address = args[0];
				std::string amount = args[1];
				std::string fee = "0";
				if (args.size() > 2) {
					fee = args[2];
				}

				wallet.sendTransaction(address, amount, fee);
			}
		}
		else if (cmd == "test") {
			wallet.sendTransaction(toHex(wallet.keyStore.getPublicKey()), "0", "0");
		}
		else if (cmd == "sync") {
			wallet.node.blockChain.accountTree.reset(wallet.node.blockChain.getBlockHeader(wallet.node.blockChain.getLatestBlock()).accountTreeRoot);
			wallet.node.synchronize();
		}
		else if (cmd == "verify") {
			wallet.node.verifyChain();
		}
		else if (cmd == "reset") {
			wallet.node.blockChain.resetTip(wallet.node.blockChain.getBlockHash(0));
		}
		else {
			printf("invalid command\n");
		}
	}
	return 0;
}
