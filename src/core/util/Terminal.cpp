//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "Terminal.h"
#include <stdarg.h>

void Terminal::init() {
	inputLine = "";
}

void Terminal::print(const std::string& str) {
	clearInputLine();
	printf(str.c_str());
	printf(inputLine.c_str());
}

void Terminal::println(const std::string& str) {
	print(str + "\n");
}

void Terminal::log(const char* fmt, va_list args) {
	static const int SIZE = 1024;
	static char buffer[SIZE];
	int len = 0;
	try {
		len = vsnprintf(buffer, SIZE - 1, fmt, args);
	}
	catch (...) {
		len = 0;
	}
	if (len >= 0 && len < SIZE) {
		buffer[len] = '\0';
		print(buffer);
	}
}

void Terminal::log(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	log(fmt, args);
	va_end(args);
}

void Terminal::clearInputLine() {
	int count = inputLine.size();
	for (int i = 0; i < count; i++) {
		printf("\b");
	}
	for (int i = 0; i < count; i++) {
		printf(" ");
	}
	for (int i = 0; i < count; i++) {
		printf("\b");
	}
}

#ifdef WIN32
#include <Windows.h>

std::string Terminal::input(const std::string& prefix) {
	inputLine = prefix;
	printf(prefix.c_str());

	std::string input;
	const char BACKSPACE = 127;
	const char RETURN = 13;
	const char UP = 72;
	const char DOWN = 80;
	const char SPECIAL = 27;

	DWORD mode, count;
	HANDLE ih = GetStdHandle(STD_INPUT_HANDLE);

	GetConsoleMode(ih, &mode);
	SetConsoleMode(ih, mode & ~(ENABLE_LINE_INPUT) | (ENABLE_VIRTUAL_TERMINAL_INPUT));

	while (true) {
		char ch;
		if (ReadConsoleA(ih, &ch, 1, &count, NULL) && count == 1) {
			if (ch == RETURN) {
				printf("\n");
				break;
			}
			else if (ch == BACKSPACE) {
				if (!input.empty()) {
					printf("\b \b");
					input.pop_back();
					inputLine.pop_back();
				}
			}
			else if (ch == SPECIAL) {
				ReadConsoleA(ih, &ch, 1, &count, NULL);
				ReadConsoleA(ih, &ch, 1, &count, NULL);
				if (ch == 'A') {
					ch = UP;
				}
				else if (ch == 'B') {
					ch = DOWN;
				}

				if (ch == UP) {
					if (historyIndex == -1) {
						historyIndex = history.size() - 1;
						tmpHistory = input;
					}
					else if(historyIndex > 0) {
						historyIndex--;
					}
					if (historyIndex >= 0 && historyIndex < history.size()) {
						clearInputLine();
						input = history[historyIndex];
						inputLine = prefix + input;
						printf(inputLine.c_str());
					}
				}
				else if (ch == DOWN) {
					if (historyIndex == -1) {
						historyIndex = history.size() - 1;
						tmpHistory = input;
					}
					else if (historyIndex < history.size()) {
						historyIndex++;
					}

					if (historyIndex >= 0 && historyIndex <= history.size()) {
						if (historyIndex == history.size()) {
							input = tmpHistory;
						}
						else {
							input = history[historyIndex];
						}

						clearInputLine();
						inputLine = prefix + input;
						printf(inputLine.c_str());
					}
				}
			}
			else {
				printf("%c", ch);
				input.push_back(ch);
				inputLine.push_back(ch);
			}
		}
	}

	SetConsoleMode(ih, mode);
	history.push_back(input);
	inputLine = "";
	tmpHistory = "";
	historyIndex = -1;
	return input;
}

std::string Terminal::passwordInput(const std::string& prefix, bool useAsterisk) {
	inputLine = prefix;
	printf(prefix.c_str());

	std::string input;
	const char BACKSPACE = 8;
	const char RETURN = 13;

	DWORD mode, count;
	HANDLE ih = GetStdHandle(STD_INPUT_HANDLE);

	GetConsoleMode(ih, &mode);
	SetConsoleMode(ih, mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));

	while (true) {
		char ch;
		if (ReadConsoleA(ih, &ch, 1, &count, NULL) && count == 1) {
			if (ch == RETURN) {
				printf("\n");
				break;
			}
			if (ch == BACKSPACE) {
				if (!input.empty()) {
					if (useAsterisk) {
						printf("\b \b");
						inputLine.pop_back();
					}
					input.pop_back();
				}
			}
			else {
				if (useAsterisk) {
					printf("*");
					inputLine.push_back('*');
				}
				input.push_back(ch);
			}
		}
	}

	SetConsoleMode(ih, mode);

	inputLine = "";
	return input;
}

#else
#include <iostream>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <cstdio>

std::string Terminal::input(const std::string& prefix) {
    inputLine = prefix;
    std::cout << prefix;
	std::cout.flush();

    std::string input;
    const char BACKSPACE = 127;
    const char RETURN = '\n';
    const char ESC = 27;

    static struct termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (true) {
        char ch;
        ssize_t bytesRead = read(STDIN_FILENO, &ch, 1);

        if (bytesRead == 1) {
            if (ch == RETURN) {
                std::cout << std::endl;
				std::cout.flush();
                break;
            } else if (ch == BACKSPACE) {
                if (!input.empty()) {
                    std::cout << "\b \b";
					std::cout.flush();
                    input.pop_back();
                    inputLine.pop_back();
                }
            } else if (ch == ESC) {
                char seq[2];
                read(STDIN_FILENO, &seq[0], 1);
                read(STDIN_FILENO, &seq[1], 1);

                if (seq[0] == '[') {
                    if (seq[1] == 'A') {
                        if (historyIndex == -1) {
                            historyIndex = history.size() - 1;
                            tmpHistory = input;
                        } else if (historyIndex > 0) {
                            historyIndex--;
                        }

                        if (historyIndex >= 0 && historyIndex < history.size()) {
                            clearInputLine();
                            input = history[historyIndex];
                            inputLine = prefix + input;
                            std::cout << inputLine;
							std::cout.flush();
                        }
                    } else if (seq[1] == 'B') {
                        if (historyIndex == -1) {
                            historyIndex = history.size() - 1;
                            tmpHistory = input;
                        } else if (historyIndex < history.size()) {
                            historyIndex++;
                        }

                        if (historyIndex >= 0 && historyIndex <= history.size()) {
                            clearInputLine();
                            if (historyIndex == history.size()) {
                                input = tmpHistory;
                            } else {
                                input = history[historyIndex];
                            }
                            inputLine = prefix + input;
                            std::cout << inputLine;
							std::cout.flush();
                        }
                    }
                }
            } else {
                std::cout << ch;
				std::cout.flush();
                input.push_back(ch);
                inputLine.push_back(ch);
            }
        }
    }

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    history.push_back(input);
    inputLine = "";
    tmpHistory = "";
    historyIndex = -1;
    return input;
}

std::string Terminal::passwordInput(const std::string& prefix, bool useAsterisk) {
	std::string input;
    const char BACKSPACE = 127;
    const char RETURN = '\n';
    std::cout << prefix;
    fflush(stdout);

    static struct termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (true) {
        char ch;
        ssize_t bytesRead = read(STDIN_FILENO, &ch, 1);

        if (bytesRead == 1) {
            if (ch == RETURN) {
                std::cout << std::endl;
                break;
            } else if (ch == BACKSPACE) {
                if (!input.empty()) {
                    std::cout << "\b \b";
                    input.pop_back();
                }
            } else {
                if (useAsterisk) {
                    std::cout << "*";
                }
                input.push_back(ch);
            }
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return input;
}

#endif
