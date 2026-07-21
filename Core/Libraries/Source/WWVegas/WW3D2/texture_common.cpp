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

// The one piece of texture.cpp's subsystem that dx8wrapper_draw.cpp's
// Apply_Render_State_Changes needs a direct symbol for (native port plan
// Phase 5(a) Milestone 3, finding 2): TextureBaseClass::Apply_Null is a
// single line (Set_DX8_Texture(stage, nullptr)) with zero texture-loading/
// asset-manager dependencies, but it's still a real symbol reference, not
// a virtual call - referencing it from texture.cpp itself would drag that
// whole not-yet-portable subsystem into the link. Hosted here instead,
// in its own subsystem's TU rather than the wrapper files, so Milestone 4
// (which ports texture.cpp for real) has a natural home to grow this file
// into rather than needing to un-bury it from dx8wrapper_draw.cpp later.
#include "texture.h"
#include "dx8wrapper.h"

void TextureBaseClass::Apply_Null(unsigned int stage)
{
	// This function sets the render states for a "null" texture
	DX8Wrapper::Set_DX8_Texture(stage, nullptr);
}
