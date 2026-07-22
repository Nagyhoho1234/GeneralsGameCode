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

// Engine-level AnimatedSoundMgrClass stub for non-Windows builds (native
// port plan Phase 5(a) Milestone 6, Draft 26 Step 6, finding 4). The real
// animatedsoundmgr.cpp genuinely needs WWAudio.h (Miles Sound System,
// mss.h) - a catalogued Phase 6 deferral, unrelated to this milestone's
// rendering work. Compiled NOT WIN32 in WW3D2_SRC_PORTABLE, mutually
// exclusive with animatedsoundmgr.cpp (compiled WIN32-only) the same way
// dx8wrapper_gl.cpp/dx8wrapper_d3d8.cpp are mutually exclusive backends.
//
// This graduates the harness-local Tests/RenderW3DMesh/anim_sound_link_
// stub.cpp (deleted in the same change that adds this file - duplicate-
// symbol collision otherwise) to engine level, because ww3d.cpp joining
// WW3D2_SRC_PORTABLE this same milestone means WW3D::Init(!lite)/Shutdown
// now call AnimatedSoundMgrClass::Initialize()/Shutdown() at runtime
// (ww3d.cpp), not just animobj.cpp's two per-frame statics the old stub
// covered.
//
// Quiet no-op is the correct semantic for all four: sounds simply don't
// trigger, audio stays deferred wholesale, and every call site
// (ww3d.cpp, animobj.cpp) stays real and unconditional on both platforms.
#include "animatedsoundmgr.h"

void AnimatedSoundMgrClass::Initialize(const char *ini_filename)
{
}

void AnimatedSoundMgrClass::Shutdown()
{
}

const char* AnimatedSoundMgrClass::Get_Embedded_Sound_Name(HAnimClass* anim)
{
	return nullptr;
}

float AnimatedSoundMgrClass::Trigger_Sound(HAnimClass* anim, float old_frame, float new_frame, const Matrix3D& tm)
{
	return old_frame;
}
