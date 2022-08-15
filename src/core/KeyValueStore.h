#pragma once

#include "util/Buffer.h"
#include <string>
#include <unordered_map>
#include <fstream>

class KeyValueStore {
public:
	class Key {
	public:
		const void* ptr;
		int size;

		Key();
		Key(const void* key, int keySize);
		Key(const std::string& str);
		Key(Buffer& buffer);

		template<typename T>
		Key(const T& t) : Key(&t, sizeof(T)) {}


		void toString(std::string &str) const;
		void toBuffer(Buffer &buffer) const;

		std::string toString() const;
		Buffer toBuffer() const;

		template<typename T>
		T toValue() const {
			T t = T();
			int bytes = size < sizeof(t) ? size : sizeof(t);
			for (int i = 0; i < bytes; i++) {
				((uint8_t*)&t)[i] = ((uint8_t*)ptr)[i];
			}
			return t;
		}
	};
	typedef Key Value;

	KeyValueStore();
	void init(const std::string &directory, int version = 1, uint64_t maxFileSize = uint64_t(4) * 1024 * 1024 * 1024 - 1);
	bool set(const Key &key, const Key &value);
	Value get(const Key &key);
	bool remove(const Key& key);

private:
	class Index {
	public:
		class Entry {
		public:
			int fileId;
			int keySize;
			int valueSize;
			uint64_t offset;
		};

		std::unordered_map<std::string, Entry> entries;
		std::string file;
		std::ofstream stream;
		int version;

		void load(const std::string& file, int version);
		void add(const Key& key, const Entry& entry);
		Entry get(const Key &key);
	};

	std::string directory;
	int version;
	Index index;
	Buffer buffer;
	std::vector<std::shared_ptr<std::ifstream>> readStreams;
	std::ofstream writeStream;

	uint64_t maxFileSize;
	int currentWriteFileIndex;

	bool getPair(const Index::Entry &entry, Key &key, Value &value);
};
