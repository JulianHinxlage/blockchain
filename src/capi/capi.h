//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//


extern "C" {

	void init(const char* chainDir, const char* keyFile, const char* entryNodeFile);

	const char* getBalance(const char* address);
	const char* getPendingBalance(const char* address);

	const char* getTransactionAmount(const char* transactionHash);
	const char* getTransactionTime(const char* transactionHash);
	const char* getTransactionSender(const char* transactionHash);
	const char* getTransactionRecipient(const char* transactionHash);

	const char* getTransactions(const char* address);
	const char* getPendingTransactions();
	const char* getPendingTransactionsForAddress(const char* address);

	const char* createTransaction(const char* sender, const char* recipient, const char* amount, const char* fee, const char* type);
	const char* sendTransaction(const char* data);

}