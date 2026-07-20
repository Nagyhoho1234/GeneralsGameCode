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

// This file contains the time functions for compatibility with non-windows platforms.
#pragma once
#include <time.h>
#include <sys/time.h>

#define TIMERR_NOERROR 0
typedef int MMRESULT;
static inline MMRESULT timeBeginPeriod(int) { return TIMERR_NOERROR; }
static inline MMRESULT timeEndPeriod(int) { return TIMERR_NOERROR; }

inline unsigned int timeGetTime()
{
  struct timespec ts;
  // CLOCK_BOOTTIME is Linux-specific (not POSIX, not available on
  // macOS/BSD - confirmed via real macOS CI, a pre-existing gap not
  // introduced by this port). CLOCK_MONOTONIC is the portable
  // fallback; it lacks Linux's distinction of counting through
  // suspend, which doesn't matter for this function's actual use
  // (frame/network timing, not wall-clock-since-boot display).
#ifdef CLOCK_BOOTTIME
  clock_gettime(CLOCK_BOOTTIME, &ts);
#else
  clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
inline unsigned int GetTickCount()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  // Return ms since boot
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// SYSTEMTIME / GetLocalTime (native port plan Phase 1). Field layout
// matches Windows' SYSTEMTIME so Recorder.cpp's binary replay-header
// serialization (raw byte read/write of this struct) stays compatible.
struct SYSTEMTIME
{
	unsigned short wYear;
	unsigned short wMonth;
	unsigned short wDayOfWeek;
	unsigned short wDay;
	unsigned short wHour;
	unsigned short wMinute;
	unsigned short wSecond;
	unsigned short wMilliseconds;
};

inline void GetLocalTime(SYSTEMTIME* out)
{
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	struct tm local;
	localtime_r(&tv.tv_sec, &local);
	out->wYear = (unsigned short)(local.tm_year + 1900);
	out->wMonth = (unsigned short)(local.tm_mon + 1);
	out->wDayOfWeek = (unsigned short)local.tm_wday;
	out->wDay = (unsigned short)local.tm_mday;
	out->wHour = (unsigned short)local.tm_hour;
	out->wMinute = (unsigned short)local.tm_min;
	out->wSecond = (unsigned short)local.tm_sec;
	out->wMilliseconds = (unsigned short)(tv.tv_usec / 1000);
}

// LARGE_INTEGER / QueryPerformanceCounter / QueryPerformanceFrequency
// (native port plan Phase 1 Draft 11): Network.cpp is the only file in
// this codebase that calls these unconditionally (everywhere else -
// PerfTimer.cpp, FrameRateLimit.cpp, W3DDevice's timing code, etc. -
// they're already gated behind #ifdef _WIN32). Network.cpp only ever
// reinterprets a __int64 as a LARGE_INTEGER via pointer cast (never
// touches a .QuadPart member), so a plain same-size typedef is
// sufficient - no union needed.
typedef __int64 LARGE_INTEGER;

inline int QueryPerformanceCounter(LARGE_INTEGER* out)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	*out = (LARGE_INTEGER)ts.tv_sec * 1000000000LL + ts.tv_nsec;
	return 1;
}

inline int QueryPerformanceFrequency(LARGE_INTEGER* out)
{
	*out = 1000000000LL; // this codebase's QueryPerformanceCounter ticks in ns
	return 1;
}

