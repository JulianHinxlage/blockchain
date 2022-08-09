#pragma once

#include <vector>
#include <string>

class Packet {
public:
    std::vector<char> buffer;
    int bytes;
    int offset;

    Packet();
    Packet(const std::vector<char> &buffer, int bytes);
    std::string remaining();
    char *data();
    int size();
    std::string getStr();
    void addStr(const std::string &str);
    void add(const char* ptr, int bytes);
    void get(char *ptr, int bytes);
    void skip(int bytes);
    void unskip(int bytes);

    template<typename T>
    T get(){
        T t;
        get((char*)&t, sizeof(t));
        return t;
    }

    template<typename T>
    void add(const T &t){
        add((char*)&t, sizeof(t));
    }

};
