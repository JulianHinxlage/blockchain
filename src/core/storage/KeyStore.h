//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "cryptography/ecc.h"
#include "util/Blob.h"
#include <string>
#include <vector>

typedef Blob<256> MasterKey;

class KeyStore {
public:
	bool init(const std::string& file);
	bool createFile(const std::string& file);
	bool hasChanges();
	bool unlock(const std::string& password);
	bool isLocked();
	void lock();
	bool save();
	bool setPassword(const std::string& newPassword);

	int getMasterKeyCount();
	int getPrivateKeyCount();
	int getPublicKeyCount();
	int getViewKeyCount();
	
	MasterKey getMasterKey(int index);
	EccPrivateKey getPrivateKey(int index = 0);
	EccPublicKey getPublicKey(int index = 0);
	EccPublicKey getViewKey(int index);

	bool addMasterKey(const MasterKey& key);
	bool addPrivateKey(const EccPrivateKey &key);
	bool addViewKey(const EccPublicKey& key);
	
	bool generateMasterKey();
	bool generatePrivateKey(int masterKeyIndex, int index);

private:
	std::string file;

	enum class KeyType {
		NONE,
		MASTER_KEY,
		PRIVATE_KEY,
		PUBLIC_KEY,
		VIEW_KEY,
	};
	class Entry {
	public:
		std::string name;
		KeyType type = KeyType::NONE;
		int id = -1;
		int sourceId = -1;
		int sourceIndex = -1;
		std::string data;
	};
	class Segment {
	public:
		std::string name;
		bool encrypted = false;
		std::string data;
		Blob<256> iv;
		Blob<128> tag;
		std::string salt;
		Blob<256> key;

		int index = 0;
		bool locked = true;
		bool hasChange = false;
		std::vector<Entry> entries;

		bool serial(std::string &content);
		bool deserial(const std::string& content);

		bool load();
		bool save();

		bool unlock(const std::string& password);
		void lock();
		bool setPassword(const std::string& password);
	};
	std::vector<Segment> segments;
	std::string storeName;

	Segment* getSegment(bool encrypted);
	bool loadFile();
	void saveFile();

	bool addKey(const std::string &name, Segment *seg, const std::string& key, KeyType type, int sourceId = -1, int sourceIndex = -1, int *idOut = nullptr);
	int getKeyCount(KeyType type);
	Entry *getEntry(KeyType type, int index);
	std::string getKey(KeyType type, int index);
};
