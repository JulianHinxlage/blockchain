#pragma once

#include "BlockChain.h"
#include "DataBase.h"
#include "BlockCreator.h"

class Node {
public:
	DataBase db;
	BlockChain chain;
	BlockCreateor validator;
	BlockCreateor creator;

	Node();
	bool init(const std::string &dataDirectory, bool verbose);
	bool validateChain(bool reduceChainToValid, int verbose);
};
