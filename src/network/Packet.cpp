#include "Packet.h"

Packet::Packet() {
    bytes = 0;
    offset = 0;
}

Packet::Packet(const std::vector<char> &buffer, int bytes) {
    this->buffer = buffer;
    this->bytes = bytes;
    this->offset = 0;
}

std::string Packet::remaining() {
    return std::string(buffer.data() + offset, bytes - offset);
}

char *Packet::data() {
    return buffer.data() + offset;
}

int Packet::size() {
    return bytes - offset;
}

std::string Packet::getStr(){
    std::string str;
    while(offset < bytes){
        if(buffer[offset] == '\0'){
            offset++;
            break;
        }else{
            str.push_back(buffer[offset++]);
        }
    }
    return str;
}

void Packet::addStr(const std::string &str){
    for(int i = 0; i < str.size(); i++){
        if(buffer.size() > bytes){
            buffer[bytes++] = str[i];
        }else{
            buffer.push_back(str[i]);
            bytes++;
        }
    }
    if(buffer.size() > bytes){
        buffer[bytes++] = '\0';
    }else{
        buffer.push_back('\0');
        bytes++;
    }
}

void Packet::add(const char *ptr, int bytes) {
    for(int i = 0; i < bytes; i++){
        if(buffer.size() > this->bytes){
            buffer[this->bytes++] = ptr[i];
        }else{
            buffer.push_back(ptr[i]);
            this->bytes++;
        }
    }
}

void Packet::get(char* ptr, int bytes) {
    for (int i = 0; i < bytes; i++) {
        if (offset < this->bytes) {
            ptr[i] = buffer[offset++];
        }
        else {
            ptr[i] = 0;
        }
    }
}

void Packet::skip(int bytes) {
    offset = std::min(this->bytes, offset + bytes);
}

void Packet::unskip(int bytes) {
    offset = std::max(0, offset - bytes);
}