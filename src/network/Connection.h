#pragma once

#include <boost/asio.hpp>

using namespace boost::asio;
using ip::tcp;

class SocketContext {
public:
	static boost::asio::io_service ioService;
};

class Connection {
public:
	tcp::socket socket;
	std::shared_ptr<std::thread> thread;
	bool connected;
	std::mutex writeMutex;

	Connection();
	~Connection();
	bool connect(const std::string& ip, int port);
	void disconnect();
	std::string read();
	bool write(const std::string& message);
};
