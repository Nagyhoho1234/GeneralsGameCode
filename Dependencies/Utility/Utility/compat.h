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

// This file contains macros to help compiling on non-windows platforms.
#pragma once

#ifndef _WIN32
// For size_t
#include <cstddef>
// For isdigit
#include <cctype>
// For std::isnan, aliased as _isnan below
#include <cmath>
// For sprintf, used by itoa below
#include <cstdio>

// itoa is a non-standard MSVC function; this codebase only ever calls it
// with base 10 (native port plan Phase 1).
#ifndef itoa
inline char* itoa(int value, char* str, int base)
{
	sprintf(str, "%d", value);
	return str;
}
#endif

// __max / __min are MSVC's old CRT macros (superseded by std::max/min,
// but still used directly across this codebase).
#ifndef __max
#define __max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef __min
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#endif

// _isnan is MSVC's pre-C++11 name for isnan(); many call sites across this
// codebase still use it directly (native port plan Phase 1).
#ifndef _isnan
#define _isnan(x) std::isnan(x)
#endif

// __forceinline
#ifndef __forceinline
#if defined __has_attribute && __has_attribute(always_inline)
#define __forceinline __attribute__((always_inline)) inline
#else
#define __forceinline inline
#endif
#endif

// _cdecl / __cdecl
#ifndef _cdecl
#define _cdecl
#endif
#ifndef __cdecl
#define __cdecl
#endif

// OutputDebugString
#ifndef OutputDebugString
#define OutputDebugString(str) printf("%s\n", str)
#endif

// _access (existence/permission check) and CreateDirectory - used across
// this codebase with the "path, unused-second-arg" call shape.
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#ifndef _access
#define _access access
#endif
#ifndef CreateDirectory
inline int CreateDirectory(const char* path, void*)
{
	if (mkdir(path, 0755) == 0)
		return 1;
	return errno == EEXIST ? 1 : 0;
}
#endif

// __int64 is MSVC's pre-C++11 64-bit integer name. wwprofile.h already
// has an identical `#ifdef _UNIX` typedef for this (pre-existing, not
// added by this port) - a plain typedef, not a macro, since a macro
// substitutes into token position wherever __int64 appears, including
// wwprofile.h's own `typedef ... __int64;` name, corrupting it into
// `typedef ... long long long long;` (a real regression this exact
// change caused and had to fix, native port plan Phase 1 Draft 11).
// Redeclaring an identical typedef is legal in C++, so this is safe
// regardless of include order relative to wwprofile.h.
typedef signed long long __int64;

// VK_RETURN: the only VK_* (virtual-key) constant this codebase uses
// anywhere (KeyboardOptionsMenu.cpp/GadgetTextEntry.cpp, both trees) -
// unlike DIK_* scan codes, no lookup table is needed.
#ifndef VK_RETURN
#define VK_RETURN 0x0D
#endif

// GetDoubleClickTime: matches the existing GlobalData.cpp precedent
// (Windows queries the OS setting; non-Windows already hardcodes 500
// there). This is a second, independent call site predating that fix.
#ifndef GetDoubleClickTime
inline unsigned int GetDoubleClickTime() { return 500; }
#endif

// DeleteFile / CopyFile - Win32 BOOL-return semantics (nonzero success,
// 0 failure), used directly by ReplayMenu.cpp/PopupReplay.cpp/Recorder.cpp.
#ifndef DeleteFile
inline int DeleteFile(const char* path)
{
	return remove(path) == 0 ? 1 : 0;
}
#endif
#ifndef CopyFile
inline int CopyFile(const char* src, const char* dst, int failIfExists)
{
	if (failIfExists)
	{
		FILE* existing = fopen(dst, "rb");
		if (existing)
		{
			fclose(existing);
			return 0;
		}
	}
	FILE* in = fopen(src, "rb");
	if (!in)
		return 0;
	FILE* out = fopen(dst, "wb");
	if (!out)
	{
		fclose(in);
		return 0;
	}
	char buf[8192];
	size_t n;
	int ok = 1;
	while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
	{
		if (fwrite(buf, 1, n, out) != n)
		{
			ok = 0;
			break;
		}
	}
	fclose(in);
	fclose(out);
	return ok;
}
#endif

// GetLastError / FormatMessage family - errno-backed. Only ever used
// together in this codebase (GetLastError() as FormatMessage's
// messageId argument), so a strerror()-based equivalent is a faithful
// substitute without reimplementing Windows' message-table lookup.
#include <cstring>
#ifndef GetLastError
#define GetLastError() errno
#endif
#ifndef FORMAT_MESSAGE_FROM_SYSTEM
#define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000
#endif
#ifndef FormatMessage
inline void FormatMessage(unsigned int /*flags*/, const void* /*source*/, int messageId,
	unsigned int /*languageId*/, char* buffer, unsigned int size, void* /*args*/)
{
	strncpy(buffer, strerror(messageId), size);
	buffer[size - 1] = '\0';
}
#endif
#ifndef FormatMessageW
inline void FormatMessageW(unsigned int /*flags*/, const void* /*source*/, int messageId,
	unsigned int /*languageId*/, wchar_t* buffer, unsigned int size, void* /*args*/)
{
	const char* msg = strerror(messageId);
	unsigned int i = 0;
	for (; msg[i] != '\0' && i < size - 1; ++i)
		buffer[i] = (wchar_t)(unsigned char)msg[i];
	buffer[i] = L'\0';
}
#endif

// MEMORYSTATUS / GlobalMemoryStatus - diagnostics-only in this codebase
// (GameClient.cpp logs before/after asset-preload memory use), so this
// is zero risk to gameplay regardless of platform.
// Uses plain `unsigned int` rather than UnsignedInt: this header is
// included from Lib/BaseTypeCore.h before UnsignedInt itself is
// declared there, so that typedef isn't available yet at this point.
#if defined(__linux__)
#include <sys/sysinfo.h>
#elif defined(__APPLE__)
// <sys/sysinfo.h>/sysinfo() is Linux (glibc)-only - does not exist on
// macOS (confirmed via real macos-latest CI, not guessed). Physical
// RAM and swap have standard sysctl equivalents; available/free RAM
// would need the Mach host_statistics64() API - honestly stubbed to 0
// rather than guessed at without being able to verify the Mach API
// surface locally (this is diagnostics-only, so no behavioral risk).
#include <sys/sysctl.h>
#endif
#ifndef MEMORYSTATUS
struct MEMORYSTATUS
{
	unsigned int dwLength;
	unsigned int dwMemoryLoad;
	unsigned int dwTotalPhys;
	unsigned int dwAvailPhys;
	unsigned int dwTotalPageFile;
	unsigned int dwAvailPageFile;
	unsigned int dwTotalVirtual;
	unsigned int dwAvailVirtual;
};
inline void GlobalMemoryStatus(MEMORYSTATUS* out)
{
	out->dwLength = sizeof(MEMORYSTATUS);
#if defined(__linux__)
	struct sysinfo si;
	sysinfo(&si);
	out->dwMemoryLoad = si.totalram ? (unsigned int)(100ULL * (si.totalram - si.freeram) / si.totalram) : 0;
	out->dwTotalPhys = (unsigned int)(si.totalram * si.mem_unit);
	out->dwAvailPhys = (unsigned int)(si.freeram * si.mem_unit);
	out->dwTotalPageFile = (unsigned int)(si.totalswap * si.mem_unit);
	out->dwAvailPageFile = (unsigned int)(si.freeswap * si.mem_unit);
#elif defined(__APPLE__)
	uint64_t totalPhys = 0;
	size_t size = sizeof(totalPhys);
	sysctlbyname("hw.memsize", &totalPhys, &size, nullptr, 0);
	out->dwTotalPhys = (unsigned int)totalPhys;
	out->dwAvailPhys = 0; // would need Mach host_statistics64(), stubbed (see above)
	out->dwMemoryLoad = 0;

	struct xsw_usage swapUsage;
	size = sizeof(swapUsage);
	if (sysctlbyname("vm.swapusage", &swapUsage, &size, nullptr, 0) == 0)
	{
		out->dwTotalPageFile = (unsigned int)swapUsage.xsu_total;
		out->dwAvailPageFile = (unsigned int)swapUsage.xsu_avail;
	}
	else
	{
		out->dwTotalPageFile = 0;
		out->dwAvailPageFile = 0;
	}
#else
	out->dwMemoryLoad = 0;
	out->dwTotalPhys = 0;
	out->dwAvailPhys = 0;
	out->dwTotalPageFile = 0;
	out->dwAvailPageFile = 0;
#endif
	out->dwTotalVirtual = out->dwTotalPhys;
	out->dwAvailVirtual = out->dwAvailPhys;
}
#endif

// AddFontResource / RemoveFontResource - honest no-op stubs (native
// port plan Phase 1 Draft 11): GlobalLanguage.cpp loads custom
// per-process font files this way; real fonts on non-Windows come from
// Fontconfig (Phase 7), not implemented yet. AddFontResource returns
// success (nonzero) rather than 0/failure so GlobalLanguage::init()'s
// DEBUG_CRASH-on-failure check doesn't fire spuriously every run.
#ifndef AddFontResource
inline int AddFontResource(const char*) { return 1; }
#endif
#ifndef RemoveFontResource
inline void RemoveFontResource(const char*) {}
#endif

// _MAX_DRIVE, _MAX_DIR, _MAX_FNAME, _MAX_EXT, _MAX_PATH
#ifndef _MAX_DRIVE
#define _MAX_DRIVE 3
#endif
#ifndef _MAX_DIR
#define _MAX_DIR 256
#endif
#ifndef _MAX_FNAME
#define _MAX_FNAME 256
#endif
#ifndef _MAX_EXT
#define _MAX_EXT 256
#endif
#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif

// HINSTANCE / CComModule / SetWindowText / SetWindowTextW (native port plan
// Phase 5(a) Milestone 12, Draft 36): GameEngine.cpp's own constructor/
// destructor unconditionally call "_Module.Init(nullptr, ApplicationHInstance,
// nullptr);" / "_Module.Term();" (real ATL::CComModule usage, unguarded by
// _WIN32/ATL - this codebase's real WebBrowser class is the only genuinely
// ATL/COM-based thing GameEngine.cpp needs, and that dependency is guarded
// out separately, see GameEngine.cpp's own #ifdef _WIN32 guard on
// WebBrowser.h). A real compile spike confirmed these are the only two
// _Module members ever called (both from GameEngine.cpp only, grep-confirmed)
// and CComModule::Init()/Term() are genuinely both no-ops for this port's
// purposes (no in-process COM server registration happens or is needed on
// POSIX) - true, honest no-op shims, not behavior-changing stand-ins.
// updateWindowTitle() (GameEngine.cpp) similarly calls
// ::SetWindowText()/::SetWindowTextW() unconditionally, but only inside an
// "if (ApplicationHWnd)" guard - ApplicationHWnd stays permanently null on
// every harness that reaches this code today, so these two calls are
// link-live, provably runtime-dead no-ops for this port's current call
// paths, exactly like timeBeginPeriod()'s shim below.
//
// win32_compat.h (this same directory) is the real owner of HWND's own
// typedef - included explicitly here (its own #ifndef WIN32_COMPAT_H guard
// makes repeat inclusion from elsewhere in this TU's own include graph
// safe) rather than assumed already-included, since compat.h's own
// inclusion order (via Lib/BaseTypeCore.h, ahead of WWLib/bittype.h in some
// translation units) is not guaranteed to have pulled it in yet.
#include "win32_compat.h"

#ifndef HINSTANCE
typedef void* HINSTANCE;
#endif

class CComModule
{
public:
	inline long Init(void* /*objectMap*/, HINSTANCE /*hInstance*/, const void* /*libId*/ = nullptr) { return 0; }
	inline void Term() {}
};

inline int SetWindowText(HWND /*hwnd*/, const char* /*title*/) { return 1; }
inline int SetWindowTextW(HWND /*hwnd*/, const wchar_t* /*title*/) { return 1; }

#include "mem_compat.h"
#include "string_compat.h"
#include "tchar_compat.h"
#include "wchar_compat.h"
#include "time_compat.h"
#include "thread_compat.h"

#endif

