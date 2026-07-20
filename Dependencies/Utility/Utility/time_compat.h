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
  clock_gettime(CLOCK_BOOTTIME, &ts);
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

