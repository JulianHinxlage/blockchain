#pragma once

#include "Connection.h"
#include <vector>

class Listener {
public:
    std::shared_ptr<tcp::acceptor> acceptor;
    bool running = false;

    ~Listener();
    bool listen(uint16_t port);
    bool accept(std::function<void(Connection*)> callback);
    bool accept(std::function<void(std::shared_ptr<Connection>)> callback);
    void stop();
};