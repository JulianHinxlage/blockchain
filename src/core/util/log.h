#pragma once

#include <string>
#include <functional>

enum class LogLevel {
	TRACE,
	DEBUG,
	INFO,
	WARNING,
	ERROR,
	FATAL,
};

void setLogCallback(const std::function<void(LogLevel level, const std::string& category, const std::string &msg)>& callback);

void log(LogLevel level, const char* category, const char* fmt, va_list args);
void log(LogLevel level, const char* category, const char *fmt, ...);
