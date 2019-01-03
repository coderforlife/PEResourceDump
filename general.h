#pragma once

#include "PE\PEDataTypes.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>

template <class T>
inline std::wstring to_string(T t, std::ios_base & (*f)(std::ios_base&) = std::dec)
{
	std::wostringstream oss;
	oss << f << t;
	return oss.str();
}

// Gets the last error (GetLastError()) as a string
std::wstring LastErrorString();
// Outputs the last error
void ReportLastError(const std::wstring& s);
// Outputs the last error, possibly only as a warning
void ReportLastError(const std::wstring& s, bool warning);

std::wstring getTypeName(PE::const_resid rid);
std::wstring getName(PE::const_resid rid);
std::wstring getPath(const std::wstring& directory, PE::const_resid name, const std::wstring& ext);
std::wstring sanitizeFilename(std::wstring filename);
bool writeFile(const std::wstring& path, const void * const data, size_t size);

bool createDirectory(const std::wstring& path);
