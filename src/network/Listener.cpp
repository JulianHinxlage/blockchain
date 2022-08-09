#include "Listener.h"

Listener::~Listener() {
    stop();
}

bool Listener::listen(uint16_t port) {
    acceptor = std::make_shared<tcp::acceptor>(SocketContext::ioService);
    tcp::endpoint ep(tcp::v4(), port);
    acceptor->open(ep.protocol());
    boost::system::error_code error;
    acceptor->bind(ep, error);
    if (error) {
        return false;
    }
    acceptor->listen();
    return true;
}

bool Listener::accept(std::function<void(std::shared_ptr<Connection>)> callback) {
    running = true;
    while (running) {
        std::shared_ptr<Connection> connection = std::make_shared<Connection>();
        boost::system::error_code error;
        acceptor->accept(connection->socket, error);
        if (!running) {
            break;
        }
        if (error) {
            running = false;
            return false;
        }
        connection->connected = true;
        connection->thread = std::make_shared<std::thread>([callback, connection]() { callback(connection); });
    }
    return true;
}

bool Listener::accept(std::function<void(Connection*)> callback) {
    return accept([&](std::shared_ptr<Connection> con) {
        callback(con.get());
    });
}

void Listener::stop() {
    running = false;
    if (acceptor) {
        acceptor->close();
        acceptor = nullptr;
    }
}