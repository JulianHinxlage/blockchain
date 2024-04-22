//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "storage/KeyStore.h"
#include "util/hex.h"
#include <iostream>

std::string passwordInput() {
	std::string input;
	printf("password: ");
	if (std::getline(std::cin, input)) {
		return input;
	}
	else {
		return "";
	}
}

void print(KeyStore2& store) {
	int count = store.getPublicKeyCount();
	printf("pub count: %i\n", count);
	for (int i = 0; i < count; i++) {
		EccPublicKey pub = store.getPublicKey(i);
		printf("pub:  %s\n", toHex(pub).c_str());
	}

	count = store.getPrivateKeyCount();
	printf("priv count: %i\n", count);
	for (int i = 0; i < count; i++) {
		EccPrivateKey priv = store.getPrivateKey(i);
		EccPublicKey pub = eccToPublicKey(priv);
		printf("pub:  %s\n", toHex(pub).c_str());
		printf("priv: %s\n", toHex(priv).c_str());
	}
}

int main(int argc, char* argv[]) {
	KeyStore2 store;
	if (!store.init("test.key")) {
		store.createFile("test.key");
		printf("create new key store\n");
		store.setPassword(passwordInput());
		store.generateMasterKey();
		store.generatePrivateKey(0, 0);
		store.generatePrivateKey(0, 1);
		store.generatePrivateKey(0, 2);
		store.save();
	}
	else {
		printf("loaded key store\n");

		print(store);

		for (int i = 0; i < 5; i++) {
			if (store.unlock(passwordInput())) {
				break;
			}
			else {
				printf("invalid\n");
			}
		}

		if (!store.isLocked()) {
			print(store);
		}
	}
}
