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

// WW3D's static member variable definitions, extracted verbatim out of
// ww3d.cpp's "WW3D Static Globals" block. Compiled on every platform.
//
// Why this file exists (native port plan Phase 5(a) Milestone 3, finding 2):
// ww3d.h's inline accessors (Get_Sync_Time, Is_Sorting_Enabled,
// Is_Snapshot_Activated, ...) are cheap to call from portable code such as
// mapper.cpp/sortingrenderer.cpp, but the *statics they read* used to live
// only in ww3d.cpp.o - and ww3d.cpp itself pulls in mesh/boxrobj/texture
// filters/dazzle/etc. Any TU that referenced one of those accessors dragged
// ww3d.cpp's entire undefined-symbol closure into the link (the same
// whole-object-file lesson Milestone 1 learned building
// dx8wrapper_common.cpp). Hosting just the statics here, once, kills that
// whole class of link failure at the root instead of debugging it per step.
#include "ww3d.h"
#include "dx8wrapper.h"
#include "shader.h"
#include "texturefilter.h"
#include "rendobj.h"
#include "static_sort_list.h"

#define DEFAULT_DEBUG_SHADER_BITS	(		SHADE_CNST(\
												ShaderClass::PASS_LEQUAL,\
												ShaderClass::DEPTH_WRITE_ENABLE,\
												ShaderClass::COLOR_WRITE_ENABLE,\
												ShaderClass::SRCBLEND_ONE,\
												ShaderClass::DSTBLEND_ZERO,\
												ShaderClass::FOG_DISABLE,\
												ShaderClass::GRADIENT_MODULATE,\
												ShaderClass::SECONDARY_GRADIENT_DISABLE,\
												ShaderClass::TEXTURING_DISABLE,\
												ShaderClass::ALPHATEST_DISABLE,\
												ShaderClass::CULL_MODE_ENABLE, \
												ShaderClass::DETAILCOLOR_DISABLE,\
												ShaderClass::DETAILALPHA_DISABLE) )

#define LIGHTMAP_DEBUG_SHADER_BITS	(		SHADE_CNST(\
												ShaderClass::PASS_LEQUAL,\
												ShaderClass::DEPTH_WRITE_ENABLE,\
												ShaderClass::COLOR_WRITE_ENABLE,\
												ShaderClass::SRCBLEND_ONE,\
												ShaderClass::DSTBLEND_ZERO,\
												ShaderClass::FOG_DISABLE,\
												ShaderClass::GRADIENT_DISABLE,\
												ShaderClass::SECONDARY_GRADIENT_DISABLE,\
												ShaderClass::TEXTURING_ENABLE,\
												ShaderClass::ALPHATEST_DISABLE,\
												ShaderClass::CULL_MODE_ENABLE, \
												ShaderClass::DETAILCOLOR_DISABLE,\
												ShaderClass::DETAILALPHA_DISABLE) )

float														WW3D::LogicFrameTimeMs = 1000.0f / WWSyncPerSecond; // initialized to something to avoid division by zero on first use
float															WW3D::FractionalSyncMs = 0.0f;
unsigned int											WW3D::SyncTime = 0;
unsigned int											WW3D::PreviousSyncTime = 0;
bool														WW3D::IsSortingEnabled = true;

float														WW3D::PixelCenterX = 0.0f;
float														WW3D::PixelCenterY = 0.0f;


bool														WW3D::IsInitted = false;
bool														WW3D::IsRendering = false;
bool														WW3D::IsCapturing = false;
bool														WW3D::IsScreenUVBiased = false;

bool														WW3D::AreDecalsEnabled = true;
float														WW3D::DecalRejectionDistance = 1000000.0f;

bool														WW3D::AreStaticSortListsEnabled = false;
bool														WW3D::MungeSortOnLoad = false;

bool														WW3D::OverbrightModifyOnLoad = false;

FrameGrabClass *										WW3D::Movie = nullptr;
bool														WW3D::PauseRecord;
bool														WW3D::RecordNextFrame;

int														WW3D::FrameCount = 0;
long														WW3D::UserStat0 = 0;
long														WW3D::UserStat1 = 0;
long														WW3D::UserStat2 = 0;

float														WW3D::DefaultNativeScreenSize = 1.0f;

StaticSortListClass *								WW3D::DefaultStaticSortLists = nullptr;
StaticSortListClass *								WW3D::CurrentStaticSortLists = nullptr;


VertexMaterialClass *								WW3D::DefaultDebugMaterial  = nullptr;
ShaderClass												WW3D::DefaultDebugShader(DEFAULT_DEBUG_SHADER_BITS);
ShaderClass												WW3D::LightmapDebugShader(LIGHTMAP_DEBUG_SHADER_BITS);

WW3D::PrelitModeEnum									WW3D::PrelitMode = PRELIT_MODE_LIGHTMAP_MULTI_PASS;
bool														WW3D::ExposePrelit = false;

bool														WW3D::SnapshotActivated=false;
bool														WW3D::ThumbnailEnabled=true;

WW3D::MeshDrawModeEnum								WW3D::MeshDrawMode = MESH_DRAW_MODE_OLD;
WW3D::NPatchesGapFillingModeEnum					WW3D::NPatchesGapFillingMode = NPATCHES_GAP_FILLING_ENABLED;
unsigned													WW3D::NPatchesLevel=1;
bool														WW3D::IsTexturingEnabled=true;
bool										WW3D::IsColoringEnabled=false;

int														WW3D::LastFrameMemoryAllocations;
int														WW3D::LastFrameMemoryFrees;

int														WW3D::TextureFilter = TextureFilterClass::TextureFilterMode::TEXTURE_FILTER_BILINEAR;
int														WW3D::AnisotropyLevel = TextureFilterClass::AnisotropicFilterMode::TEXTURE_FILTER_ANISOTROPIC_2X;

bool														WW3D::Lite = false;

// Trivial DX8Wrapper forwarders, moved here alongside the statics above
// (native port plan Phase 5(a) Milestone 3, finding 2) - ww3dformat.cpp's
// Get_Valid_Texture_Format calls both, and referencing either from that
// portable TU used to require ww3d.cpp's whole link closure.
void WW3D::Get_Device_Resolution(int & set_w,int & set_h,int & set_bits,bool & set_windowed)
{
	DX8Wrapper::Get_Device_Resolution(set_w,set_h,set_bits,set_windowed);
}

int WW3D::Get_Texture_Bitdepth()
{
	return DX8Wrapper::Get_Texture_Bitdepth();
}

// Texture-reduction family, round 2 of the static-extraction precedent
// above (native port plan Phase 5(a) Milestone 4, Draft 22 Step 4, finding
// 7): texture.cpp calls Get_Texture_Reduction()/
// Is_Large_Texture_Extra_Reduction_Enabled() and textureloader.cpp calls
// Get_Texture_Min_Dimension() - all three were defined in monolithic
// ww3d.cpp over file-local (internal-linkage) statics, so referencing any
// of them from a portable TU used to pull ww3d.cpp's whole closure in.
//
// Scoped narrower than "the whole family" on purpose: only these three
// pure getters (and the statics they read) move here. Their setters
// (Set_Texture_Reduction, Enable_Large_Texture_Extra_Reduction) call
// WW3D::_Invalidate_Textures(), whose own body calls
// TextureLoader::Flush_Pending_Load_Tasks()/TextureClass::Invalidate() -
// both still Windows-only until Step 5's portability sweep lands. Moving
// the setters (or _Invalidate_Textures itself) here now would make this
// already-linked TU carry an unresolved external symbol, breaking
// RenderDeviceInit/RenderTexturedTriangle/RenderEngineDrawPath's link
// today for code no harness yet exercises - the same link-closure trap
// this milestone's Step 3 hit once already (DX8_Assert()). The setters
// and _Invalidate_Textures stay in ww3d.cpp untouched, reading these same
// statics via the extern declarations there; nothing in the pipeline's
// read path (finding 7's own citations) needs them to be portable, only
// the getters.
int  _TextureReduction = 0;
int  _TextureMinDim = 1;
bool _LargeTextureExtraReductionEnabled = false;

int	WW3D::Get_Texture_Reduction()
{
	return _TextureReduction;
}

int	WW3D::Get_Texture_Min_Dimension()
{
	return _TextureMinDim;
}

bool WW3D::Is_Large_Texture_Extra_Reduction_Enabled()
{
	return _LargeTextureExtraReductionEnabled;
}

// Add_To_Static_Sort_List/Render_And_Clear_Static_Sort_Lists, round 3 of the
// static-extraction precedent above (native port plan Phase 5(a) Milestone
// 5, Draft 24 Step 2, finding 5): mesh.cpp (portable as of this milestone)
// calls both, and they were defined in monolithic ww3d.cpp over the
// CurrentStaticSortLists/AreStaticSortListsEnabled statics already hosted
// here since Milestone 3 - referencing either from mesh.cpp used to pull
// ww3d.cpp's entire closure into the link, the exact trap this file exists
// to kill at the root (see the file header comment).
void WW3D::Add_To_Static_Sort_List(RenderObjClass *robj, unsigned int sort_level)
{
	CurrentStaticSortLists->Add_To_List(robj, sort_level);
}

void WW3D::Render_And_Clear_Static_Sort_Lists(RenderInfoClass & rinfo)
{
	// The ststic sort lists need to be disabled while we are rendering from them otherwise the
	// Render() function will just dump the objects right back on the same lists.
	bool old_enable = AreStaticSortListsEnabled;
	AreStaticSortListsEnabled = false;
	CurrentStaticSortLists->Render_And_Clear(rinfo);
	AreStaticSortListsEnabled = old_enable;
}
