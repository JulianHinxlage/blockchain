//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Endpoint.h"
#include "ErrorCode.h"

namespace net {

	class UdpSocket {
	public:
		UdpSocket();
		~UdpSocket();

		ErrorCode create(bool prefereIpv4 = false);
		ErrorCode listen(uint16_t port, bool prefereIpv4 = false, bool reuseAddress = false, bool dualStacking = false);
		void close();

		ErrorCode write(const void* data, int bytes, const Endpoint &endpoint);
		ErrorCode read(void* data, int &bytes, Endpoint& endpoint);

	private:
		int handle;
	};

}
