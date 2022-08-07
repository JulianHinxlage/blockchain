#include "Wallet.h"
#include "util/hex.h"
#include <fstream>
#include <filesystem>

Wallet::Wallet() {
	file = "";
	privateKey = 0;
	publicKey = 0;
}

bool Wallet::init(const std::string& file) {
	this->file = file;
	bool valid = false;

	if (std::filesystem::exists(file)) {
		std::ifstream stream;
		stream.open(file);

		std::string line1;
		std::getline(stream, line1);
		std::string line2;
		std::getline(stream, line2);

		fromHex(line1, privateKey);
		fromHex(line2, publicKey);

		if (eccValidPublicKey(publicKey)) {
			if (eccToPublicKey(privateKey) == publicKey) {
				valid = true;
			}
		}

		if (!valid) {
			privateKey = 0;
			publicKey = 0;
		}
		
		stream.close();
	}
	return valid;
}

void Wallet::createKey() {
	if (!std::filesystem::exists(file)) {
		eccGenerate(privateKey, publicKey);
		std::ofstream stream;
		stream.open(file);

		std::string line1;
		std::string line2;

		toHex(privateKey, line1);
		toHex(publicKey, line2);

		stream.write(line1.c_str(), line1.size());
		stream.write("\n", 1);
		stream.write(line2.c_str(), line2.size());
		stream.write("\n", 1);

		stream.close();
	}
}
