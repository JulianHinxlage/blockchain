//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include "ErrorCode.h"

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

	const char* getErrorString(ErrorCode error) {
		switch (error) {
		case ErrorCode::NO_ERROR:
			return "NO_ERROR";
		case ErrorCode::GENERAL_ERROR:
			return "GENERAL_ERROR";
		case ErrorCode::DISCONNECTED:
			return "DISCONNECTED";
		case ErrorCode::CONNECTION_REFUSED:
			return "CONNECTION_REFUSED";
		case ErrorCode::TIME_OUT:
			return "TIME_OUT";
		case ErrorCode::INVALID_ENDPOINT:
			return "INVALID_ENDPOINT";
		case ErrorCode::RESET:
			return "RESET";
		case ErrorCode::ENDPOINT_IN_USE:
			return "ENDPOINT_IN_USE";
		case ErrorCode::INVALID_PACKET:
			return "INVALID_PACKET";
		default:
			return "UNDEFINED";
		}
	}

#ifdef WIN32

	ErrorCode getLastError() {
		return getErrorCodeFromInternal(WSAGetLastError());
	}

	ErrorCode getErrorCodeFromInternal(int internal) {
		switch (internal) {
		case 0:
			return ErrorCode::NO_ERROR;
		case WSAEINTR:
			return ErrorCode::DISCONNECTED;
		case WSAECONNREFUSED:
			return ErrorCode::CONNECTION_REFUSED;
		case WSAETIMEDOUT:
			return ErrorCode::TIME_OUT;
		case WSAECONNRESET:
			return ErrorCode::RESET;
		case WSAEADDRINUSE:
			return ErrorCode::ENDPOINT_IN_USE;
		default:
			return ErrorCode::GENERAL_ERROR;
		}
	}

	int getInternalFromErrorCode(ErrorCode error) {
		switch (error) {
		case ErrorCode::NO_ERROR:
			return 0;
		case ErrorCode::GENERAL_ERROR:
			return -1;
		case ErrorCode::DISCONNECTED:
			return -1;
		case ErrorCode::CONNECTION_REFUSED:
			return WSAECONNREFUSED;
		case ErrorCode::TIME_OUT:
			return WSAETIMEDOUT;
		case ErrorCode::INVALID_ENDPOINT:
			return -1;
		case ErrorCode::RESET:
			return WSAECONNRESET;
		case ErrorCode::ENDPOINT_IN_USE:
			return WSAEADDRINUSE;
		default:
			return -1;
		}
	}

	const char* getInternalErrorString(int internal) {
		wchar_t* wstr = NULL;
		static char str[1024];
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, internal,
			MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			(LPWSTR)&wstr, 0, NULL);
		wcstombs(str, wstr, sizeof(str));
		return str;
	}

#else

	ErrorCode getLastError() {
		return ErrorCode::GENERAL_ERROR;
	}

	ErrorCode getErrorCodeFromInternal(int internal) {
		switch (internal) {
		case 0:
			return NO_ERROR;
		default:
			return GENERAL_ERROR;
		}
	}

	int getInternalFromErrorCode(ErrorCode error) {
		return 0;
	}

	const char* getInternalErrorString(int internal) {
		return "";
	}

#endif

}
