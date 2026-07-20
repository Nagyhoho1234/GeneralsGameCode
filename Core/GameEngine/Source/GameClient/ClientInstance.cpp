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
#include "PreRTS.h"
#include "GameClient/ClientInstance.h"

#ifndef _WIN32
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define GENERALS_GUID "685EAFF2-3216-4265-B047-251C5F4B82F3"

namespace rts
{
#ifdef _WIN32
HANDLE ClientInstance::s_mutexHandle = nullptr;
#else
int ClientInstance::s_mutexHandle = -1;
#endif
UnsignedInt ClientInstance::s_instanceIndex = 0;

#if defined(RTS_MULTI_INSTANCE)
Bool ClientInstance::s_isMultiInstance = true;
#else
Bool ClientInstance::s_isMultiInstance = false;
#endif

#ifndef _WIN32
// tryLockInstance()'s three possible outcomes.
enum class LockResult { Acquired, HeldByOther, OpenFailed };
#endif

namespace
{
#ifndef _WIN32
// Per-user, non-world-writable directory for the lock file, in place of a
// shared, predictable path directly under /tmp (fable review: a fixed
// world-writable path is a symlink-squat surface, and 0666 masked by umask
// in a *shared* /tmp means a second user on the same machine could collide
// with the first user's lock - Windows' unnamed-prefix mutexes are
// per-session, not machine-wide, so this restores the same isolation).
// Prefers $XDG_RUNTIME_DIR (already per-user, 0700, tmpfs on most distros);
// falls back to a per-uid subdirectory of /tmp otherwise.
std::string getLockDir()
{
	if (const char* runtimeDir = getenv("XDG_RUNTIME_DIR"))
	{
		if (runtimeDir[0] != '\0')
			return runtimeDir;
	}

	char dir[64];
	snprintf(dir, sizeof(dir), "/tmp/genzh-%d", (int)getuid());
	mkdir(dir, 0700); // ignore EEXIST; ownership is checked implicitly by open() failing for other users
	return dir;
}

// POSIX equivalent of the named-Windows-mutex "is another instance already
// running" check (native port plan Phase 1): a non-blocking flock() on a
// well-known lock file. O_NOFOLLOW refuses to open a symlink planted at the
// expected path. Returns the open fd via *outFd on Acquired; the caller
// must distinguish HeldByOther (retry with a new instance index, matching
// GetLastError()==ERROR_ALREADY_EXISTS) from OpenFailed (a real error -
// e.g. an unwritable directory - which should abort like CreateMutex
// returning nullptr for a reason other than ERROR_ALREADY_EXISTS, not loop
// forever incrementing the instance index).
LockResult tryLockInstance(const char* name, int* outFd)
{
	std::string path = getLockDir();
	path += '/';
	path += name;
	path += ".lock";

	int fd = open(path.c_str(), O_CREAT | O_RDWR | O_NOFOLLOW, 0600);
	if (fd < 0)
		return LockResult::OpenFailed;
	if (flock(fd, LOCK_EX | LOCK_NB) != 0)
	{
		close(fd);
		return LockResult::HeldByOther;
	}
	*outFd = fd;
	return LockResult::Acquired;
}
#endif
}

bool ClientInstance::initialize()
{
	if (isInitialized())
	{
		return true;
	}

	// Create a mutex with a unique name to Generals in order to determine if our app is already running.
	// WARNING: DO NOT use this number for any other application except Generals.
	while (true)
	{
		if (isMultiInstance())
		{
			std::string guidStr = getFirstInstanceName();
			if (s_instanceIndex > 0u)
			{
				char idStr[33];
#ifdef _WIN32
				itoa(s_instanceIndex, idStr, 10);
#else
				snprintf(idStr, sizeof(idStr), "%u", s_instanceIndex);
#endif
				guidStr.push_back('-');
				guidStr.append(idStr);
			}
#ifdef _WIN32
			s_mutexHandle = CreateMutex(nullptr, FALSE, guidStr.c_str());
			if (GetLastError() == ERROR_ALREADY_EXISTS)
			{
				if (s_mutexHandle != nullptr)
				{
					CloseHandle(s_mutexHandle);
					s_mutexHandle = nullptr;
				}
				// Try again with a new instance.
				++s_instanceIndex;
				continue;
			}
#else
			int fd = -1;
			LockResult result = tryLockInstance(guidStr.c_str(), &fd);
			if (result == LockResult::HeldByOther)
			{
				// Try again with a new instance.
				++s_instanceIndex;
				continue;
			}
			if (result == LockResult::OpenFailed)
			{
				// A real error (unwritable lock directory, etc.) - not
				// "already running", so don't loop forever retrying.
				return false;
			}
			s_mutexHandle = fd;
#endif
		}
		else
		{
#ifdef _WIN32
			s_mutexHandle = CreateMutex(nullptr, FALSE, getFirstInstanceName());
			if (GetLastError() == ERROR_ALREADY_EXISTS)
			{
				if (s_mutexHandle != nullptr)
				{
					CloseHandle(s_mutexHandle);
					s_mutexHandle = nullptr;
				}
				return false;
			}
#else
			int fd = -1;
			if (tryLockInstance(getFirstInstanceName(), &fd) != LockResult::Acquired)
			{
				return false;
			}
			s_mutexHandle = fd;
#endif
		}
		break;
	}

	return true;
}

bool ClientInstance::isInitialized()
{
#ifdef _WIN32
	return s_mutexHandle != nullptr;
#else
	return s_mutexHandle >= 0;
#endif
}

bool ClientInstance::isMultiInstance()
{
	return s_isMultiInstance;
}

void ClientInstance::setMultiInstance(bool v)
{
	if (isInitialized())
	{
		DEBUG_CRASH(("ClientInstance::setMultiInstance(%d) - cannot set multi instance after initialization", (int)v));
		return;
	}
	s_isMultiInstance = v;
}

void ClientInstance::skipPrimaryInstance()
{
	if (isInitialized())
	{
		DEBUG_CRASH(("ClientInstance::skipPrimaryInstance() - cannot skip primary instance after initialization"));
		return;
	}
	s_instanceIndex = 1;
}

UnsignedInt ClientInstance::getInstanceIndex()
{
	DEBUG_ASSERTLOG(isInitialized(), ("ClientInstance::isInitialized() failed"));
	return s_instanceIndex;
}

UnsignedInt ClientInstance::getInstanceId()
{
	return getInstanceIndex() + 1;
}

const char* ClientInstance::getFirstInstanceName()
{
	return GENERALS_GUID;
}

} // namespace rts
