// PEResourceDump: program for automated dumping of resources from pe-files
// Copyright (C) 2019  Jeffrey Bush  jeff@coderforlife.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// This is the source code file for the entry point into the program

#include "stdafx.h"

#include "general.h"
#define EXPOSE_DIRECT_RESOURCES
#include "PE/PEFile.h"
#include "ICO_CUR.h"

typedef PE::const_resid resid;

#include <string>
#include <iostream>

#pragma warning(push)
#pragma warning(disable:4100) // unreferenced formal parameter

typedef bool(*dump_func)(const PE::Rsrc * const rsrc, resid type, resid name, uint16_t lang, const std::wstring &directory, void* data, size_t size);

/* Converts DIB image to BMP image */
bool dib2bmp(LPVOID *data, size_t *size) {
	size_t off = ((uint32_t*)*data)[0];
	BYTE *d = (BYTE*)realloc(*data, *size + sizeof(BITMAPFILEHEADER));
	if (!d) { return false; }
	memmove(d + sizeof(BITMAPFILEHEADER), *data = d, *size);
	// need to add bitmap header: type="BM", size and offset now include header
	*size += sizeof(BITMAPFILEHEADER); off += sizeof(BITMAPFILEHEADER);
	BITMAPFILEHEADER bmp = { 0x4D42, (WORD)*size, 0, 0, (DWORD)off };
	memcpy(d, &bmp, sizeof(BITMAPFILEHEADER));
	return true;
}

/* Dumps a RT_BITMAP (actually a DIB) to a BMP file */
bool dump_bitmap(const PE::Rsrc * const rsrc, resid type, resid name, uint16_t lang, const std::wstring &directory, void* data, size_t size) {
	return (type == RT_BITMAP) && dib2bmp(&data, &size) && writeFile(getPath(directory, name, L"bmp"), data, size);
}

/* Dumps a RT_ICON, RT_CURSOR, RT_GROUP_ICON, or RT_GROUP_CURSOR to an ICO/CUR file */
bool dump_ico(const PE::Rsrc * const rsrc, resid type, resid name, uint16_t lang, const std::wstring &directory, void* data, size_t size) {
	bool retval = false;
	bool is_cursor = (type == RT_CURSOR || type == RT_GROUP_CURSOR);
	if (type == RT_ICON || type == RT_CURSOR) { retval = extractICOIndividual(type, name, lang, &data, &size, rsrc); }
	else if (type == RT_GROUP_ICON || type == RT_GROUP_CURSOR) { retval = extractICOGroup(type, name, lang, &data, &size, rsrc); }
	return retval && writeFile(getPath(directory, name, (is_cursor ? L"cur" : L"ico")), data, size);
}

/* Dumps a RT_MANIFEST to an XML file */
bool dump_manifest(const PE::Rsrc * const rsrc, resid type, resid name, uint16_t lang, const std::wstring &directory, void* data, size_t size) {
	if (type != RT_MANIFEST || ((char*)data)[0] != '<') { return false; }
	return writeFile(getPath(directory, name, L"xml"), data, size);
}

/* Dumps PNG, JPEG, or GIF images */
bool dump_image(const PE::Rsrc * const rsrc, resid type, resid name, uint16_t lang, const std::wstring &directory, void* data, size_t size) {
	static const uint8_t PNG_HEADER[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	static const uint8_t GIF_HEADER_START[] = { 0x47, 0x49, 0x46, 0x38 };
	static const uint8_t GIF_87_HEADER[] = { 0x37, 0x61 };
	static const uint8_t GIF_89_HEADER[] = { 0x39, 0x61 };
	static const uint8_t JPEG_HEADER_START[] = { 0xFF, 0xD8, 0xFF };
	static const uint8_t JPEG_1_HEADER = 0xD8;
	static const uint8_t JPEG_2_HEADER[] = { 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01 };
	static const uint8_t JPEG_3_HEADER = 0xEE;
	static const uint8_t JPEG_4_HEADER_START = 0xE1;
	static const uint8_t JPEG_4_HEADER_END[] = { 0x45, 0x78, 0x69, 0x66, 0x00, 0x00 };
	static const size_t JPEG_4_HEADER_GAP = sizeof(JPEG_HEADER_START) + 1 + 4;

	uint8_t* d = (uint8_t*)data;
	wchar_t* ext = nullptr;
	if (memcmp(d, PNG_HEADER, sizeof(PNG_HEADER)) == 0) { ext = L"png"; }
	else if (memcmp(d, GIF_HEADER_START, sizeof(GIF_HEADER_START)) == 0 && (
		memcmp(d + sizeof(GIF_HEADER_START), GIF_87_HEADER, sizeof(GIF_87_HEADER)) == 0 ||
		memcmp(d + sizeof(GIF_HEADER_START), GIF_89_HEADER, sizeof(GIF_89_HEADER)) == 0)) {
		ext = L"gif";
	}
	else if (memcmp(d, JPEG_HEADER_START, sizeof(JPEG_HEADER_START)) == 0 && (
		d[sizeof(JPEG_HEADER_START)] == JPEG_1_HEADER || d[sizeof(JPEG_HEADER_START)] == JPEG_3_HEADER ||
		memcmp(d + sizeof(JPEG_HEADER_START), JPEG_2_HEADER, sizeof(JPEG_2_HEADER)) == 0) ||
		(d[sizeof(JPEG_HEADER_START)] == JPEG_4_HEADER_START &&
			memcmp(d + JPEG_4_HEADER_GAP, JPEG_4_HEADER_END, sizeof(JPEG_4_HEADER_END)) == 0)) {
		ext = L"jpg";
	}
	return ext != nullptr && writeFile(getPath(directory, name, ext), data, size);
}

/* Dumps data straight to a binary file without any conversion, used as a fallback */
bool dump_binary(const PE::Rsrc * const rsrc, resid type, resid name, uint16_t lang, const std::wstring &directory, void* data, size_t size) {
	return writeFile(getPath(directory, name, L"bin"), data, size);
}

#pragma warning(pop)

// Dump functions are tried in this order
dump_func dumpers[] = {
	dump_bitmap, dump_ico, dump_manifest, dump_binary
};




int wmain(int argc, const wchar_t *argv[]) {
	std::wcerr << L"PEResourceDump Copyright (C) 2019  Jeffrey Bush <jeff@coderforlife.com>" << std::endl;
	std::wcerr << L"This program comes with ABSOLUTELY NO WARRANTY;" << std::endl;
	std::wcerr << L"This is free software, and you are welcome to redistribute it" << std::endl;
	std::wcerr << L"under certain conditions;" << std::endl;
	std::wcerr << L"See http://www.gnu.org/licenses/gpl.html for more details." << std::endl;
	std::wcerr << std::endl;

	// Check arguments and open them
	if (argc != 3) { std::wcerr << L"You must supply an EXE/DLL file and the output directory on the command line." << std::endl; return 1; }
	const wchar_t *filename = argv[1];
	const std::wstring directory = argv[2];
	PE::File *pe = new PE::File(filename, true);
	if (!pe->isLoaded()) { ReportLastError(L"Opening PE File"); delete pe; return 1; }
	const PE::Rsrc * const rsrc = pe->getResources();
	if (!rsrc || rsrc->isEmpty()) { std::wcerr << L"! Error: The EXE/DLL file does not have any resources." << std::endl; return 2; }
	if (!createDirectory(directory)) { ReportLastError(L"Cannot create directory '" + directory + L"'"); return 1; }

	// Loop through all resources and dump them
	for (resid type : rsrc->getTypes()) {
		const PE::ResourceType* rsrc_type = rsrc->operator[](type);
		std::wstring type_name = getTypeName(type), dir = directory + L"\\" + sanitizeFilename(type_name);
		if (!createDirectory(dir)) { ReportLastError(L"Cannot create directory '" + dir + L"'", true); continue; }
		for (resid name : rsrc_type->getNames()) {
			const PE::ResourceName* rsrc_name = rsrc_type->operator[](name);
			bool save_lang = rsrc_name->getLangs().size() > 1;
			std::wstring dir_lang = dir;
			for (uint16_t lang : rsrc_name->getLangs()) {
				const PE::ResourceLang* rsrc_lang = rsrc_name->operator[](lang);
				if (save_lang) {
					dir_lang = dir + L"\\" + to_string(lang);
					if (!createDirectory(dir_lang)) { ReportLastError(L"Cannot create directory '" + dir_lang + L"'", true); continue; }
				}
				size_t size, i;
				void* data = rsrc_lang->get(&size);
				for (i = 0; i < ARRAYSIZE(dumpers) && !dumpers[i](rsrc, type, name, lang, dir_lang, data, size); i++);
				if (i == ARRAYSIZE(dumpers)) { std::wcerr << L"! Warning: Cannot save " << dir_lang << L"\\" << getName(name) << std::endl; }
			}
		}
	}

	return 0;
}
