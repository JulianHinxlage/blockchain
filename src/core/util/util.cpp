
#include "util.h"

std::vector<std::string> split(const std::string& string, const std::string& delimiter, bool includeEmpty) {
	std::vector<std::string> parts;
	std::string token;
	int delimiterIndex = 0;
	for (char c : string) {
		if ((int)delimiter.size() == 0) {
			parts.push_back({ c, 1 });
		}
		else if (c == delimiter[delimiterIndex]) {
			delimiterIndex++;
			if (delimiterIndex == delimiter.size()) {
				if (includeEmpty || (int)token.size() != 0) {
					parts.push_back(token);
				}
				token.clear();
				delimiterIndex = 0;
			}
		}
		else {
			token += delimiter.substr(0, delimiterIndex);
			token.push_back(c);
			delimiterIndex = 0;
		}
	}
	token += delimiter.substr(0, delimiterIndex);
	if (includeEmpty || (int)token.size() != 0) {
		parts.push_back(token);
	}
	return parts;
}
