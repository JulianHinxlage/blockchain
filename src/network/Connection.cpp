#include "Connection.h"

boost::asio::io_service SocketContext::ioService;

Connection::Connection() 
	: socket(SocketContext::ioService) {
	thread = nullptr;
    connected = false;
}

Connection::~Connection() {
    disconnect();
}

bool Connection::connect(const std::string& ip, int port) {
    boost::system::error_code error;
    socket.connect(tcp::endpoint(boost::asio::ip::address::from_string(ip), port), error);
    if (!error) {
        connected = true;
    }
    return !(bool)error;
}

void Connection::disconnect() {
    connected = false;
    socket.close();
    if (thread != nullptr) {
        thread->detach();
        thread = nullptr;
    }
}

std::string Connection::read() {
    uint32_t bytes = 0;
    boost::system::error_code error;
    boost::asio::streambuf bytesBuf;
    boost::asio::read(socket, bytesBuf, boost::asio::transfer_exactly(sizeof(bytes)), error);
    if (error) {
        connected = false;
        return "";
    }
    bytes = *(uint32_t*)boost::asio::buffer_cast<const char*>(bytesBuf.data());

    boost::asio::streambuf buf;
    boost::asio::read(socket, buf, boost::asio::transfer_exactly(bytes), error);
    if (error) {
        connected = false;
        return "";
    }

    std::string data(boost::asio::buffer_cast<const char*>(buf.data()), buf.size());
    return data;
}

bool Connection::write(const std::string& message) {
    std::unique_lock<std::mutex> lock(writeMutex);
    boost::system::error_code error;
    uint32_t bytes = message.size();
    boost::asio::write(socket, boost::asio::buffer((void*)&bytes, sizeof(bytes)), error);
    boost::asio::write(socket, boost::asio::buffer(message), error);
    if (error) {
        connected = false;
    }
    return !(bool)error;
}
