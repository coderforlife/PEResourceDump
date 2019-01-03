#pragma once

// Target Windows XP
#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#undef ABSOLUTE // interferes with PE library

#include <assert.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <algorithm>

#if _WIN64
#pragma comment(lib, "PE\\PEFile64.lib")
#else
#pragma comment(lib, "PE\\PEFile.lib")
#endif
