//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "network/tcp/Server.h"
#include "network/peer/PeerNetwork.h"
#include <iostream>

void peerTest() {
	PeerNetwork net;
	uint16_t port = 54000;
	for (int i = 0; i < 10; i++) {
		net.addEntryNode("127.0.0.1", port + i);
	}

	net.messageCallback = [](const std::string &msg, PeerId source, bool broadcast) {
		printf("msg: %s\n", msg.c_str());
	};
	net.logCallback = [](const std::string& msg, int level) {
		printf("%s\n", msg.c_str());
	};

	net.connect(port);

	while (true) {
		std::string line;
		printf("> ");
		std::getline(std::cin, line);
		if (line == "exit") {
			break;
		}

		net.broadcast(line);

	}

	net.disconnect();

	std::string line;
	std::getline(std::cin, line);
}

int main(int argc, char* argv[]) {
	peerTest();
	return 0;
}