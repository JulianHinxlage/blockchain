//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "KeyStore.h"
#include "util/hex.h"
#include <fstream>
#include <filesystem>

void KeyStore::createKey(const std::string& file) {
	if (std::filesystem::exists(file)) {
		return;
	}

	eccGenerate(priv, pub);

	std::string content;
	content += toHex(pub) + "\n";
	content += toHex(priv) + "\n";

	std::ofstream stream(file);
	stream.write(content.data(), content.size());
}

void KeyStore::loadKey(const std::string& file) {
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

void KeyStore::createOrLoadKey(const std::string& file) {
	if (std::filesystem::exists(file)) {
		loadKey(file);
	}
	else {
		createKey(file);
	}
}

EccPublicKey KeyStore::getPublicKey() {
	return pub;
}

EccPrivateKey KeyStore::getPrivateKey() {
	return priv;
}
