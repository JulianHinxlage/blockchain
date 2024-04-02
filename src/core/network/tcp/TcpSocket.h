//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Endpoint.h"
#include "ErrorCode.h"
#include <memory>

namespace net {

	class TcpSocket {
	public:
		int bytesUp = 0;
		int bytesDown = 0;

		TcpSocket();
		~TcpSocket();

		ErrorCode connect(const Endpoint &endpoint);
		ErrorCode connect(const std::string& address, uint16_t port, bool resolve = true, bool prefereIpv4 = false);
		ErrorCode listen(uint16_t port, bool prefereIpv4 = false, bool reuseAddress = false, bool dualStacking = false);
		std::shared_ptr<TcpSocket> accept();
		bool disconnect();

		bool isConnected();
		const Endpoint& getEndpoint();

		ErrorCode write(const void* data, int bytes);
		ErrorCode read(void* data, int &bytes);

		int getHandle();
	private:
		Endpoint endpoint;
		bool connected;
		int handle;
	};

}
