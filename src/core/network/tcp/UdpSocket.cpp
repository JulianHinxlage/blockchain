//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include "UdpSocket.h"
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

	UdpSocket::UdpSocket() {
		handle = -1;
	}

	UdpSocket::~UdpSocket() {
		close();
	}

	ErrorCode UdpSocket::create(bool prefereIpv4) {
		handle = socket(prefereIpv4 ? AF_INET : AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (handle == -1) {
			return getLastError();
		}
		return ErrorCode::NO_ERROR;
	}

	ErrorCode UdpSocket::listen(uint16_t port, bool prefereIpv4, bool reuseAddress, bool dualStacking) {
		struct sockaddr_in6 addr6;
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
			close();
		}

		handle = socket(addr6.sin6_family, SOCK_DGRAM, IPPROTO_UDP);
		if (handle == -1) {
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

		int code = ::bind(handle, (sockaddr*)&addr6, sizeof(addr6));
		if (code != 0) {
			ErrorCode error = getLastError();
			close();
			return error;
		}

		return ErrorCode::NO_ERROR;
	}

	void UdpSocket::close() {
#if WIN32
		int status = shutdown(handle, SD_BOTH);
		if (status == 0) {
			status = closesocket(handle);
			handle = -1;
		}
#else
		int status = shutdown(handle, SHUT_RDWR);
		if (status == 0) {
			status = ::close(handle);
			handle = -1;
		}
#endif
	}

	ErrorCode UdpSocket::write(const void* data, int bytes, const Endpoint& endpoint) {
		int code = ::sendto(handle, (char*)data, bytes, 0, (sockaddr*)endpoint.getHandle(), sizeof(endpoint));
		if (code < 0) {
			return getLastError();
		}
		return ErrorCode::NO_ERROR;
	}

	ErrorCode UdpSocket::read(void* data, int& bytes, Endpoint& endpoint) {
		socklen_t size = sizeof(Endpoint);
		bytes = ::recvfrom(handle, (char*)data, bytes, 0, (sockaddr*)endpoint.getHandle(), &size);
		if (bytes <= 0) {
			return getLastError();
		}
		return ErrorCode::NO_ERROR;
	}

}
