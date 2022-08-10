#include "Wallet.h"
#include "util/hex.h"
#include <fstream>
#include <filesystem>
#include <iostream>

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

std::string Wallet::selectFile(const std::string& directory) {
	std::vector<std::string> files;
	for (auto& file : std::filesystem::directory_iterator(directory)) {
		if (file.is_regular_file()) {
			printf("(%i) %s\n", (int)files.size(), file.path().filename().string().c_str());
			files.push_back(file.path().string());
		}
	}
	printf("(%i) new\n", (int)files.size());

	printf("select number> ");
	std::string line;
	if (std::getline(std::cin, line)) {
		if (line == "exit") {
			return "";
		}

		int num = -1;
		try {
			num = std::stoi(line);
		}
		catch (...) {
			printf("invalid number\n");
		}
		if (num != -1) {
			if (num >= 0 && num < files.size()) {
				return files[num];
				if (!init(files[num])) {
					createKey();
				}
			}
			else if (num == files.size()) {
				printf("file> ");
				if (std::getline(std::cin, line)) {
					line = std::string(directory) + "/" + line;
					return line;
				}
			}
		}
	}
	return "";
}

std::string Wallet::selectDirectory(const std::string& directory) {
	std::vector<std::string> dirs;
	std::filesystem::create_directories(directory);
	for (auto& dir : std::filesystem::directory_iterator(directory)) {
		if (dir.is_directory()) {
			printf("(%i) %s\n", (int)dirs.size(), dir.path().filename().string().c_str());
			dirs.push_back(dir.path().string());
		}
	}
	printf("(%i) new\n", (int)dirs.size());

	printf("select number> ");
	std::string line;
	if (std::getline(std::cin, line)) {
		if (line == "exit") {
			return "";
		}

		int num = -1;
		try {
			num = std::stoi(line);
		}
		catch (...) {
			printf("invalid number\n");
		}
		if (num != -1) {
			if (num >= 0 && num < dirs.size()) {
				return dirs[num];
			}
			else if (num == dirs.size()) {
				printf("directory> ");
				if (std::getline(std::cin, line)) {
					line = std::string(directory) + "/" + line;
					std::filesystem::create_directories(line);
					return line;
				}
			}
		}
	}
	return "";
}

