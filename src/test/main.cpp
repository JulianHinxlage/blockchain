
#include "network/Connection.h"
#include "network/Listener.h"
#include "network/PeerNetwork.h"
#include "util/hex.h"
#include <iostream>

int main(int argc, char* args[]) {
	PeerNetwork network;
	for (int i = 0; i < 32; i++) {
		network.addEntryNode(Endpoint("127.0.0.1", 6000 + i));
	}

	network.msgCallback = [](const std::string &msg, PeerId id) {
		std::string hex;
		toHex(id, hex);;
		hex.resize(4);
		printf("%s: %s\n", hex.c_str(), msg.c_str());
	};
	network.logCallback = [](const std::string& msg, int level) {
		if (level >= 2) {
			printf("%s\n", msg.c_str());
		}
	};
	
	if (!network.start(6000, "127.0.0.1", 32)) {
		printf("listen failed\n");
	}
	printf("port: %i\n", network.localPeer().ep.port);

	std::string hex;
	toHex(network.localPeer().id, hex);;
	hex.resize(4);
	printf("id:   %s\n", hex.c_str());

	if (!network.connect()) {
		printf("failed to connect to network\n");
	}
	else {
		printf("connected to network\n");
	}

	std::string line;
	while (true) {
		if (std::getline(std::cin, line)) {
			if (line == "exit") {
				break;
			}
			else {
				network.broadcast(line);
			}
		}
	}

	network.disconnect();
	network.stop();
	return 0;
}