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

// This file contains thread related functions for compatibility with non-windows platforms.
#pragma once
#include <pthread.h>
#include <unistd.h>

inline int GetCurrentThreadId()
{
  // pthread_self() returns an integer-like pthread_t on Linux (implicit
  // conversion to int is fine, if narrowing), but an opaque pointer on
  // macOS/BSD, which cannot convert to int at all (confirmed via real
  // macOS CI - a pre-existing gap, not introduced by this port).
  // pthread_mach_thread_np() is macOS's actual small-integer thread ID.
#if defined(__APPLE__)
  return (int)pthread_mach_thread_np(pthread_self());
#else
  return (int)pthread_self();
#endif
}

inline void Sleep(int ms)
{
  usleep(ms * 1000);
}

