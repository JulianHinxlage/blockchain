#include "log.h"
#include <cstdarg>
#include <mutex>
#include <chrono>
#include <ctime>

std::function<void(LogLevel level, const std::string& category, const std::string& msg)> logCallback = [](LogLevel level, const std::string& category, const std::string& msg) {printf("%s\n", msg.c_str());};
std::mutex logMutex;

void setLogCallback(const std::function<void(LogLevel level, const std::string& category, const std::string& msg)>& callback) {
	logCallback = callback;
}

std::string dateAndTime() {
	time_t rawtime;
	struct tm* timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, sizeof(buffer), "[%d.%m.%Y] [%H:%M:%S]", timeinfo);
	return std::string(buffer);
}

const char* logLevelToString(LogLevel level) {
	switch (level)
	{
	case LogLevel::TRACE:
		return "TRACE";
	case LogLevel::DEBUG:
		return "DEBUG";
	case LogLevel::INFO:
		return "INFO";
	case LogLevel::WARNING:
		return "WARNING";
	case LogLevel::ERROR:
		return "ERROR";
	case LogLevel::FATAL:
		return "FATAL";
	default:
		return "DEFAULT";
	}
}

void log(LogLevel level, const char* category, const char* fmt, va_list args) {
	std::unique_lock<std::mutex> lovk(logMutex);
	static const int SIZE = 1024;
	static char buffer[SIZE];
	int len = 0;
	try {
		len = vsnprintf(buffer, SIZE-1, fmt, args);
	}
	catch (...) {
		len = 0;
	}
	if (len >= 0 && len < SIZE) {
		buffer[len] = '\0';
		std::string msg;
		msg += dateAndTime() + " ";
		msg = msg + "[" + logLevelToString(level) + "] " + "[" + category + "] " + buffer;
		if (logCallback) {
			logCallback(level, category, msg);
		}
	}
}

void log(LogLevel level, const char* category, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	log(level, category, fmt, args);
	va_end(args);
}
