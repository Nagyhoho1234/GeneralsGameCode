/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Generic Win32 basic types (HWND, DWORD, RECT, ...) for non-Windows builds.
// Nothing in this codebase's compat headers has needed these before now -
// every prior user of them was gated behind _WIN32. Kept generic (not
// D3D8-specific) since Phase 4 windowing work will also need HWND-like
// types; D3D8-specific vocabulary lives in WW3D2/PortableD3D8/ instead.
#pragma once

#ifndef WIN32_COMPAT_H
#define WIN32_COMPAT_H

#ifndef _WIN32

#include <cstdint>
#include <cstring>

// DWORD/WORD/BYTE/BOOL/UINT/ULONG also get typedef'd by WWLib/bittype.h -
// whichever header is included first wins so the two never fight over the
// same names (see the matching #ifndef WIN32_COMPAT_H there).
#ifndef WWLIB_BITTYPE_H
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef uint32_t UINT;
typedef int32_t BOOL;
typedef uint32_t ULONG;
#endif
typedef int32_t LONG;
typedef long HRESULT;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef CONST
#define CONST const
#endif

// Opaque window handle - a real pointer on Windows, an opaque tag here.
// The GL backend never dereferences it; native-port-spike proved a GLFW
// window pointer can stand in wherever the game code merely threads an
// HWND through as an opaque value.
struct HWND__;
typedef HWND__* HWND;

typedef struct tagRECT
{
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT;

typedef struct tagPOINT
{
	LONG x;
	LONG y;
} POINT;

struct GUID
{
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t  Data4[8];
};

inline bool operator==(const GUID& a, const GUID& b)
{
	return std::memcmp(&a, &b, sizeof(GUID)) == 0;
}

inline bool operator!=(const GUID& a, const GUID& b)
{
	return !(a == b);
}

#define S_OK ((HRESULT)0L)
#define S_FALSE ((HRESULT)1L)
#define E_FAIL ((HRESULT)0x80004005L)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)

#endif // !_WIN32

#endif // WIN32_COMPAT_H
