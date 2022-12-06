#pragma once

#include "KeyValueStore.h"

class CachedKeyValueStore {
public:
	KeyValueStore store;
	std::unordered_map<std::string, Buffer> cache;
	Buffer buffer;

	void init(const std::string& directory, int version = 1, uint64_t maxFileSize = uint64_t(4) * 1024 * 1024 * 1024 - 1) {
		store.init(directory, version, maxFileSize);
	}

	bool set(const KeyValueStore::Key& key, KeyValueStore::Value value) {
		cache[key.toString()] = value.toBuffer();
		return store.set(key, value);
	}

	KeyValueStore::Value get(const KeyValueStore::Key& key) {
		auto x = cache.find(key.toString());
		if (x == cache.end()) {
			buffer = store.get(key).toBuffer();
			cache[key.toString()] = buffer;
			return buffer;
		}
		else {
			return x->second;
		}
	}

	bool remove(const KeyValueStore::Key& key) {
		cache.erase(key.toString());
		return store.remove(key);
	}

};
