//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#pragma once

#undef NO_ERROR

namespace net {

	enum class ErrorCode {
		NO_ERROR = 0,
		GENERAL_ERROR,
		CONNECTION_REFUSED,
		TIME_OUT,
		DISCONNECTED,
		RESET,
		INVALID_ENDPOINT,
		ENDPOINT_IN_USE,
		INVALID_PACKET,
	};

	const char* getErrorString(ErrorCode error);
	ErrorCode getLastError();
	
	ErrorCode getErrorCodeFromInternal(int internal);
	int getInternalFromErrorCode(ErrorCode error);
	const char* getInternalErrorString(int internal);

}
