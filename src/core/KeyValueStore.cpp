#include "KeyValueStore.h"
#include <filesystem>

KeyValueStore::Key::Key() {
	ptr = nullptr;
	size = 0;
}

KeyValueStore::Key::Key(const void* key, int keySize) {
	ptr = key;
	size = keySize;
}

KeyValueStore::Key::Key(const std::string& str)
	: Key(str.data(), str.size()) {}

KeyValueStore::Key::Key(Buffer& buffer)
	: Key(buffer.data(), buffer.size()) {}

void KeyValueStore::Key::toString(std::string& str) const {
	str = std::string((char*)ptr, size);
}

void KeyValueStore::Key::toBuffer(Buffer& buffer) const {
	buffer.add(ptr, size);
}

std::string KeyValueStore::Key::toString() const {
	return std::string((char*)ptr, size);
}

Buffer KeyValueStore::Key::toBuffer() const {
	Buffer buffer;
	toBuffer(buffer);
	return buffer;
}

KeyValueStore::KeyValueStore() {
	directory = "";
}

void KeyValueStore::init(const std::string& directory, int version, uint64_t maxFileSize) {
	this->directory = directory;
	this->version = version;
	this->maxFileSize = maxFileSize;
	std::filesystem::create_directories(directory);
	index.load(directory + "/" + "index.dat", version);
	
	std::string lastFile = "";
	for (int i = 0; i < 100; i++) {
		std::string file = directory + "/" + "data" + std::to_string(i) + ".dat";
		if (std::filesystem::exists(file)) {
			lastFile = file;
			std::shared_ptr<std::ifstream> readStream = std::make_shared<std::ifstream>();
			readStream->open(file, std::ios_base::binary);
			readStreams.push_back(readStream);
			currentWriteFileIndex = i;
		}
		else {
			if (i == 0) {
				lastFile = file;
				currentWriteFileIndex = 0;

				std::shared_ptr<std::ifstream> readStream = std::make_shared<std::ifstream>();
				readStream->open(file, std::ios_base::binary);
				readStreams.push_back(readStream);
			}
			break;
		}
	}
	if (std::filesystem::exists(lastFile)) {
		writeStreamFileSize = std::filesystem::file_size(lastFile);
	}
	else {
		writeStreamFileSize = 0;
	}
	writeStream.open(lastFile, std::ios_base::app | std::ios_base::binary);
}

bool KeyValueStore::set(const Key& key, const Key& value) {
	Index::Entry entry = index.get(key);
	if (entry.fileId == -1) {
		Index::Entry entry;
		entry.fileId = currentWriteFileIndex;
		entry.keySize = key.size;
		entry.valueSize = value.size;
		entry.offset = writeStreamFileSize;

		if (entry.offset + value.size > maxFileSize) {
			currentWriteFileIndex++;
			std::string file = directory + "/" + "data" + std::to_string(currentWriteFileIndex) + ".dat";
			writeStream.close();
			if (std::filesystem::exists(file)) {
				writeStreamFileSize = std::filesystem::file_size(file);
			}
			else {
				writeStreamFileSize = 0;
			}
			writeStream.open(file, std::ios_base::app | std::ios_base::binary);
			writeStreamFileSize = writeStream.tellp();
			entry.fileId = currentWriteFileIndex;
			entry.offset = writeStreamFileSize;
		}

		writeStream.write((char*)key.ptr, key.size);
		writeStream.write((char*)value.ptr, value.size);
		index.add(key, entry);
		writeStream.flush();
		writeStreamFileSize += key.size + value.size;
		return true;
	}
	else {
		return false;
	}
}

bool KeyValueStore::getPair(const Index::Entry& entry, Key& key, Value& value) {
	if (entry.fileId >= 0 && entry.fileId < readStreams.size()) {
		auto* readStream = readStreams[entry.fileId].get();
		readStream->seekg(entry.offset);
		buffer.resize(entry.keySize + entry.valueSize, 0);
		readStream->read((char*)buffer.data(), buffer.size());
		key = Key(buffer.data(), entry.keySize);
		value = Value(buffer.data() + entry.keySize, entry.valueSize);
		return true;
	}
	else {
		return false;
	}
}

KeyValueStore::Value KeyValueStore::get(const Key& key) {
	Index::Entry entry = index.get(key);
	Key pairKey;
	Value pairValue;
	if (getPair(entry, pairKey, pairValue)) {
		if (pairKey.toString() == key.toString()) {
			return pairValue;
		}
	}
	return Value();
}

bool KeyValueStore::remove(const Key& key) {
	return false;
}

void KeyValueStore::Index::load(const std::string& file, int version) {
	this->file = file;
	this->version = version;
	Buffer buffer;
	entries.clear();
	std::ifstream istream;
	istream.open(file, std::ios_base::binary);
	if (istream.is_open()) {

		int fileVersion = 0;
		istream.read((char*)&fileVersion, sizeof(fileVersion));
		if (fileVersion != version) {
			version = fileVersion;
		}

		while (!istream.eof()) {
			Entry entry = {-1, 0, 0, 0};
			int size = 0;
			istream.read((char*)&size, sizeof(size));
			if (size != 0) {
				buffer.resize(size, 0);
				istream.read((char*)buffer.data(), size);
				istream.read((char*)&entry, sizeof(entry));
				entries[buffer.toString()] = entry;
			}
		}
		istream.close();
	}

	if (!std::filesystem::exists(file)) {
		stream.open(file, std::ios_base::app | std::ios_base::binary);
		stream.write((char*)&this->version, sizeof(this->version));
		stream.flush();
	}
	else {
		stream.open(file, std::ios_base::app | std::ios_base::binary);
	}
}

void KeyValueStore::Index::add(const Key& key, const Entry& entry) {
	entries[key.toString()] = entry;
	stream.write((char*)&key.size, sizeof(key.size));
	stream.write((char*)key.ptr, key.size);
	stream.write((char*)&entry, sizeof(entry));
	stream.flush();
}

KeyValueStore::Index::Entry KeyValueStore::Index::get(const Key& key) {
	auto entry = entries.find(key.toString());
	if (entry == entries.end()) {
		return { -1, 0, 0, 0 };
	}
	else {
		return entry->second;
	}
}
