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

#include "stdafx.h"
#include "ICO_CUR.h"

#include <vector>

#pragma region General Defines and Objects
////////////////////////////////////////////////////////////////////////////////
///// General Defines and Objects
////////////////////////////////////////////////////////////////////////////////
#define ICO_ID	1
#define CUR_ID	2

#include <pshpack2.h> //these require WORD alignment, not DWORD
struct ICO_CUR_HEADER {
	WORD wReserved;  // Always 0
	WORD wResID;     // ICO_ID or CUR_ID
	WORD wNumImages; // Number of image/directory entries
};
struct ICO_CUR_RT_ENTRY {
	union {
		struct {
			BYTE  bWidth;
			BYTE  bHeight;
			BYTE  bColorCount;
			BYTE  bReserved;
		} ICO;
		struct {
			WORD  wWidth;
			WORD  wHeight; // Divide by 2 to get the actual height.
		} CUR;
	};
	WORD  wPlanes;
	WORD  wBitCount;
	DWORD dwSize;
	WORD  wID;
};
#include <poppack.h>

struct CUR_HOTSPOT {
	WORD  wHotspotX;
	WORD  wHotspotY;
};
struct ICO_CUR_ENTRY {
	BYTE  bWidth;
	BYTE  bHeight;
	BYTE  bColorCount;
	BYTE  bReserved;
	union {
		struct {
			WORD  wPlanes;
			WORD  wBitCount;
		} ICO;
		CUR_HOTSPOT CUR;
	};
	DWORD dwSize;
	DWORD dwOffset;
};
#pragma endregion

#pragma region General Functions
////////////////////////////////////////////////////////////////////////////////
///// General Functions
////////////////////////////////////////////////////////////////////////////////
// Finds the icon/cursor group that a particular icon/cursor belongs to.
// IN		type must be RT_GROUP_CURSOR or RT_GROUP_ICON
// IN		name/lang are for the icon or group
// OUT		grpIndx is set to the group found
// OUT		return value is the data for the found group
// All of the OUT and IN_OUT parameters are OPTIONAL (you can provide NULL if you don't care about them). 
LPVOID findICOGroup(LPCWSTR type, LPCWSTR name, WORD lang, const PE::Rsrc * const r, WORD *grpIndx) {
	std::vector<LPCWSTR> names = r->getNames(type);
	for (DWORD i = 0; i < names.size(); i++) {
		size_t size = 0;
		LPVOID gdata = r->get(type, names[i], lang, &size);
		if (!gdata)
			continue;
		ICO_CUR_HEADER *header = (ICO_CUR_HEADER*)gdata;
		ICO_CUR_RT_ENTRY *entries = (ICO_CUR_RT_ENTRY*)((LPBYTE)gdata+sizeof(ICO_CUR_HEADER));
		for (WORD j = 0; j < header->wNumImages; j++) {
			if (MAKEINTRESOURCE(entries[j].wID) == name) {
				*grpIndx = j;
				return gdata;
			}
		}
		free(gdata);
	}
	SetLastError(ERROR_MOD_NOT_FOUND);
	return NULL;
}
#pragma endregion

#pragma region Extract Functions
////////////////////////////////////////////////////////////////////////////////
///// Extract Functions
////////////////////////////////////////////////////////////////////////////////
bool extractICOIndividual(LPCWSTR type, LPCWSTR name, WORD lang, LPVOID *data, size_t *size, const PE::Rsrc * const r) {
	type = (type==RT_GROUP_CURSOR||type==RT_CURSOR)?RT_GROUP_CURSOR:RT_GROUP_ICON;

	WORD grpIndx;
	LPVOID gdata = findICOGroup(type, name, lang, r, &grpIndx);
	if (!gdata)
		return false;

	ICO_CUR_RT_ENTRY *rtEntry = ((ICO_CUR_RT_ENTRY*)((LPBYTE)gdata+sizeof(ICO_CUR_HEADER)))+grpIndx;
	ICO_CUR_ENTRY entry = *(ICO_CUR_ENTRY*)(rtEntry);
	BYTE *bytes = (BYTE*)*data;

	// some extra processing is required for cursors
	if (type == RT_GROUP_CURSOR) {
		if (entry.bWidth == 0) {
			entry.bHeight = (BYTE)(rtEntry->CUR.wHeight / 2);
			entry.bWidth = (BYTE)(rtEntry->CUR.wWidth);
			entry.bColorCount = 0;
			entry.bReserved = 0;
		}
		entry.CUR = *(CUR_HOTSPOT*)bytes;
		bytes += sizeof(CUR_HOTSPOT);
		entry.dwSize -= sizeof(CUR_HOTSPOT); //!! is this really right?
	}

	entry.dwOffset = sizeof(ICO_CUR_HEADER)+sizeof(ICO_CUR_ENTRY);

	free(gdata);

	ICO_CUR_HEADER header = {0, (WORD)((type==RT_GROUP_CURSOR)?CUR_ID:ICO_ID), 1};

	*size = sizeof(ICO_CUR_HEADER)+sizeof(ICO_CUR_ENTRY)+entry.dwSize;
	BYTE *d = (BYTE*)malloc(*size);
	memcpy(d, &header, sizeof(ICO_CUR_HEADER));
	memcpy(d+sizeof(ICO_CUR_HEADER), &entry, sizeof(ICO_CUR_ENTRY));
	memcpy(d+sizeof(ICO_CUR_HEADER)+sizeof(ICO_CUR_ENTRY), bytes, entry.dwSize);
	free(*data);
	*data = d;
	return true;
}

bool extractICOGroup(LPCWSTR type, LPCWSTR name, WORD lang, LPVOID *data, size_t *size, const PE::Rsrc * const r) {
	UNREFERENCED_PARAMETER(name); // unreferenced parameter

	type = (type==RT_GROUP_CURSOR||type==RT_CURSOR)?RT_CURSOR:RT_ICON;

	BYTE *bytes = (BYTE*)*data;

	// get the ICO/CUR header
	ICO_CUR_HEADER *header = (ICO_CUR_HEADER*)bytes;

	// get all the RT entries
	ICO_CUR_RT_ENTRY *entries = (ICO_CUR_RT_ENTRY*)(bytes+sizeof(ICO_CUR_HEADER));

	// calculate where the entries and img data start
	DWORD entry_offset = sizeof(ICO_CUR_HEADER);
	DWORD offset = entry_offset+sizeof(ICO_CUR_ENTRY)*header->wNumImages;

	// write the header stuff
	BYTE *d = (BYTE*)malloc(offset);
	memcpy(d, header, sizeof(ICO_CUR_HEADER));

	for (DWORD i = 0; i < header->wNumImages; i++) {

		// get the current entry and image data
		ICO_CUR_ENTRY entry = *(ICO_CUR_ENTRY*)(entries+i);
		size_t temp_size;
		LPVOID img_orig = r->get(type, MAKEINTRESOURCE(entries[i].wID), lang, &temp_size), img = img_orig;
		entry.dwSize = (DWORD)temp_size;

		// some extra processing is required for cursors
		if (type == RT_CURSOR) {
			if (entry.bWidth == 0) {
				entry.bHeight = (BYTE)(entries[i].CUR.wHeight / 2);
				entry.bWidth = (BYTE)(entries[i].CUR.wWidth);
				entry.bColorCount = 0;
				entry.bReserved = 0;
			}
			entry.CUR = *(CUR_HOTSPOT*)img;
			img = ((LPBYTE)img)+sizeof(CUR_HOTSPOT);
			entry.dwSize -= sizeof(CUR_HOTSPOT);
		}
		entry.dwOffset = offset;

		// write the data
		d = (BYTE*)realloc(d, offset+entry.dwSize);
		memcpy(d+entry_offset, &entry, sizeof(ICO_CUR_ENTRY));
		memcpy(d+offset, img, entry.dwSize);

		// update the offsets
		entry_offset += sizeof(ICO_CUR_ENTRY);
		offset += entry.dwSize; //entries[i].dwSize;

		// cleanup
		free(img_orig);
	}

	free(*data);
	*data = d;
	*size = offset;

	return true;
}
#pragma endregion
