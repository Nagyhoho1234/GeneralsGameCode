// Link-only stub for AnimatedSoundMgrClass's two static methods
// animobj.cpp (portable since Milestone 5 Step 5) calls unconditionally
// from its per-frame update path (native port plan Phase 5(a) Milestone
// 5, Draft 24 Step 7). The real animatedsoundmgr.cpp needs WWAudio.h,
// which needs the Miles Sound System (mss.h) - a pre-existing, already-
// catalogued Phase 1 deferred dependency (part of the established 17-
// error-per-target WSL2 baseline), unrelated to and out of scope for
// this milestone's rendering work. Bodies match the real
// implementation's own behavior for an unconfigured sound manager
// (SoundLibrary/registered sound lists both null, exactly this
// harness's state): Get_Embedded_Sound_Name returns null,
// Trigger_Sound is a pass-through.
#include "animatedsoundmgr.h"

const char* AnimatedSoundMgrClass::Get_Embedded_Sound_Name(HAnimClass* anim)
{
	return nullptr;
}

float AnimatedSoundMgrClass::Trigger_Sound(HAnimClass* anim, float old_frame, float new_frame, const Matrix3D& tm)
{
	return old_frame;
}
