//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "KeyStore.h"
#include "util/hex.h"
#include "util/util.h"
#include "util/random.h"
#include "cryptography/secRandom.h"
#include "cryptography/pbkdf2.h"
#include "cryptography/aes.h"
#include "cryptography/sha.h"
#include "util/Serializer.h"
#include <fstream>
#include <filesystem>
#include <algorithm>

bool KeyStore::init(const std::string& file) {
	this->file = "";
	if (!std::filesystem::exists(file)) {
		return false;
	}
	this->file = file;

	loadFile();
	return true;
}

bool KeyStore::createFile(const std::string& file) {
	if (std::filesystem::exists(file)) {
		return false;
	}
	
	segments.clear();
	storeName = std::filesystem::path(file).stem().string();
	this->file = file;

	{
		Segment segment;
		segment.index = 0;
		segment.name = "public";
		segment.encrypted = false;
		segment.locked = false;
		segment.hasChange = true;
		segments.push_back(segment);
	}

	{
		Segment segment;
		segment.index = 1;
		segment.name = "private";
		segment.encrypted = true;
		segment.locked = false;
		segment.hasChange = true;
		segment.setPassword("");
		segments.push_back(segment);
	}

	return true;
}

bool KeyStore::hasChanges() {
	for (auto& seg : segments) {
		if (seg.hasChange) {
			return true;
		}
	}
	return false;
}

bool KeyStore::unlock(const std::string& password) {
	for (auto& seg : segments) {
		if (seg.encrypted && seg.locked) {
			if (!seg.unlock(password)) {
				return false;
			}
			if (!seg.load()) {
				return false;
			}
		}
	}
	return true;
}

bool KeyStore::isLocked() {
	for (auto& seg : segments) {
		if (seg.encrypted) {
			if (seg.locked) {
				return true;
			}
		}
	}
	return false;
}

void KeyStore::lock() {
	for (auto& seg : segments) {
		if (seg.encrypted) {
			seg.lock();
		}
	}
}

bool KeyStore::save() {
	for (auto& seg : segments) {
		if (seg.hasChange) {
			if (seg.save()) {
				seg.hasChange = false;
			}
		}
	}
	saveFile();
	return true;
}

bool KeyStore::setPassword(const std::string& newPassword) {
	for (auto& seg : segments) {
		if (seg.encrypted) {
			if (!seg.locked) {
				seg.setPassword(newPassword);
				seg.hasChange = true;
			}
		}
	}
	save();
	return true;
}

int KeyStore::getMasterKeyCount() {
	return getKeyCount(KeyType::MASTER_KEY);
}

int KeyStore::getPrivateKeyCount() {
	return getKeyCount(KeyType::PRIVATE_KEY);
}

int KeyStore::getPublicKeyCount() {
	return getKeyCount(KeyType::PUBLIC_KEY);
}

int KeyStore::getViewKeyCount() {
	return getKeyCount(KeyType::VIEW_KEY);
}

MasterKey KeyStore::getMasterKey(int index) {
	std::string data = getKey(KeyType::MASTER_KEY, index);
	Serializer serial(data);
	MasterKey key;
	serial.read(key);
	return key;
}

EccPrivateKey KeyStore::getPrivateKey(int index) {
	std::string data = getKey(KeyType::PRIVATE_KEY, index);
	Serializer serial(data);
	EccPrivateKey key;
	serial.read(key);
	return key;
}

EccPublicKey KeyStore::getPublicKey(int index) {
	std::string data = getKey(KeyType::PUBLIC_KEY, index);
	Serializer serial(data);
	EccPublicKey key;
	serial.read(key);
	return key;
}

EccPublicKey KeyStore::getViewKey(int index) {
	std::string data = getKey(KeyType::VIEW_KEY, index);
	Serializer serial(data);
	EccPublicKey key;
	serial.read(key);
	return key;
}

bool KeyStore::addMasterKey(const MasterKey& key) {
	Serializer serial;
	serial.write(key);
	return addKey("master", getSegment(true), serial.toString(), KeyType::MASTER_KEY);
}

bool KeyStore::addPrivateKey(const EccPrivateKey& key) {
	Serializer serial;
	serial.write(key);

	int id = 0;
	if (!addKey("private", getSegment(true), serial.toString(), KeyType::PRIVATE_KEY, -1, -1, &id)) {
		return false;
	}

	EccPublicKey pub = eccToPublicKey(key);
	Serializer serial2;
	serial2.write(pub);
	return addKey("public", getSegment(false), serial2.toString(), KeyType::PUBLIC_KEY, id);
}

bool KeyStore::addViewKey(const EccPublicKey& key) {
	Serializer serial;
	serial.write(key);
	return addKey("view", getSegment(false), serial.toString(), KeyType::VIEW_KEY);
}

bool KeyStore::generateMasterKey() {
	MasterKey key;
	secRandom((void*)&key, sizeof(key));
	return addMasterKey(key);
}

bool KeyStore::generatePrivateKey(int masterKeyIndex, int index) {
	Entry *entry = getEntry(KeyType::MASTER_KEY, masterKeyIndex);
	if (entry && !entry->data.empty()) {
		Serializer serial;
		serial.writeBytes((uint8_t*)entry->data.data(), entry->data.size());
		serial.write(index);
		EccPrivateKey key = sha256(serial.toString());

		Serializer serial2;
		serial2.write(key);

		int id = 0;
		if (!addKey("private", getSegment(true), serial2.toString(), KeyType::PRIVATE_KEY, entry->id, index, &id)) {
			return false;
		}

		EccPublicKey pub = eccToPublicKey(key);
		Serializer serial3;
		serial3.write(pub);
		return addKey("public", getSegment(false), serial3.toString(), KeyType::PUBLIC_KEY, id);
	}
	return false;
}

bool KeyStore::Segment::serial(std::string& content) {
	if (!locked) {
		content = "";
		for (auto& e : entries) {
			content += "beginEntry\n";
			content += "name:" + e.name + "\n";
			content += "type:" + std::to_string((int)e.type) + "\n";
			content += "id:" + std::to_string(e.id) + "\n";
			if (e.sourceId != -1) {
				content += "sourceId:" + std::to_string(e.sourceId) + "\n";
			}
			if (e.sourceIndex != -1) {
				content += "sourceIndex:" + std::to_string(e.sourceIndex) + "\n";
			}
			content += "data:" + stringToHex(e.data) + "\n";
			content += "endEntry\n";
		}
		return true;
	}
	return false;
}

bool KeyStore::Segment::deserial(const std::string& content) {
	entries.clear();

	bool inEntry = false;
	Entry entry;
	auto lines = split(content, "\n", false);
	for (auto& line : lines) {
		auto parts = split(line, ":", true);
		if (parts.size() > 0) {
			std::string key = parts[0];
			std::string value;
			if (parts.size() > 1) {
				value = parts[1];
			}

			if (!inEntry) {
				if (key == "beginEntry") {
					inEntry = true;
					entry = Entry();
				}
			}
			else {
				if (key == "endEntry") {
					entries.push_back(entry);
					entry = Entry();
					inEntry = false;
				}
				else if (key == "name") {
					entry.name = value;
				}
				else if (key == "type") {
					try {
						entry.type = (KeyType)std::stoi(value);
					}
					catch (...) {
						return false;
					}
				}
				else if (key == "id") {
					try {
						entry.id = std::stoi(value);
					}
					catch (...) {
						return false;
					}
				}
				else if (key == "sourceId") {
					try {
						entry.sourceId = std::stoi(value);
					}
					catch (...) {
						return false;
					}
				}
				else if (key == "sourceIndex") {
					try {
						entry.sourceIndex = std::stoi(value);
					}
					catch (...) {
						return false;
					}
				}
				else if (key == "data") {
					entry.data = stringfromHex(value);
				}
			}

		}
	}

	if (inEntry) {
		return false;
	}
	return true;
}

bool KeyStore::Segment::unlock(const std::string& password) {
	if (encrypted) {
		key = pbkdf2(password, salt, 1000000);
	}
	return true;
}

bool KeyStore::Segment::load() {
	if (encrypted) {
		std::string content;
		if (!aesDecrypt(data, content, key, iv, tag)) {
			return false;
		}
		if (deserial(content)) {
			locked = false;
			return true;
		}
		else {
			entries.clear();
		}
	}
	else {
		if (deserial(data)) {
			return true;
		}
		else {
			entries.clear();
		}
	}
	return false;
}

void KeyStore::Segment::lock() {
	if (encrypted) {
		key = 0;
		entries.clear();
		locked = true;
	}
}

bool KeyStore::Segment::save() {
	if (encrypted) {
		std::string content;
		if (serial(content)) {
			secRandom((void*)&iv, sizeof(iv));
			if (aesEncrypt(content, data, key, iv, tag)) {
				return true;
			}
		}
		return false;
	}
	else {
		return serial(data);
	}
}

bool KeyStore::Segment::setPassword(const std::string& password) {
	if (encrypted && !locked) {
		salt.resize(16);
		secRandom((void*)salt.data(), salt.size());
		key = pbkdf2(password, salt, 1000000);
		return true;
	}
	else {
		return false;
	}
}

KeyStore::Segment* KeyStore::getSegment(bool encrypted) {
	for (auto& seg : segments) {
		if (seg.encrypted == encrypted) {
			return &seg;
		}
	}
	return nullptr;
}

bool KeyStore::loadFile() {
	segments.clear();
	storeName = "";

	std::ifstream stream(file);
	if (stream.is_open()) {
		int nextSegmentIndex = 0;
		bool inSegment = false;
		bool inData = false;
		Segment seg;
		std::string line;
		while (std::getline(stream, line)) {
			line.erase(std::remove(line.begin(), line.end(), '\r' ), line.end());
			if (!line.empty()) {
				auto parts = split(line, ":", true);
				if (parts.size() > 0) {
					std::string key = parts[0];
					std::string value;
					if (parts.size() > 1) {
						value = parts[1];
					}

					if (!inSegment) {
						if (key == "storeName") {
							storeName = value;
						}
						else if (key == "beginSegment") {
							if (!inSegment) {
								seg = Segment();
								seg.index = nextSegmentIndex++;
								seg.locked = true;
								inSegment = true;
							}
							else {
								return false;
							}
						}
						else if (key == "endSegment") {
							return false;
						}
					}
					else {

						if (!inData) {
							if (key == "name") {
								seg.name = value;
							}
							else if (key == "encrypted") {
								try {
									seg.encrypted = (bool)std::stoi(value);
								}
								catch (...) {
									return false;
								}
							}
							else if (key == "salt") {
								seg.salt = stringfromHex(value);
							}
							else if (key == "iv") {
								seg.iv = fromHex<decltype(seg.iv)>(value);
							}
							else if (key == "tag") {
								seg.tag = fromHex<decltype(seg.tag)>(value);
							}
							else if (key == "beginData") {
								if (!inData) {
									inData = true;
								}
								else {
									return false;
								}
							}
							else if (key == "beginSegment") {
								return false;
							}
							else if (key == "endData") {
								return false;
							}
							else if (key == "endSegment") {
								if (inSegment && !inData) {
									segments.push_back(seg);
									inSegment = false;
									seg = Segment();
								}
								else {
									return false;
								}
							}
						}
						else {
							if (key == "endData") {
								if (inData) {
									inData = false;
								}
								else {
									return false;
								}
							}
							else if (key == "beginData") {
								return false;
							}
							else if (key == "beginSegment") {
								return false;
							}
							else {
								if (!seg.data.empty()) {
									seg.data += "\n";
								}
								seg.data += line;
							}
						}
					}

				}
			}
		}
		stream.close();

		if (inSegment || inData) {
			return false;
		}

		for (auto& seg : segments) {
			if (!seg.encrypted) {
				seg.locked = false;
				seg.load();
			}
			else {
				seg.data = stringfromHex(seg.data);
			}
		}

		return true;
	}
	else {
		return false;
	}
}

void KeyStore::saveFile() {
	std::string content = "";
	content += "storeName:" + storeName + "\n";
	for (auto& seg : segments) {
		content += "beginSegment\n";
		content += "name:" + seg.name + "\n";
		content += "encrypted:" + std::to_string(seg.encrypted) + "\n";
		if (seg.encrypted) {
			content += "comment:pbkdf2_hmac_sha256_1M_aes_256_gcm\n";
			content += "salt:" + stringToHex(seg.salt) + "\n";
			content += "iv:" + toHex(seg.iv) + "\n";
			content += "tag:" + toHex(seg.tag) + "\n";
			content += "beginData\n";
			content += stringToHex(seg.data) + "\n";
			content += "endData\n";
		}
		else {
			content += "beginData\n";
			content += seg.data + "\n";
			content += "endData\n";
		}
		content += "endSegment\n";
	}

	std::filesystem::create_directories(std::filesystem::path(file).parent_path());
	std::ofstream stream(file);
	stream.write(content.data(), content.size());
	stream.close();
}

bool KeyStore::addKey(const std::string& name, Segment* seg, const std::string& key, KeyType type, int sourceId, int sourceIndex, int* idOut) {
	if (seg) {
		Entry e;
		e.id = random<int>();
		if (e.id < 0) {
			e.id = -e.id;
		}
		e.name = name;
		e.data = key;
		e.sourceId = sourceId;
		e.sourceIndex = sourceIndex;
		e.type = type;
		seg->entries.push_back(e);
		seg->hasChange = true;
		if (idOut) {
			*idOut = e.id;
		}
		return true;
	}
	else {
		return false;
	}
}

int KeyStore::getKeyCount(KeyType type) {
	int count = 0;
	for (auto& seg : segments) {
		if (!seg.locked) {
			for (auto& e : seg.entries) {
				if (e.type == type) {
					count++;
				}
			}
		}
	}
	return count;
}

KeyStore::Entry * KeyStore::getEntry(KeyType type, int index) {
	int count = 0;
	for (auto& seg : segments) {
		if (!seg.locked) {
			for (auto& e : seg.entries) {
				if (e.type == type) {
					if (count == index) {
						return &e;
					}
					count++;
				}
			}
		}
	}
	return nullptr;
}

std::string KeyStore::getKey(KeyType type, int index) {
	Entry* entry = getEntry(type, index);
	if (entry) {
		return entry->data;
	}
	return "";
}
