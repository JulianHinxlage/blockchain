//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "Transaction.h"
#include "util/Serializer.h"
#include "cryptography/sha.h"
#include "cryptography/ecc.h"

Hash TransactionHeader::caclulateHash() const {
    return sha256(serial());
}

void TransactionHeader::sign(const EccPrivateKey& privateKey) {
    Serializer serial;
    serial.write(version);
    serial.write(type);
    serial.write(transactionNumber);
    serial.write(timestamp);
    serial.write(sender);
    serial.write(recipient);
    serial.write(amount);
    serial.write(fee);
    serial.write(dataHash);
    std::string data = serial.toString();
    signature = eccCreateSignature(data.data(), data.size(), privateKey);
}

bool TransactionHeader::verifySignature() const {
    Serializer serial;
    serial.write(version);
    serial.write(type);
    serial.write(transactionNumber);
    serial.write(timestamp);
    serial.write(sender);
    serial.write(recipient);
    serial.write(amount);
    serial.write(fee);
    serial.write(dataHash);
    std::string data = serial.toString();
    return eccVerifySignature(data.data(), data.size(), sender, signature);
}

std::string TransactionHeader::serial() const {
    Serializer serial;
    serial.write(version);
    serial.write(type);
    serial.write(transactionNumber);
    serial.write(timestamp);
    serial.write(sender);
    serial.write(recipient);
    serial.write(amount);
    serial.write(fee);
    serial.write(dataHash);
    serial.write(signature);
    return serial.toString();
}

int TransactionHeader::deserial(const std::string& str) {
    Serializer serial(str);
    serial.read(version);
    serial.read(type);
    serial.read(transactionNumber);
    serial.read(timestamp);
    serial.read(sender);
    serial.read(recipient);
    serial.read(amount);
    serial.read(fee);
    serial.read(dataHash);
    serial.read(signature);
    return serial.getReadIndex();
}

std::string Transaction::serial() const {
    Serializer serial(header.serial());
    serial.write((int)data.size());
    serial.writeBytes(data.data(), data.size());
    return serial.toString();
}

int Transaction::deserial(const std::string& str) {
    Serializer serial(str);
    serial.skip(header.deserial(str));
    int size = serial.read<int>();
    data.resize(size);
    serial.readBytes(data.data(), data.size());
    return serial.getReadIndex();
}
