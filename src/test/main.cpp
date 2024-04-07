//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "blockchain/BlockChainNode.h"
#include "blockchain/BlockChain.h"
#include "blockchain/BlockCreator.h"
#include "blockchain/KeyStore.h"
#include <iostream>

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

int main(int argc, char* argv[]) {
	int num = processCount("test.exe");

	BlockChain chain;
	BlockChainNode node;
	KeyStore key;
	node.blockChain = &chain;

	printf("num: %i\n", num);
	key.createOrLoadKey("key_" + std::to_string(num) + ".txt");
	chain.init("chain_" + std::to_string(num));
	printf("chain loaded with %i block\n", chain.getBlockCount());

	node.init(NodeType::FULL_NODE);

	node.logCalback = [](const std::string& msg, bool error) {
		printf("%s\n", msg.c_str());
	};

	node.onBlockRecived = [&](const Block& block) {
		printf("block: %s num=%i\n", toHex(block.blockHash).c_str(), (int)block.header.blockNumber);
		chain.addBlock(block);
		if (block.header.blockNumber == chain.getBlockCount()) {
			if (chain.addBlockToTip(block.blockHash)) {
				printf("added to tip\n");
			}
			else {
				printf("got invalid block\n");
			}
		}
	};

	node.connect("127.0.0.1", 5400);
	node.synchronize();

	std::string line;
	while (std::getline(std::cin, line)) {
		if (line == "exit") {
			break;
		}
		if (line == "block") {
			BlockCreator creator;
			creator.blockChain = &chain;
			creator.beginBlock(key.getPublicKey());
			Block block = creator.endBlock();
			block.header.sign(key.getPrivateKey());
			block.blockHash = block.header.caclulateHash();
			chain.addBlock(block);
			chain.addBlockToTip(block.blockHash);
			node.sendBlock(block);
		}
	}
	return 0;
}
