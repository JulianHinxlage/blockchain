//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "blockchain/BinaryTree.h"
#include "util/hex.h"

int main(int argc, char* argv[]) {
	BinaryTree<int, int> tree;
	tree.init("tree");

	for (int i = 0; i < 100; i++) {
		tree.set(i, i);
	}
	printf("hash: %s\n", toHex(tree.getRoot()).c_str());
	return 0;
}
