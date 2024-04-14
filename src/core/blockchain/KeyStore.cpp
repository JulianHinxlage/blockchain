//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "KeyStore.h"
#include "util/hex.h"
#include <fstream>
#include <filesystem>

void KeyStore::create(const std::string& file) {
	if (std::filesystem::exists(file)) {
		return;
	}

	eccGenerate(priv, pub);

	std::string content;
	content += toHex(pub) + "\n";
	content += toHex(priv) + "\n";

	std::filesystem::create_directories(std::filesystem::path(file).parent_path());
	std::ofstream stream(file);
	stream.write(content.data(), content.size());
}

void KeyStore::load(const std::string& file) {
	if (!std::filesystem::exists(file)) {
		return;
	}

	std::ifstream stream(file);
	std::string line;
	std::getline(stream, line);
	pub = fromHex<EccPublicKey>(line);
	std::getline(stream, line);
	priv = fromHex<EccPrivateKey>(line);
}

void KeyStore::loadOrCreate(const std::string& file) {
	if (std::filesystem::exists(file)) {
		load(file);
	}
	else {
		create(file);
	}
}

EccPublicKey KeyStore::getPublicKey() {
	return pub;
}

EccPrivateKey KeyStore::getPrivateKey() {
	return priv;
}
