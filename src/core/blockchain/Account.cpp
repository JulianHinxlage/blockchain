//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "Account.h"

std::string Account::serial() const {
    Serializer serial;
    serial.write(transactionCount);
    serial.write(balance);
    serial.write(stakeAmount);
    serial.write(stakeOwner);
    serial.write(validatorNumber);
    serial.write(stakeBlockNumber);
    serial.write(data);
    serial.write(code);
    return serial.toString();
}

int Account::deserial(const std::string& str) {
    Serializer serial(str);
    serial.read(transactionCount);
    serial.read(balance);
    serial.read(stakeAmount);
    serial.read(stakeOwner);
    serial.read(validatorNumber);
    serial.read(stakeBlockNumber);
    serial.read(data);
    serial.read(code);
    return serial.getReadIndex();
}
