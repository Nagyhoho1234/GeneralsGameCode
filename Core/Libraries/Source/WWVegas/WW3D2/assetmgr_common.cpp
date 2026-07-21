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

// WW3DAssetManager::TheInstance, extracted out of both per-tree assetmgr.cpp
// files. Compiled on every platform.
//
// Why this file exists (native port plan Phase 5(a) Milestone 4, Draft 22
// Step 4, finding 1): assetmgr.cpp itself is still per-tree (Generals/ and
// GeneralsMD/ each carry their own copy, differing by 7 lines - a shader-
// library-registration/comment diff, not worth unifying into Core/ this
// milestone) and pulls in the entire prototype-loader/mesh/hlod closure, so
// it stays WIN32-only. But the texture pipeline's only *link* dependency on
// it is this one static: Get_Instance()/Texture_Hash() are inline
// (assetmgr.h), Get_Texture() is virtual (a vtable call through the
// instance pointer, no direct symbol). Hosting just TheInstance here - the
// same dx8wrapper_common.cpp/ww3d_common.cpp precedent - lets texture.cpp's
// object file link on GL without dragging assetmgr.cpp's non-portable
// closure in at all. With TheInstance==nullptr (no manager ever
// constructed on GL this milestone), every runtime touch point is already
// guarded (ww3d.cpp:648 checks it; Invalidate_Old_Unused_Textures
// early-returns when thumbnails are disabled, texture.cpp:141).
#include "assetmgr.h"

WW3DAssetManager * WW3DAssetManager::TheInstance = nullptr;
