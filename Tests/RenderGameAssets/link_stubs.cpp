// Harness-local link stubs (native port plan Phase 5(a) Milestone 7 Task 4,
// Draft 28 finding 5) - identical in kind to Tests/GameFileSystem/
// link_stubs.cpp, needed here for the same reason: this harness directly
// links the same curated slice of Core/GameEngine's Common/System layer
// (ArchiveFileSystem.cpp/FileSystem.cpp/GameMemory.cpp/etc.), which
// references these three global singletons. All three are genuinely
// unexercised at runtime by anything this harness calls; each stub below
// documents exactly why. Follows the established anim_sound_link_stub.cpp /
// Tests/GameFileSystem/link_stubs.cpp pattern: loud comments, no hidden
// behavior.

// ---- TheSubsystemList (SubsystemInterface.h/.cpp) --------------------------
// SubsystemInterface::SubsystemInterface()/~SubsystemInterface() (the base
// class LocalFileSystem/ArchiveFileSystem/FileSystem all derive from)
// null-guard every touch: "if (TheSubsystemList) { ... }". Real definition
// lives in the monolithic per-tree GameEngine.cpp - unacceptable to drag into
// this harness. This harness constructs its file-system objects directly
// (GUIEdit.cpp's real, shipped precedent), never via GameEngine::
// initSubsystem's SubsystemInterfaceList::initSubsystem path, so
// TheSubsystemList stays nullptr for this harness's entire lifetime - safe
// by the same null guard every other tool (WorldBuilder, GUIEdit, Autorun)
// already relies on.
#include "Common/SubsystemInterface.h"
SubsystemInterfaceList *TheSubsystemList = nullptr;

// ---- TheAudio (GameAudio.h, defined in GameAudio.cpp) ----------------------
// StdBIGFileSystem::closeArchiveFile calls TheAudio->stopAudio(...) only
// when closing an archive literally named "Music.big"
// (ArchiveFileSystem.h's MUSIC_BIG). This harness's own archive is never
// named that, so that call is unreached. Real definition lives in
// GameAudio.cpp, a heavy TU needing the Miles Sound System - out of scope
// (audio stays a null link-stub for this entire milestone per the plan's
// non-goals list).
#include "Common/GameAudio.h"
AudioManager *TheAudio = nullptr;

// ---- TheWritableGlobalData (GlobalData.h, defined in GlobalData.cpp) ------
// GameMemory.cpp references TheGlobalData, a read-only view/macro over
// TheWritableGlobalData (GlobalData.h:616-623), every call site already
// null-guarded. W3DFileSystem.cpp's Set_Name also checks
// "if (m_fileExists == FALSE && TheGlobalData)" before ever dereferencing it
// (W3DFileSystem.cpp:333,355) - both user-data and map-preview fallback
// lookups are skipped entirely with TheGlobalData null, which is correct for
// this harness (it never authors user-data/map-preview paths). Real
// definition lives in GlobalData.cpp, a per-tree TU needing the full
// INI-driven settings closure (explicit milestone non-goal). Stays nullptr
// for this harness's entire lifetime; every reachable reader tolerates it.
#include "Common/GlobalData.h"
GlobalData *TheWritableGlobalData = nullptr;
