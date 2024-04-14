
#include "blockchain/Node.h"
#include "blockchain/KeyStore.h"

class Validator {
public:
	Node node;
	KeyStore keyStore;
	std::map<Hash, Transaction> pendingTransactions;
	std::thread* thread;
	std::mutex mutex;
	std::condition_variable cv;
	std::atomic_bool running;

	~Validator();
	void init(const std::string& chainDir, const std::string& keyFile, const std::string& entryNodeFile);
	void createBlock();
};
