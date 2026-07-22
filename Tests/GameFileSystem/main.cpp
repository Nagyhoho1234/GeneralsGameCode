// Phase 5(a) Milestone 7 Task 2 (native port plan, Draft 28) - the headless
// file-system harness: the FIRST GameEngine code (not WW3D2 rendering code)
// ever to RUN, not merely compile, on POSIX. See docs/native-port-plan.md's
// Draft 28 section for the full design rationale (findings 1, 2, 4, 5, 6,
// 7; design decisions; open questions).
//
// Prologue mirrors WinMain.cpp:875-882 exactly: five real CriticalSection
// objects wired to the five global pointers, THEN initMemoryManager() -
// GameEngine file-system TUs allocate via newInstance/MSGNEW, which need
// the real memory manager (and AsciiString/UnicodeString/Debug all touch
// their CriticalSection globals on nearly every call). Then construction
// follows GUIEdit.cpp's real, shipped precedent (Core/Tools/GUIEdit/
// Source/GUIEdit.cpp:478-483 - the same file-system spine GameEngine::
// init() runs at GameEngine.cpp:403-445, minus the subsystem list, which
// is out of scope for this harness): TheFileSystem = new FileSystem;
// TheLocalFileSystem = new StdLocalFileSystem; TheArchiveFileSystem =
// new StdBIGFileSystem; TheFileSystem->init() (which cascades into both).
// TheSubsystemList/TheAudio/TheWritableGlobalData stay harness-local link
// stubs (link_stubs.cpp) - see that file for exactly why each is safe to
// leave null.
//
// Six checks (design decisions + finding 2 give the semantics to prove,
// not assume - see docs/native-port-plan.md's Draft 28 "Implementation
// ordering" step 2 for the authoritative list):
//   1. Windows-style path fixup - open a file by a backslashed, wrong-case
//      name against a mixed-case on-disk tree, byte-compare.
//   2. Directory listing - masks, subdirectories, getFileInfo sizes.
//   3. Author a real multi-file .big at runtime (nested paths, '\'
//      separators, big-endian directory fields per finding 7's exact
//      format spec) and verify the loaded directory tree matches what was
//      authored.
//   4. Archive reads byte-exact via BOTH RAMFile (plain reads) and
//      File::STREAMING (StreamingArchiveFile) - StdBIGFile::openFile
//      routes between the two based on the open mode.
//   5. Local-shadows-archive precedence, both directions: a file that
//      exists only in the archive is found; a file that exists in both
//      places resolves to the local copy.
//   6. Clean teardown, real ctest exit 0, stable across 5+ consecutive
//      runs (verified externally by re-invoking ctest/the binary
//      repeatedly, not by looping inside this process - matching this
//      port's standing stability-verification convention).
#include "PreRTS.h"

#include "Common/AsciiString.h"
#include "Common/CriticalSection.h"
#include "Common/file.h"
#include "Common/FileSystem.h"
#include "Common/LocalFileSystem.h"
#include "Common/ArchiveFileSystem.h"
#include "Common/RAMFile.h"
#include "Common/StreamingArchiveFile.h"
#include "Common/GameMemory.h"
#include "StdDevice/Common/StdLocalFileSystem.h"
#include "StdDevice/Common/StdBIGFileSystem.h"

#include <Utility/endian_compat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace
{
	bool g_AnyFailure = false;

	void Fail(const char* what)
	{
		fprintf(stderr, "GAMEFILESYSTEM_FAIL: %s\n", what);
		g_AnyFailure = true;
	}

	void Check(bool ok, const char* what)
	{
		if (ok) {
			printf("  %s: OK\n", what);
		} else {
			fprintf(stderr, "  %s: FAILED\n", what);
			g_AnyFailure = true;
		}
	}

	// ---- Plain-C++ authoring helpers - deliberately NOT using the engine's
	// own File/FileSystem classes to write these bytes (Draft 24 precedent:
	// the harness authors ground truth independently of the code under
	// test, the same way Tests/RenderWW3DFrame authors .tga/.w3d bytes with
	// plain fopen/fwrite before ever handing them to the real asset
	// manager). ----

	void WriteWholeFile(const fs::path& path, const void* data, size_t size)
	{
		fs::create_directories(path.parent_path());
		FILE* f = fopen(path.string().c_str(), "wb");
		if (!f) { Fail("could not create authoring file"); return; }
		if (size > 0) fwrite(data, 1, size, f);
		fclose(f);
	}

	void WriteWholeFile(const fs::path& path, const std::string& content)
	{
		WriteWholeFile(path, content.data(), content.size());
	}

	struct ArchiveEntry
	{
		std::string path; // '\' separators, matching finding 7's real format.
		std::string data;
	};

	// Authors a real .big archive matching StdBIGFileSystem::openArchiveFile's
	// exact consumption format (StdBIGFileSystem.cpp:81-180, "finding 7"):
	// "BIGF" magic, archive size (read raw and unused by the reader - written
	// here in native order since it is genuinely never validated), entry
	// count (big-endian), directory starting at seek 0x10 (4 bytes of
	// reserved padding after the 12-byte fixed header), then per entry:
	// offset (big-endian), size (big-endian), nul-terminated path.
	void AuthorBigArchive(const fs::path& outPath, const std::vector<ArchiveEntry>& entries)
	{
		uint32_t dirSize = 0;
		for (const auto& e : entries) dirSize += 4 + 4 + static_cast<uint32_t>(e.path.size() + 1);

		const uint32_t headerSize = 0x10;
		const uint32_t dataStart = headerSize + dirSize;

		std::vector<uint32_t> offsets(entries.size());
		uint32_t cursor = dataStart;
		for (size_t i = 0; i < entries.size(); ++i)
		{
			offsets[i] = cursor;
			cursor += static_cast<uint32_t>(entries[i].data.size());
		}
		const uint32_t archiveSize = cursor;

		FILE* f = fopen(outPath.string().c_str(), "wb");
		if (!f) { Fail("could not create .big archive file"); return; }

		fwrite("BIGF", 1, 4, f);
		fwrite(&archiveSize, 4, 1, f); // read raw and unused by the reader.
		uint32_t countBE = htobe(static_cast<uint32_t>(entries.size()));
		fwrite(&countBE, 4, 1, f);
		uint32_t reserved = 0;
		fwrite(&reserved, 4, 1, f); // pads the fixed header out to seek 0x10.

		for (size_t i = 0; i < entries.size(); ++i)
		{
			uint32_t offBE = htobe(offsets[i]);
			uint32_t sizeBE = htobe(static_cast<uint32_t>(entries[i].data.size()));
			fwrite(&offBE, 4, 1, f);
			fwrite(&sizeBE, 4, 1, f);
			fwrite(entries[i].path.c_str(), 1, entries[i].path.size() + 1, f); // incl. '\0'.
		}
		for (const auto& e : entries)
		{
			if (!e.data.empty()) fwrite(e.data.data(), 1, e.data.size(), f);
		}

		fclose(f);
	}

	// Reads an entire engine File* to a std::string via the polymorphic
	// read()/size()/close() contract (file.h:142-176), then closes it (which
	// self-deletes File objects opened with deleteOnClose(), the contract
	// every RAMFile/StreamingArchiveFile StdBIGFile::openFile hands back
	// (StdBIGFile.cpp:76) relies on). Deliberately NOT
	// File::readEntireAndClose() - that entry point is a RAMFile-only
	// shortcut (RAMFile::readEntireAndClose(), RAMFile.cpp:551-566, returns
	// RAMFile's private m_data buffer directly); StreamingArchiveFile
	// inherits it unchanged from RAMFile but never populates m_data (it
	// streams lazily through its own m_file/m_curPos instead, StreamingArchiveFile.cpp:215-233),
	// so calling it on a StreamingArchiveFile hits RAMFile::readEntireAndClose's
	// own "m_data is null" guard and silently returns a throwaway 1-byte
	// buffer - a real bug this harness found in check 4 by using the wrong
	// entry point, not a StreamingArchiveFile defect. The loop below is the
	// one read path guaranteed correct for all three concrete File
	// subclasses this harness exercises (StdLocalFile/RAMFile/
	// StreamingArchiveFile), and is a more general proof of check 4's point
	// besides.
	std::string ReadEntireFile(File* file)
	{
		if (!file) return std::string();
		int total = file->size();
		std::string result;
		result.resize(static_cast<size_t>(total));
		int done = 0;
		while (done < total)
		{
			int n = file->read(&result[done], total - done);
			if (n <= 0) break;
			done += n;
		}
		result.resize(static_cast<size_t>(done));
		file->close(); // self-deletes (deleteOnClose()) - file is invalid after this call.
		return result;
	}

	void RemoveIfExists(const fs::path& p)
	{
		std::error_code ec;
		fs::remove_all(p, ec);
	}

	// All six checks and every local (std::string content buffers,
	// ArchiveEntry vectors, ...) they need, in one function called from
	// main() BEFORE shutdownMemoryManager() - deliberately, not
	// incidentally. GameMemory.cpp overrides the process-global operator
	// new/delete to route through TheDynamicMemoryAllocator; any local
	// object with a non-trivial destructor still in scope when main()
	// returns would free its storage AFTER shutdownMemoryManager() has torn
	// that allocator down, exactly the class of crash real WinMain.cpp
	// avoids by explicitly deleting every heap object it owns (TheVersion,
	// WinMain.cpp:988-989) before its own shutdownMemoryManager() call
	// (:1000) - this harness's first draft missed that discipline (every
	// std::string content buffer below was a local of main() itself) and a
	// real run caught it: SIGSEGV inside DynamicMemoryAllocator::freeBytes,
	// GameMemory.cpp:2316, during a std::string destructor running after
	// main()'s closing brace. Confirmed via gdb backtrace, not guessed at.
	void RunAllChecksThenTearDownFileSystems()
	{
	// ---- Stability across repeated runs (check 6's "5+ consecutive runs"
	// bar): wipe every path this harness authors BEFORE constructing the
	// archive file system, so its own init()-time cwd scan
	// (StdBIGFileSystem::init -> loadBigFilesFromDirectory("", "*.big"),
	// finding 2) never picks up a stale archive left by a prior run. ----
	RemoveIfExists("MixedCaseTree");
	RemoveIfExists("Listing");
	RemoveIfExists("Data");
	RemoveIfExists("TestAssets.big");

	// ---- Author the on-disk trees checks 1/2/5 need, BEFORE constructing
	// any file-system object (LocalFileSystem has no cached state - order
	// doesn't matter functionally, but authoring first keeps the sequence
	// closest to a real test's arrange/act/assert shape). ----

	// Check 1's mixed-case tree: actual on-disk casing is all-lowercase;
	// the request below is title-case with backslashes - full mismatch on
	// every path segment, exercising fixFilenameFromWindowsPath's
	// case-insensitive directory_iterator walk (StdLocalFileSystem.cpp:
	// 45-131) on every component, not just the leaf.
	const std::string kFixupContent = "PATH_FIXUP_OK - real bytes from a mixed-case on-disk tree\n";
	WriteWholeFile("MixedCaseTree/data/ini/foo.ini", kFixupContent);

	// Check 2's listing tree: masks (*.ini vs .txt), a subdirectory, and
	// known byte counts for getFileInfo.
	const std::string kGameDataContent = "; GameData.ini - 42 known bytes for getFileInfo.\n";
	const std::string kExtraContent = "NestedExtra.ini content, distinct from GameData's.\n";
	const std::string kOtherContent = "Other.txt must NOT match a *.ini mask search.\n";
	WriteWholeFile("Listing/GameData.ini", kGameDataContent);
	WriteWholeFile("Listing/Sub/Extra.ini", kExtraContent);
	WriteWholeFile("Listing/Other.txt", kOtherContent);

	// Check 5's local half: Shadowed.ini exists locally with THIS content;
	// the archive (authored below) will contain a file at the exact same
	// logical path with DIFFERENT content, so FileSystem::openFile's
	// local-first precedence (FileSystem.cpp:175-220) has something real
	// to prove.
	const std::string kShadowedLocalContent = "LOCAL COPY WINS - this is the on-disk version.\n";
	WriteWholeFile("Data/INI/Shadowed.ini", kShadowedLocalContent);

	// ---- Construct the file-system stack, in GameEngine::init()'s own
	// spine order (GameEngine.cpp:403-445, minus the subsystem list) - the
	// same real sequence GUIEdit.cpp uses (GUIEdit.cpp:478-483) with the
	// Std twins swapped in for Win32's. ----
	TheFileSystem = new FileSystem;
	TheLocalFileSystem = new StdLocalFileSystem;
	TheArchiveFileSystem = new StdBIGFileSystem;
	TheFileSystem->init(); // cascades into TheLocalFileSystem->init() and
	                        // TheArchiveFileSystem->init() (FileSystem.cpp:
	                        // 143-147) - StdBIGFileSystem::init() asserts
	                        // TheLocalFileSystem is already up (finding 2)
	                        // and scans cwd for "*.big" (none exist yet,
	                        // by design - see the RemoveIfExists calls
	                        // above).

	printf("=== Check 1: Windows-style path fixup ===\n");
	{
		File* f = TheFileSystem->openFile("MixedCaseTree\\Data\\INI\\Foo.ini", File::READ | File::BINARY);
		Check(f != nullptr, "1a. openFile(\"MixedCaseTree\\\\Data\\\\INI\\\\Foo.ini\") against lowercase on-disk tree succeeded");
		if (f)
		{
			std::string content = ReadEntireFile(f);
			Check(content == kFixupContent, "1b. byte-exact content via the case-insensitive fixup path");
		}
		else
		{
			Fail("1b. skipped (open failed)");
		}

		// Negative control: a genuinely absent file must still fail (proves
		// the fixup isn't silently matching anything).
		File* missing = TheFileSystem->openFile("MixedCaseTree\\Data\\INI\\DoesNotExist.ini", File::READ | File::BINARY);
		Check(missing == nullptr, "1c. negative control - genuinely absent file still fails to open");
		if (missing) missing->close();
	}

	printf("=== Check 2: directory listing (masks/subdirectories/getFileInfo) ===\n");
	{
		// Real convention, confirmed against INI::loadDirectory (INI.cpp:
		// 233-243, "dirName.concat('\\'); TheFileSystem->getFileListInDirectory
		// (dirName, ...)") - StdLocalFileSystem::getFileListInDirectory's
		// recursive case concatenates originalDirectory+currentDirectory with
		// NO separator inserted (StdLocalFileSystem.cpp:219-221), so the
		// caller must supply the trailing separator itself.
		FilenameList iniFiles;
		TheFileSystem->getFileListInDirectory("Listing\\", "*.ini", iniFiles, TRUE);

		size_t iniMatches = 0, txtLeaked = 0;
		for (const AsciiString& name : iniFiles)
		{
			if (name.endsWithNoCase(".ini")) ++iniMatches;
			if (name.endsWithNoCase(".txt")) ++txtLeaked;
		}
		Check(iniMatches == 2, "2a. *.ini mask + searchSubdirectories finds both GameData.ini and Sub/Extra.ini");
		Check(txtLeaked == 0, "2b. *.ini mask correctly excludes Other.txt");

		bool foundGameData = false, foundExtra = false;
		for (const AsciiString& name : iniFiles)
		{
			if (name.endsWithNoCase("GameData.ini")) foundGameData = true;
			if (name.endsWithNoCase("Extra.ini")) foundExtra = true;
		}
		Check(foundGameData, "2c. top-level GameData.ini present by name");
		Check(foundExtra, "2d. subdirectory Sub/Extra.ini present by name (recursion proven)");

		FileInfo info;
		Bool gotInfo = TheFileSystem->getFileInfo("Listing/GameData.ini", &info);
		Check(gotInfo == TRUE, "2e. getFileInfo succeeded for Listing/GameData.ini");
		if (gotInfo)
		{
			Check(static_cast<size_t>(info.sizeLow) == kGameDataContent.size(),
				"2f. getFileInfo size matches the authored byte count exactly");
		}
		else
		{
			Fail("2f. skipped (getFileInfo failed)");
		}
	}

	printf("=== Check 3: author a real .big archive, verify the loaded directory tree ===\n");
	const std::string kArchiveOnlyContent = "ARCHIVE_ONLY content - never exists on local disk.\n";
	const std::string kShadowedArchiveContent = "ARCHIVE COPY LOSES - local must win over this.\n";
	const std::string kNestedContent = "Nested/Sub content inside the archive, proves tree depth.\n";
	{
		std::vector<ArchiveEntry> entries = {
			{ "Data\\INI\\ArchiveOnly.ini", kArchiveOnlyContent },
			{ "Data\\INI\\Shadowed.ini",    kShadowedArchiveContent },
			{ "Data\\INI\\Sub\\Nested.txt", kNestedContent },
		};
		AuthorBigArchive("TestAssets.big", entries);

		Bool loaded = TheArchiveFileSystem->loadBigFilesFromDirectory("", "*.big");
		Check(loaded == TRUE, "3a. loadBigFilesFromDirectory found and loaded TestAssets.big");

		ArchivedDirectoryInfo* dirInfo = TheArchiveFileSystem->friend_getArchivedDirectoryInfo("Data\\INI");
		Check(dirInfo != nullptr, "3b. Data\\INI directory node exists in the loaded tree");
		if (dirInfo)
		{
			Check(dirInfo->m_files.find(AsciiString("archiveonly.ini")) != dirInfo->m_files.end(),
				"3c. Data\\INI directory tree contains archiveonly.ini");
			Check(dirInfo->m_files.find(AsciiString("shadowed.ini")) != dirInfo->m_files.end(),
				"3d. Data\\INI directory tree contains shadowed.ini");
			Check(dirInfo->m_directories.find(AsciiString("sub")) != dirInfo->m_directories.end(),
				"3e. Data\\INI\\Sub nested directory node exists (tree depth proven)");
		}
		else
		{
			Fail("3c/3d/3e. skipped (no directory node)");
		}

		ArchivedDirectoryInfo* subDirInfo = TheArchiveFileSystem->friend_getArchivedDirectoryInfo("Data\\INI\\Sub");
		Check(subDirInfo != nullptr && subDirInfo->m_files.find(AsciiString("nested.txt")) != subDirInfo->m_files.end(),
			"3f. Data\\INI\\Sub directory tree contains nested.txt");

		Check(TheArchiveFileSystem->doesFileExist("Data\\INI\\ArchiveOnly.ini") == TRUE,
			"3g. ArchiveFileSystem::doesFileExist confirms ArchiveOnly.ini is reachable");
	}

	printf("=== Check 4: archive reads byte-exact via RAMFile AND File::STREAMING ===\n");
	{
		// Plain read - access has no STREAMING bit, StdBIGFile::openFile
		// (StdBIGFile.cpp:61-101) takes the RAMFile branch.
		File* ramFile = TheArchiveFileSystem->openFile("Data\\INI\\ArchiveOnly.ini", File::READ | File::BINARY);
		Check(ramFile != nullptr, "4a. archive open (plain access) succeeded");
		if (ramFile)
		{
			Check(dynamic_cast<StreamingArchiveFile*>(ramFile) == nullptr,
				"4b. plain access really returned a RAMFile, not a StreamingArchiveFile");
			std::string content = ReadEntireFile(ramFile);
			Check(content == kArchiveOnlyContent, "4c. RAMFile path is byte-exact against the authored content");
		}
		else
		{
			Fail("4b/4c. skipped (open failed)");
		}

		// Streaming read - File::STREAMING set, takes the StreamingArchiveFile
		// branch (mutually exclusive with WRITE per file.h:103-104).
		File* streamFile = TheArchiveFileSystem->openFile("Data\\INI\\ArchiveOnly.ini", File::READ | File::BINARY | File::STREAMING);
		Check(streamFile != nullptr, "4d. archive open (File::STREAMING) succeeded");
		if (streamFile)
		{
			Check(dynamic_cast<StreamingArchiveFile*>(streamFile) != nullptr,
				"4e. File::STREAMING access really returned a StreamingArchiveFile");
			std::string content = ReadEntireFile(streamFile);
			Check(content == kArchiveOnlyContent, "4f. StreamingArchiveFile path is byte-exact against the same authored content");
		}
		else
		{
			Fail("4e/4f. skipped (open failed)");
		}
	}

	printf("=== Check 5: local-shadows-archive precedence, both directions ===\n");
	{
		// Direction A: exists only in the archive -> found via the
		// archive-fallback branch of FileSystem::openFile (FileSystem.cpp:
		// 213-217).
		File* archiveOnly = TheFileSystem->openFile("Data\\INI\\ArchiveOnly.ini", File::READ | File::BINARY);
		Check(archiveOnly != nullptr, "5a. archive-only file found through TheFileSystem (local absent, archive fallback taken)");
		if (archiveOnly)
		{
			std::string content = ReadEntireFile(archiveOnly);
			Check(content == kArchiveOnlyContent, "5b. archive-only content is byte-exact");
		}
		else
		{
			Fail("5b. skipped (open failed)");
		}

		// Direction B: exists in BOTH -> local wins (FileSystem.cpp:191's
		// TheLocalFileSystem->openFile is tried and satisfied first,
		// FileSystem.cpp:213's archive fallback never runs for this name).
		File* shadowed = TheFileSystem->openFile("Data\\INI\\Shadowed.ini", File::READ | File::BINARY);
		Check(shadowed != nullptr, "5c. shadowed (local+archive) file found through TheFileSystem");
		if (shadowed)
		{
			std::string content = ReadEntireFile(shadowed);
			Check(content == kShadowedLocalContent, "5d. local copy wins - content matches the on-disk version, not the archive's");
			Check(content != kShadowedArchiveContent, "5e. archive's differing content was NOT what got returned");
		}
		else
		{
			Fail("5d/5e. skipped (open failed)");
		}
	}

	printf("=== Check 6: clean teardown ===\n");
	{
		delete TheFileSystem;
		TheFileSystem = nullptr;
		delete TheLocalFileSystem;
		TheLocalFileSystem = nullptr;
		delete TheArchiveFileSystem;
		TheArchiveFileSystem = nullptr;
		Check(TheFileSystem == nullptr && TheLocalFileSystem == nullptr && TheArchiveFileSystem == nullptr,
			"6a. all three file-system singletons torn down and nulled");
	}
	} // RunAllChecksThenTearDownFileSystems - every local above is destroyed
	  // at this closing brace, before main() calls shutdownMemoryManager().
}

int main()
{
	// Line-buffer both streams so a crash mid-run (which this harness
	// deliberately does not paper over - see the report for what this
	// caught) still leaves every check result up to that point on disk,
	// not lost in a full-buffered stdio flush that never happens.
	setvbuf(stdout, nullptr, _IOLBF, 0);
	setvbuf(stderr, nullptr, _IOLBF, 0);

	// ---- Prologue: mirrors WinMain.cpp:875-882 exactly. -----------------
	static CriticalSection critSec1, critSec2, critSec3, critSec4, critSec5;
	TheAsciiStringCriticalSection = &critSec1;
	TheUnicodeStringCriticalSection = &critSec2;
	TheDmaCriticalSection = &critSec3;
	TheMemoryPoolCriticalSection = &critSec4;
	TheDebugLogCriticalSection = &critSec5;

	initMemoryManager();

	RunAllChecksThenTearDownFileSystems();

	// Mirrors WinMain.cpp:1000/1011-1013 - shutdownMemoryManager() and
	// clearing the CriticalSection globals are the last things that happen,
	// with no heap-backed local of this harness's own left alive above them.
	shutdownMemoryManager();

	TheAsciiStringCriticalSection = nullptr;
	TheUnicodeStringCriticalSection = nullptr;
	TheDmaCriticalSection = nullptr;
	TheMemoryPoolCriticalSection = nullptr;
	TheDebugLogCriticalSection = nullptr;

	if (g_AnyFailure)
	{
		fprintf(stderr, "GAMEFILESYSTEM_FAIL: one or more checks failed (see above)\n");
		return 1;
	}

	printf("GAMEFILESYSTEM_OK: all checks passed\n");
	return 0;
}
