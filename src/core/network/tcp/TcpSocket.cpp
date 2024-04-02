//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include "TcpSocket.h"
#include <cstring>

#if WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#undef NO_ERROR
#else
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<arpa/inet.h>
#endif

namespace net {
	
	TcpSocket::TcpSocket() {
		handle = -1;
		connected = false;
	}

	TcpSocket::~TcpSocket() {
		disconnect();
	}

	ErrorCode TcpSocket::connect(const Endpoint& ep) {
		if (handle != -1) {
			disconnect();
		}

		endpoint = ep;

		if (!endpoint.isValid()) {
			return ErrorCode::INVALID_ENDPOINT;
		}

		handle = socket(ep.isIpv4() ? AF_INET : AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		if (handle == -1) {
			connected = false;
			return getLastError();
		}

		int code = ::connect(handle, (sockaddr*)endpoint.getHandle(), sizeof(Endpoint));
		if (code != 0) {
			connected = false;
			ErrorCode error = getLastError();
			disconnect();
			return error;
		}

		connected = true;
		return ErrorCode::NO_ERROR;
	}

	ErrorCode TcpSocket::connect(const std::string& address, uint16_t port, bool resolve, bool prefereIpv4) {
		return connect(Endpoint(address, port, resolve, prefereIpv4));
	}

	ErrorCode TcpSocket::listen(uint16_t port, bool prefereIpv4, bool reuseAddress, bool dualStacking) {
		struct sockaddr_in6 &addr6 = *(sockaddr_in6*)endpoint.getHandle();
		struct sockaddr_in& addr4 = *(sockaddr_in*)&addr6;
		memset(&addr6, 0, sizeof(addr6));
		
		if (prefereIpv4) {
			addr4.sin_family = AF_INET;
			addr4.sin_port = htons(port);
#if WIN32
			addr4.sin_addr.S_un.S_addr = INADDR_ANY;
#else
			addr4.sin_addr.s_addr = INADDR_ANY;
#endif
		}
		else {
			addr6.sin6_family = AF_INET6;
			addr6.sin6_port = htons(port);
			addr6.sin6_addr = in6addr_any;
		}

		if (handle != -1) {
			disconnect();
		}

		handle = socket(addr6.sin6_family, SOCK_STREAM, IPPROTO_TCP);
		if (handle == -1) {
			connected = false;
			return getLastError();
		}

		if (reuseAddress) {
			int flag = 1;
			setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));
		}
		
		if (dualStacking) {
			int flag = 0;
			setsockopt(handle, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&flag, sizeof(flag));
		}

		int code = bind(handle, (sockaddr*)&addr6, sizeof(addr6));
		if (code != 0) {
			connected = false;
			ErrorCode error = getLastError();
			disconnect();
			return error;
		}

		code = ::listen(handle, 10);
		if (code != 0) {
			connected = false;
			ErrorCode error = getLastError();
			disconnect();
			return error;
		}

		connected = true;
		return ErrorCode::NO_ERROR;
	}

	std::shared_ptr<TcpSocket> TcpSocket::accept() {
		Endpoint ep;

		socklen_t size = sizeof(Endpoint);
		int result = ::accept(handle, (sockaddr*)ep.getHandle(), &size);
		if (result == -1) {
			connected = false;
			return nullptr;
		}

		std::shared_ptr<TcpSocket> socket = std::make_shared<TcpSocket>();
		socket->handle = result;
		socket->endpoint = ep;
		socket->connected = true;
		return socket;
	}

	bool TcpSocket::disconnect() {
#if WIN32
		int status = shutdown(handle, SD_BOTH);
		status = closesocket(handle);
		handle = -1;
#else
		int status = shutdown(handle, SHUT_RDWR);
		status = ::close(handle);
		handle = -1;
#endif
		connected = false;
		return status == 0;
	}

	bool TcpSocket::isConnected() {
		return connected;
	}

	const Endpoint& TcpSocket::getEndpoint() {
		return endpoint;
	}

	ErrorCode TcpSocket::write(const void* data, int bytes) {
		int code = ::send(handle, (char*)data, bytes, 0);
		if (code < 0) {
			connected = false;
			return getLastError();
		}
		bytesUp += bytes;
		return ErrorCode::NO_ERROR;
	}

	ErrorCode TcpSocket::read(void* data, int& bytes) {
		int code = ::recv(handle, (char*)data, bytes, 0);
		if (code <= 0) {
			connected = false;
			if (code == 0) {
				return ErrorCode::DISCONNECTED;
			}
			return getLastError();
		}
		bytes = code;
		bytesDown += bytes;
		return ErrorCode::NO_ERROR;
	}

	int TcpSocket::getHandle() {
		return handle;
	}

}
