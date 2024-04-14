
#include "blockchain/Node.h"
#include "blockchain/KeyStore.h"

class Wallet {
public:
	Node node;
	KeyStore keyStore;

	void init(const std::string &chainDir, const std::string& keyFile, const std::string& entryNodeFile);
	void sendTransaction(const std::string& address, const std::string& amount, const std::string& fee);
};
