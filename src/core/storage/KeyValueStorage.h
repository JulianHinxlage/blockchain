//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <memory>
#include <vector>

class KeyValueStorage {
public:
	bool init(const std::string& directory, int keySize = 0, int valueSize = 0);
	bool has(const std::string &key);
	std::string get(const std::string &key);
	void set(const std::string &key, const std::string &value);
	void remove(const std::string &key);

	template<typename T>
	bool has(const T& key) {
		return has(std::string((char*)&key, sizeof(key)));
	}

	template<typename T>
	std::string get(const T& key) {
		return get(std::string((char*)&key, sizeof(key)));
	}
	
	template<typename T>
	void set(const T& key, const std::string& value) {
		return set(std::string((char*)&key, sizeof(key)), value);
	}
	
	template<typename T>
	void remove(const T& key) {
		return remove(std::string((char*)&key, sizeof(key)));
	}

private:
	class Index {
	public:
		class Header {
		public:
			int version = 1;
			int keySize = 0;
			int valueSize = 0;
		};

		class Entry {
		public:
			int fileId = -1;
			int offset = 0;
		};
		
		std::unordered_map<std::string, Entry> entries;
		std::string file;
		std::ofstream stream;
		Header header;

		bool load();
		void set(const std::string &key, Entry entry);
		bool has(const std::string& key);
		Entry get(const std::string& key);
		void remove(const std::string& key);
	};


	Index index;
	std::string directory;
	std::unordered_map<std::string, std::string> cache;
	std::vector<std::shared_ptr<std::ifstream>> streams;
	std::ofstream writeStream;
	int writeStreamId = 0;
	int maxFileSize = 0;
};
