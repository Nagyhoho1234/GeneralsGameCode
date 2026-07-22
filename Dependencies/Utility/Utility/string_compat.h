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

// This file contains string macros and alias functions to help compiling on non-windows platforms
#pragma once
#include <ctype.h>
#include <cstring>
#include <cstdlib>

typedef const char* LPCSTR;
typedef char* LPSTR;

// String functions
// extern "C" linkage (native port plan Phase 1 Draft 11): the vendored
// GameSpy SDK (_deps/gamespy-src/include/gamespy/gsplatform.h) declares
// its own `char* _strlwr(char*)` inside an `extern "C" {}` block on
// non-Windows, expecting this codebase to provide the definition (no
// libc equivalent exists off Windows). A plain C++-linkage inline
// definition here conflicted with that declaration wherever both
// headers reach the same translation unit.
extern "C" inline char *_strlwr(char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    str[i] = tolower(str[i]);
  }
  return str;
}

#define strlwr _strlwr
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define strnicmp strncasecmp
#define strcmpi strcasecmp

// Win32 kernel32.dll "lstr*" string functions + MSVC CRT's _strdup (native
// port plan Phase 5(a) Milestone 5, Draft 24 Step 5) - real Windows API/CRT
// functions with no POSIX equivalent name, first needed once hlod.cpp/
// rendobj.cpp joined the portable build. lstrcpy/lstrlen/lstrcmpi/_strdup
// are direct aliases (case-insensitive-compare and string-length semantics
// match exactly); lstrcpyn is NOT a strncpy alias - unlike strncpy, real
// lstrcpyn always null-terminates the destination and never pads beyond
// it, so it gets a real definition instead of a macro.
#define lstrcpy strcpy
#define lstrlen strlen
#define lstrcmpi strcasecmp
#define _strdup strdup

inline char* lstrcpyn(char* dest, const char* src, int max_length) {
  if (max_length <= 0) return dest;
  int i = 0;
  for (; i < max_length - 1 && src[i] != '\0'; ++i) {
    dest[i] = src[i];
  }
  dest[i] = '\0';
  return dest;
}

