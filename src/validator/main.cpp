//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "validator/Validator.h"
#include "util/log.h"
#include "util/util.h"
#include "util/Terminal.h"

#include <iostream>
#include <filesystem>

#if WIN32
#include <Windows.h>
#include <Psapi.h>
#include <Winternl.h>

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


	Validator validator;
	validator.init(chainDir, keyFile, search("entry.txt"));


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
			EccPublicKey address = validator.keyStore.getPublicKey();
			Account account = validator.node.blockChain.getAccountTree().get(address);

			terminal.log("block count:  %i\n", validator.node.blockChain.getBlockCount());
			terminal.log("chain tip:    %s\n", toHex(validator.node.blockChain.getHeadBlock()).c_str());
			terminal.log("address:      %s\n", toHex(address).c_str());
			terminal.log("balance:      %s\n", amountToCoin(account.balance).c_str());
			if (account.stakeAmount != 0) {
				terminal.log("stake:        %s\n", amountToCoin(account.stakeAmount).c_str());
				terminal.log("stake block:  %llu\n", account.stakeBlockNumber);
				terminal.log("validator num:%llu\n", account.validatorNumber);
				terminal.log("stake owner:  %s\n", toHex(account.stakeOwner).c_str());
			}
			terminal.log("transactions: %u\n", account.transactionCount);
		}
		else {
			terminal.log("invalid command\n");
		}
	}
	return 0;
}
