//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "KeyValueStorage.h"
#include <filesystem>

std::string readStr(std::ifstream& stream) {
	int size = -1;
	stream.read((char*)&size, sizeof(size));
	std::string str;
	if (size > 0) {
		str.resize(size);
		stream.read(str.data(), str.size());
	}
	return str;
}

template<typename T>
T read(std::ifstream& stream) {
	T t;
	stream.read((char*)&t, sizeof(t));
	return t;
}

void writeStr(std::ofstream& stream, const std::string &str) {
	int size = str.size();
	stream.write((char*)&size, sizeof(size));
	stream.write(str.data(), str.size());
}

template<typename T>
void write(std::ofstream& stream, const T &t) {
	stream.write((char*)&t, sizeof(t));
}

bool KeyValueStorage::init(const std::string& directory, int keySize, int valueSize) {
	this->directory = directory;
	maxFileSize = 1024 * 1024 * 1024;
	std::filesystem::create_directories(directory);
	index.file = directory + "/index.dat";
	index.header.keySize = keySize;
	index.header.valueSize = valueSize;
	if (!index.load()) {
		return false;
	}

	for (int i = 0; i < 100; i++) {
		std::string file = directory + "/data" + std::to_string(i) + ".dat";
		if (std::filesystem::exists(file)) {
			auto stream = std::make_shared<std::ifstream>();
			stream->open(file, std::ios::binary);
			streams.push_back(stream);
		}
		else {
			break;
		}
	}
	
	if (streams.size() == 0) {
		std::string file = directory + "/data0.dat";
		writeStream.open(file, std::ios::binary | std::ios::app);
		writeStream.close();

		auto stream = std::make_shared<std::ifstream>();
		stream->open(file, std::ios::binary);
		streams.push_back(stream);
	}

	writeStreamId = streams.size() - 1;
	std::string file = directory + "/data" + std::to_string(writeStreamId) + ".dat";
	writeStream.open(file, std::ios::binary | std::ios::app);
	writeStream.seekp(0, std::ios::end);
	return true;
}

bool KeyValueStorage::has(const std::string& key) {
	if (cache.find(key) != cache.end()) {
		return true;
	}
	return index.has(key);
}

std::string KeyValueStorage::get(const std::string& key) {
	auto i = cache.find(key);
	if (i != cache.end()) {
		return i->second;
	}

	if (streams.size() == 0) {
		for (int i = 0; i < 100; i++) {
			std::string file = directory + "/data" + std::to_string(i) + ".dat";
			if (std::filesystem::exists(file)) {
				auto stream = std::make_shared<std::ifstream>();
				stream->open(file, std::ios::binary);
				streams.push_back(stream);
			}
			else {
				break;
			}
		}
	}

	Index::Entry entry = index.get(key);
	if (streams.size() > entry.fileId) {
		auto &stream = *streams[entry.fileId].get();
		stream.seekg(entry.offset);
		std::string value = readStr(stream);
		cache[key] = value;
		return value;
	}
	else {
		return "";
	}
}

void KeyValueStorage::set(const std::string& key, const std::string& value) {
	Index::Entry entry;
	entry.offset = writeStream.tellp();
	while (entry.offset == -1) {
		writeStream.close();
		std::string file = directory + "/data" + std::to_string(writeStreamId) + ".dat";
		writeStream.open(file, std::ios::binary | std::ios::app);
		writeStream.seekp(0, std::ios::end);
		entry.offset = writeStream.tellp();
	}

	if (entry.offset + 4 + value.size() > maxFileSize) {
		writeStream.close();
		writeStreamId++;
		std::string file = directory + "/data" + std::to_string(writeStreamId) + ".dat";

		writeStream.open(file, std::ios::binary | std::ios::app);
		writeStream.close();

		auto stream = std::make_shared<std::ifstream>();
		stream->open(file, std::ios::binary);
		streams.push_back(stream);

		writeStream.open(file, std::ios::binary | std::ios::app);
		writeStream.seekp(0, std::ios::end);
		entry.offset = writeStream.tellp();
	}

	entry.fileId = writeStreamId;
	writeStr(writeStream, value);
	writeStream.flush();
	index.set(key, entry);
	cache[key] = value;
}

void KeyValueStorage::remove(const std::string& key) {
	cache.erase(key);
	index.remove(key);
}

bool KeyValueStorage::Index::load() {
	std::ifstream in(file, std::ios::binary);
	if (in.is_open()) {
		Header tmp = header;
		read<Header>(in);

		if (header.version != tmp.version) {
			return false;
		}
		if (header.keySize != tmp.keySize) {
			return false;
		}
		if (header.valueSize != tmp.valueSize) {
			return false;
		}

		while (!in.eof()) {
			std::string key = readStr(in);
			if (!key.empty()) {
				Entry entry = read<Entry>(in);
				if (entry.offset == -1 && entry.fileId == -1) {
					entries.erase(key);
				}
				else {
					entries[key] = entry;
				}
			}
		}

		in.close();
		stream.open(file, std::ios::binary | std::ios::app);
		stream.seekp(sizeof(header));
	}
	else {
		stream.open(file, std::ios::binary | std::ios::app);
		write(stream, header);
	}
}

void KeyValueStorage::Index::set(const std::string& key, Entry entry) {
	entries[key] = entry;
	writeStr(stream, key);
	write(stream, entry);
	stream.flush();
}

bool KeyValueStorage::Index::has(const std::string& key) {
	return entries.find(key) != entries.end();
}

KeyValueStorage::Index::Entry KeyValueStorage::Index::get(const std::string& key) {
	return entries.find(key)->second;
}

void KeyValueStorage::Index::remove(const std::string& key) {
	entries.erase(key);
	Entry entry;
	entry.fileId = -1;
	entry.offset = -1;
	writeStr(stream, key);
	write(stream, entry);
	stream.flush();
}
