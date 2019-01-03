#include "stdafx.h"
#include "general.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <array>
#include <algorithm>

std::wstring LastErrorString() {
	DWORD err = GetLastError();
	std::wstring str;
	LPTSTR s;
	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, (LPTSTR)&s, 0, NULL)) {
		LPTSTR p = _tcsrchr(s, _T('\r'));
		if (p != NULL) { *p = _T('\0'); }
		str = std::wstring(s) + L" [0x" + to_string(err, std::hex) + L" (" + to_string(LOWORD(err)) + L")]";
		LocalFree(s);
	}
	else {
		str = L"Unknown error: 0x" + to_string(err, std::hex) + L" (" + to_string(LOWORD(err)) + L")";
	}
	return str;
}

void ReportLastError(const std::wstring& s) {
	ReportLastError(s, false);
}
void ReportLastError(const std::wstring& s, bool warning) {
	std::wcerr << (warning ? L"* Warning: " : L"! Error: ") << s << ": " << LastErrorString();
}

std::wstring getTypeName(PE::const_resid rid) {
	if (!IS_INTRESOURCE(rid)) { return std::wstring(rid); }
	else if (rid == RT_CURSOR) { return L"CURSOR"; }
	else if (rid == RT_BITMAP) { return L"BITMAP"; }
	else if (rid == RT_ICON) { return L"ICON"; }
	else if (rid == RT_MENU) { return L"MENU"; }
	else if (rid == RT_DIALOG) { return L"DIALOG"; }
	else if (rid == RT_STRING) { return L"STRING"; }
	else if (rid == RT_FONTDIR) { return L"FONTDIR"; }
	else if (rid == RT_FONT) { return L"FONT"; }
	else if (rid == RT_ACCELERATOR) { return L"ACCELERATOR"; }
	else if (rid == RT_RCDATA) { return L"RCDATA"; }
	else if (rid == RT_MESSAGETABLE) { return L"MESSAGETABLE"; }
	else if (rid == RT_GROUP_ICON) { return L"GROUP_ICON"; }
	else if (rid == RT_GROUP_CURSOR) { return L"GROUP_CURSOR"; }
	else if (rid == RT_VERSION) { return L"VERSION"; }
	else if (rid == RT_DLGINCLUDE) { return L"DLGINCLUDE"; }
	else if (rid == RT_PLUGPLAY) { return L"PLUGPLAY"; }
	else if (rid == RT_VXD) { return L"VXD"; }
	else if (rid == RT_ANICURSOR) { return L"ANICURSOR"; }
	else if (rid == RT_ANIICON) { return L"ANIICON"; }
	else if (rid == RT_HTML) { return L"HTML"; }
	else if (rid == RT_MANIFEST) { return L"MANIFEST"; }
	else { return to_string((int)(intptr_t)rid); }
}

std::wstring getName(PE::const_resid rid) {
	return IS_INTRESOURCE(rid) ? to_string((int)(intptr_t)rid) : std::wstring(rid);
}

std::wstring getPath(const std::wstring& directory, PE::const_resid name, const std::wstring& ext) {
	return directory + L"\\" + sanitizeFilename(getName(name) + L'.' + ext);
}

template <typename T, size_t N>
bool contains(const std::array<T, N>& arr, const T& val) {
	return std::find(std::begin(arr), std::end(arr), val) != std::end(arr);
}


std::wstring sanitizeFilename(std::wstring filename) {
	static const std::array<wchar_t, 9> invalids = {{ L'\"', L'*', L'/', L':', L'<', L'>', L'?', L'\\', L'|' }}; // sorted by ASCII code
	for (size_t i = 0; i < filename.size(); i++) {
		if (!iswprint(filename[i]) || contains(invalids, filename[i])) {
			filename[i] = L'_';
		}
	}
	static const std::array<std::wstring, 25> reserved = {{
		L"CON", L"PRN", L"AUX", L"CLOCK$", L"NUL",
		L"COM0", L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
		L"LPT0", L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9",
	}};
	std::wstring upper(filename);
	std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
	if (contains(reserved, upper)) { filename = filename + L"_"; }
	return filename;
}

bool writeFile(const std::wstring& path, const void * const data, size_t size) {
	FILE* f = _wfopen(path.c_str(), L"wb");
	if (!f || ferror(f)) { return false; }
	size_t total = 0, written;
	do {
		written = fwrite(((const unsigned char * const)data) + total, 1, size - total, f);
		if (ferror(f)) { fclose(f); return false; }
		total += written;
	} while (total != size);
	fclose(f);
	return true;
}

bool createDirectory(const std::wstring& path) {
	return CreateDirectory(path.c_str(), NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}
