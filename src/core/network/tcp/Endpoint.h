//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <cstdint>
#include <string>

namespace net {

	class Endpoint {
	public:
		Endpoint();
		Endpoint(const std::string& address, uint16_t port, bool resolve = true, bool prefereIpv4 = false);
		~Endpoint();

		void setPort(uint16_t port);
		void setAddress(const std::string& address, bool resolve = true, bool prefereIpv4 = false);
		void set(const std::string& address, uint16_t port, bool resolve = true, bool prefereIpv4 = false);

		uint16_t getPort() const;
		std::string getAddress() const;

		bool isIpv4() const;
		bool isIpv6() const;
		bool isValid() const;

		void* getHandle();
		const void* getHandle() const;
	private:
		uint8_t data[28];
	};

}
