//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "Validator.h"
#include "util/log.h"

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


	std::string line;
	while (std::getline(std::cin, line)) {
		if (line == "exit") {
			break;
		}
		else if (line == "info") {
			printf("block count: %i\n", validator.node.blockChain.getBlockCount());
			printf("chain tip:   %s\n", toHex(validator.node.blockChain.getLatestBlock()).c_str());
		}
		else if (line == "block") {
			validator.createBlock();
		}
	}
	return 0;
}
