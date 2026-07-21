/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
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

/* $Header: /G/wwlib/bittype.h 4     4/02/99 1:37p Eric_c $ */
/***************************************************************************
 ***                  Confidential - Westwood Studios                    ***
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Voxel Technology                         *
 *                                                                         *
 *                    File Name : BITTYPE.h                                *
 *                                                                         *
 *                   Programmer : Greg Hjelstrom                           *
 *                                                                         *
 *                   Start Date : 02/24/97                                 *
 *                                                                         *
 *                  Last Update : February 24, 1997 [GH]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#define WWLIB_BITTYPE_H

typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned long	uint32;
typedef unsigned int    uint;

typedef signed char		sint8;
typedef signed short		sint16;
typedef signed long		sint32;
typedef signed int      sint;

typedef float				float32;
typedef double				float64;

// DWORD/WORD/BYTE/BOOL/UINT/ULONG also get typedef'd by Utility/win32_compat.h
// (added for the Phase 5 native port, see PortableD3D8/) - whichever header
// is included first wins so the two never fight over the same names.
//
// DWORD/ULONG's underlying type must match whatever <windows.h> itself uses
// on a given platform, not just its width - a typedef redeclared with a
// different (even same-width) underlying type is a hard C2371 error in
// MSVC, which is exactly what happened when this was first changed to a
// blanket `unsigned int`: real <windows.h> (minwindef.h) defines DWORD/
// ULONG as `unsigned long` (32-bit there, since Win32 is LLP64), so on
// Windows this must stay `unsigned long` to stay compatible with whichever
// of the two headers a given Windows TU happens to include second. On
// 64-bit Linux/macOS (LP64) `unsigned long` is 64-bit, which silently
// diverged from win32_compat.h's `uint32_t` DWORD/ULONG there (Phase 5(a)
// Milestone 2, Draft 18 flagged this as "a watch item, not a demonstrated
// bug" pending a reproduction; Milestone 2 Step 7's RenderTexturedTriangle
// harness hit it for real - a struct with a DWORD member got a different
// sizeof/layout depending on which of these two headers a given
// translation unit happened to include first, corrupting vertex-buffer
// offset math computed in one TU against vertex data laid out in another) -
// `unsigned int` is correct there, matching win32_compat.h exactly.
#ifndef WIN32_COMPAT_H
#ifdef _WIN32
typedef unsigned long   DWORD;
typedef unsigned long   ULONG;
#else
typedef unsigned int    DWORD;
typedef unsigned int    ULONG;
#endif
typedef unsigned short	WORD;
typedef unsigned char   BYTE;
typedef int             BOOL;
typedef unsigned int    UINT;
#endif
typedef unsigned short	USHORT;
typedef const char *		LPCSTR;

#if defined(_MSC_VER) && _MSC_VER < 1300
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#endif
