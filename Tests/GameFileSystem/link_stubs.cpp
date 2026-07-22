// Harness-local link stubs (native port plan Phase 5(a) Milestone 7 Task 2,
// Draft 28 finding 5) - three global singletons that Core/GameEngine's
// Common/System layer references but this harness deliberately does not
// construct, because doing so would drag in an entire monolithic per-tree
// TU well outside a headless file-system test's scope. All three are
// genuinely unexercised at runtime by anything this harness calls; each
// stub below documents exactly why. Follows the established
// anim_sound_link_stub.cpp pattern (Milestone 5/6): loud comments, no
// hidden behavior.

// ---- TheSubsystemList (SubsystemInterface.h/.cpp) --------------------------
// SubsystemInterface::SubsystemInterface()/~SubsystemInterface() (the base
// class LocalFileSystem/ArchiveFileSystem/FileSystem all derive from)
// null-guard every touch: "if (TheSubsystemList) { ... }". Real definition
// lives in the monolithic per-tree GameEngine.cpp (GeneralsMD/Code/
// GameEngine/Source/Common/GameEngine.cpp:156) - unacceptable to drag into
// a headless file-system harness. This harness constructs its file-system
// objects directly (GUIEdit.cpp's real, shipped precedent: TheFileSystem =
// new FileSystem; TheLocalFileSystem = new ...; TheArchiveFileSystem =
// new ...; TheFileSystem->init();), never via GameEngine::initSubsystem's
// SubsystemInterfaceList::initSubsystem path, so TheSubsystemList stays
// nullptr for this harness's entire lifetime - safe by the same null guard
// every other tool (WorldBuilder, GUIEdit, Autorun) already relies on.
#include "Common/SubsystemInterface.h"
SubsystemInterfaceList *TheSubsystemList = nullptr;

// ---- TheAudio (GameAudio.h, defined in GameAudio.cpp) ----------------------
// StdBIGFileSystem::closeArchiveFile calls TheAudio->stopAudio(...) only
// when closing an archive literally named "Music.big"
// (ArchiveFileSystem.h's MUSIC_BIG). This harness's checks never author or
// close a "Music.big" archive, so that call is unreached. Real definition
// lives in GameAudio.cpp, a heavy TU needing the Miles Sound System - out
// of scope for a headless file-system test (audio stays a null link-stub
// for this entire milestone per the plan's non-goals list).
#include "Common/GameAudio.h"
AudioManager *TheAudio = nullptr;

// ---- TheWritableGlobalData (GlobalData.h, defined in GlobalData.cpp) ------
// Real link-closure finding, not predicted by the plan's finding 5 (which
// named only TheSubsystemList/TheAudio) - GameMemory.cpp and Debug.cpp
// both reference TheGlobalData (a read-only view/macro over
// TheWritableGlobalData, GlobalData.h:616-623), every call site already
// null-guarded ("if (!TheGlobalData || TheGlobalData->...)" /
// "if (TheGlobalData && TheGlobalData->m_headless)"). Real definition
// lives in GlobalData.cpp, a per-tree TU needing the full INI-driven
// settings closure (TheWritableGlobalData/INI loading is this milestone's
// explicit non-goal, GameEngine.cpp:456-527). Stays nullptr for this
// harness's entire lifetime; every reachable reader tolerates it.
#include "Common/GlobalData.h"
GlobalData *TheWritableGlobalData = nullptr;
