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

// DX8Wrapper's static member variable definitions, plus the couple of
// free-standing symbols dx8wrapper.h's WWINLINE methods reference directly
// (_DX8SingleThreaded, Log_DX8_ErrorCode). Compiled on every platform,
// alongside whichever of dx8wrapper_d3d8.cpp (Windows) / dx8wrapper_gl.cpp
// (native port, Phase 5(a)) actually implements DX8Wrapper's methods -
// those two are mutually exclusive per platform, but both need these
// definitions to exist exactly once. Extracted here rather than duplicated
// in both when writing dx8wrapper_gl.cpp showed they're identical either
// way (native port plan Phase 5(a) Milestone 1).
#include "dx8wrapper.h"

const int DEFAULT_RESOLUTION_WIDTH = 640;
const int DEFAULT_RESOLUTION_HEIGHT = 480;
const int DEFAULT_BIT_DEPTH = 32;
const int DEFAULT_TEXTURE_BIT_DEPTH = 16;
const D3DMULTISAMPLE_TYPE DEFAULT_MSAA = D3DMULTISAMPLE_NONE;

DX8FrameStatistics DX8Wrapper::FrameStatistics;

bool DX8Wrapper::IsInitted = false;
bool DX8Wrapper::_EnableTriangleDraw = true;

int DX8Wrapper::CurRenderDevice = -1;
int DX8Wrapper::ResolutionWidth = DEFAULT_RESOLUTION_WIDTH;
int DX8Wrapper::ResolutionHeight = DEFAULT_RESOLUTION_HEIGHT;
int DX8Wrapper::BitDepth = DEFAULT_BIT_DEPTH;
int DX8Wrapper::TextureBitDepth = DEFAULT_TEXTURE_BIT_DEPTH;
bool DX8Wrapper::IsWindowed = false;
D3DFORMAT DX8Wrapper::DisplayFormat = D3DFMT_UNKNOWN;
D3DMULTISAMPLE_TYPE DX8Wrapper::MultiSampleAntiAliasing = DEFAULT_MSAA;

DWORD DX8Wrapper::Vertex_Shader = 0;
DWORD DX8Wrapper::Pixel_Shader = 0;

Vector4 DX8Wrapper::Vertex_Shader_Constants[MAX_VERTEX_SHADER_CONSTANTS];
Vector4 DX8Wrapper::Pixel_Shader_Constants[MAX_PIXEL_SHADER_CONSTANTS];

LightEnvironmentClass* DX8Wrapper::Light_Environment = nullptr;

DWORD DX8Wrapper::Vertex_Processing_Behavior = 0;
ZTextureClass* DX8Wrapper::Shadow_Map[MAX_SHADOW_MAPS];

Vector3 DX8Wrapper::Ambient_Color;

bool DX8Wrapper::world_identity;
unsigned DX8Wrapper::RenderStates[256];
unsigned DX8Wrapper::TextureStageStates[MAX_TEXTURE_STAGES][32];
IDirect3DBaseTexture8* DX8Wrapper::Textures[MAX_TEXTURE_STAGES];
RenderStateStruct DX8Wrapper::render_state;
unsigned DX8Wrapper::render_state_changed;

bool DX8Wrapper::FogEnable = false;
D3DCOLOR DX8Wrapper::FogColor = 0;

IDirect3D8* DX8Wrapper::D3DInterface = nullptr;
IDirect3DDevice8* DX8Wrapper::D3DDevice = nullptr;
IDirect3DSurface8* DX8Wrapper::CurrentRenderTarget = nullptr;
IDirect3DSurface8* DX8Wrapper::CurrentDepthBuffer = nullptr;
IDirect3DSurface8* DX8Wrapper::DefaultRenderTarget = nullptr;
IDirect3DSurface8* DX8Wrapper::DefaultDepthBuffer = nullptr;
bool DX8Wrapper::IsRenderToTexture = false;

unsigned DX8Wrapper::_MainThreadID = 0;
bool DX8Wrapper::CurrentDX8LightEnables[4];
bool DX8Wrapper::IsDeviceLost;
int DX8Wrapper::ZBias;
float DX8Wrapper::ZNear;
float DX8Wrapper::ZFar;
D3DMATRIX DX8Wrapper::ProjectionMatrix;
D3DMATRIX DX8Wrapper::DX8Transforms[D3DTS_WORLD + 1];

DX8Caps* DX8Wrapper::CurrentCaps = nullptr;

unsigned DX8Wrapper::DrawPolygonLowBoundLimit = 0;

D3DADAPTER_IDENTIFIER8 DX8Wrapper::CurrentAdapterIdentifier;

unsigned long DX8Wrapper::FrameCount = 0;

DX8_CleanupHook* DX8Wrapper::m_pCleanupHook = nullptr;
#ifdef EXTENDED_STATS
DX8_Stats DX8Wrapper::stats;
#endif

// Pure accessor of the static members above - identical on both platforms,
// no device dependency. Moved out of dx8wrapper_d3d8.cpp (native port plan
// Phase 5(a) Milestone 3, finding 2 precedent): WW3D::Get_Device_Resolution
// forwards to this from ww3d_common.cpp, and referencing it from portable
// code used to require the whole d3d8-only TU.
void DX8Wrapper::Get_Device_Resolution(int & set_w,int & set_h,int & set_bits,bool & set_windowed)
{
	WWASSERT(IsInitted);

	set_w = ResolutionWidth;
	set_h = ResolutionHeight;
	set_bits = BitDepth;
	set_windowed = IsWindowed;
}

bool _DX8SingleThreaded = false;

// Non-Windows fallback: the real D3DXGetErrorStringA (Windows D3DX-only)
// isn't available here, so this just logs the raw HRESULT. Windows keeps
// its own richer implementation of the same declared function in
// dx8wrapper_d3d8.cpp (real D3DX-decoded error strings) - guarded out here
// so the two never collide as duplicate definitions in the same build.
#ifndef _WIN32
void Log_DX8_ErrorCode(unsigned res)
{
	WWDEBUG_SAY(("DX8 Error: 0x%08x", res));
	WWASSERT(0);
}
#endif
