//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

class Terminal {
public:
	void init();
	void print(const std::string &str);
	void println(const std::string& str);

	void log(const char* fmt, va_list args);
	void log(const char* fmt, ...);

	std::string input(const std::string& prefix);
	std::string passwordInput(const std::string &prefix, bool useAsterisk = false);

private:
	std::string inputLine;
	std::vector<std::string> history;
	std::string tmpHistory;
	int historyIndex = -1;

	void clearInputLine();
};
