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

// This file contains WCHAR and related macros for compatibility with non-windows platforms.
#pragma once

// For wcslen/wcscmp/wcschr/wcsrchr/wcscasecmp/mbstowcs/wcstombs, used by the
// aliases below and directly by callers of this header (e.g. UnicodeString.h)
#include <wchar.h>
#include <cstdlib>

// WCHAR
typedef wchar_t WCHAR;
typedef const WCHAR* LPCWSTR;
typedef WCHAR* LPWSTR;

#define _wcsicmp wcscasecmp
#define wcsicmp wcscasecmp

// _wtoi: MSVC's wide-string-to-int (native port plan Phase 1 Draft 11).
#ifndef _wtoi
inline int _wtoi(const wchar_t* str) { return (int)wcstol(str, nullptr, 10); }
#endif

// iswascii: MSVC CRT extension, no direct POSIX equivalent needed - a
// wide char is ASCII iff its value fits in 7 bits.
#ifndef iswascii
#define iswascii(c) ((unsigned int)(c) <= 0x7F)
#endif

// MultiByteToWideChar
#define CP_ACP 0
#define MultiByteToWideChar(cp, flags, mbstr, cb, wcstr, cch) mbstowcs(wcstr, mbstr, cch)
#define WideCharToMultiByte(cp, flags, wcstr, cch, mbstr, cb, defchar, used) wcstombs(mbstr, wcstr, cb)

