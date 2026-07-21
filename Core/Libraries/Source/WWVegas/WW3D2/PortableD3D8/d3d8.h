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

// Stand-in for Microsoft's d3d8.h on non-Windows (Phase 5(a), see
// docs/native-port-plan.md and the Milestone 1 plan). This is NOT a COM
// reimplementation - no IUnknown/QueryInterface/AddRef, no vtable ABI.
// dx8wrapper.h's ~20 WWINLINE methods delegate to these types via the
// DX8CALL/DX8CALL_HRES macros (`_Get_D3D_Device8()->Method(...)`); making
// them plain C++ classes lets dx8wrapper.h stay byte-identical on every
// platform while the delegation boundary moves down to here.
//
// Method surface (names + signatures) was traced from every DX8CALL*
// invocation and direct D3DInterface-> call in dx8wrapper.h/.cpp, cross-
// checked against the real SDK (build/win32/_deps/dx8-src/d3d8.h) for
// signature fidelity. Most methods are stubbed inline (never exercised by
// Milestone 1 - texture/shader/state-block paths are out of scope, see the
// plan's explicit non-goals) and simply return a benign default. The
// handful Milestone 1 actually drives (constructor/destructor, Release,
// BeginScene, EndScene, Clear, Present, Reset, TestCooperativeLevel,
// SetViewport, GetRenderTarget, GetDepthStencilSurface, and
// IDirect3D8::CreateDevice) are declared only here; their real GL-backed
// bodies are defined in dx8wrapper_gl.cpp (kept out of this header so it
// doesn't have to include GLFW/GL).
#pragma once

#ifndef PORTABLE_D3D8_H
#define PORTABLE_D3D8_H

#ifndef _WIN32

#include "d3d8types.h"
#include "d3d8caps.h"

class IDirect3DSurface8;
class IDirect3DSwapChain8;
class IDirect3DBaseTexture8;
class IDirect3DTexture8;
class IDirect3DVolumeTexture8;
class IDirect3DCubeTexture8;
class IDirect3DVertexBuffer8;
class IDirect3DIndexBuffer8;
class IDirect3DDevice8;
class IDirect3D8;

typedef void* HMONITOR;

// Common resource surface (matches IDirect3DResource8's role in the real
// SDK: base of texture/vertexbuffer/indexbuffer). Priority/private-data
// management is never used on this milestone's path - trivial stubs. Real
// AddRef/Release refcounting IS kept (unlike QueryInterface, which this
// non-COM stand-in drops entirely) since callers like
// DX8Wrapper::Set_DX8_Texture and TextureClass genuinely rely on it for
// resource lifetime management, not just COM boilerplate.
class IDirect3DResource8
{
public:
	IDirect3DResource8() : m_RefCount(1) {}
	virtual ~IDirect3DResource8() {}
	virtual ULONG AddRef() { return ++m_RefCount; }
	virtual ULONG Release() { ULONG count = --m_RefCount; if (count == 0) delete this; return count; }
	virtual HRESULT GetDevice(IDirect3DDevice8** ppDevice) { *ppDevice = nullptr; return D3D_OK; }
	virtual DWORD SetPriority(DWORD PriorityNew) { return 0; }
	virtual DWORD GetPriority() { return 0; }
	virtual void PreLoad() {}
	virtual D3DRESOURCETYPE GetType() { return D3DRTYPE_SURFACE; }

private:
	ULONG m_RefCount;
};

class IDirect3DBaseTexture8 : public IDirect3DResource8
{
public:
	virtual DWORD SetLOD(DWORD LODNew) { return 0; }
	virtual DWORD GetLOD() { return 0; }
	virtual DWORD GetLevelCount() { return 1; }
};

class IDirect3DTexture8 : public IDirect3DBaseTexture8
{
public:
	D3DRESOURCETYPE GetType() override { return D3DRTYPE_TEXTURE; }
	virtual HRESULT GetLevelDesc(UINT Level, void* pDesc) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT GetSurfaceLevel(UINT Level, IDirect3DSurface8** ppSurfaceLevel) { *ppSurfaceLevel = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT LockRect(UINT Level, void* pLockedRect, CONST RECT* pRect, DWORD Flags) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT UnlockRect(UINT Level) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT AddDirtyRect(CONST RECT* pDirtyRect) { return D3D_OK; }
};

class IDirect3DVolumeTexture8 : public IDirect3DBaseTexture8
{
public:
	D3DRESOURCETYPE GetType() override { return D3DRTYPE_VOLUMETEXTURE; }
	virtual HRESULT LockBox(UINT Level, void* pLockedVolume, CONST void* pBox, DWORD Flags) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT UnlockBox(UINT Level) { return D3DERR_NOTAVAILABLE; }
};

class IDirect3DCubeTexture8 : public IDirect3DBaseTexture8
{
public:
	D3DRESOURCETYPE GetType() override { return D3DRTYPE_CUBETEXTURE; }
	virtual HRESULT GetCubeMapSurface(UINT FaceType, UINT Level, IDirect3DSurface8** ppCubeMapSurface) { *ppCubeMapSurface = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT LockRect(UINT FaceType, UINT Level, void* pLockedRect, CONST RECT* pRect, DWORD Flags) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT UnlockRect(UINT FaceType, UINT Level) { return D3DERR_NOTAVAILABLE; }
};

class IDirect3DVertexBuffer8 : public IDirect3DResource8
{
public:
	D3DRESOURCETYPE GetType() override { return D3DRTYPE_VERTEXBUFFER; }
	virtual HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) { *ppbData = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT Unlock() { return D3D_OK; }
};

class IDirect3DIndexBuffer8 : public IDirect3DResource8
{
public:
	D3DRESOURCETYPE GetType() override { return D3DRTYPE_INDEXBUFFER; }
	virtual HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) { *ppbData = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT Unlock() { return D3D_OK; }
};

// Not an IDirect3DResource8 in the real SDK either (IUnknown-derived
// directly) - mirrored here for fidelity even though it changes nothing
// for a non-COM stand-in.
class IDirect3DSurface8
{
public:
	IDirect3DSurface8() : m_RefCount(1) {}
	virtual ~IDirect3DSurface8() {}
	virtual ULONG AddRef() { return ++m_RefCount; }
	virtual ULONG Release() { ULONG count = --m_RefCount; if (count == 0) delete this; return count; }
	virtual HRESULT GetDevice(IDirect3DDevice8** ppDevice) { *ppDevice = nullptr; return D3D_OK; }
	virtual HRESULT GetDesc(void* pDesc) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT LockRect(void* pLockedRect, CONST RECT* pRect, DWORD Flags) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT UnlockRect() { return D3DERR_NOTAVAILABLE; }

private:
	ULONG m_RefCount;
};

class IDirect3DSwapChain8
{
public:
	virtual ~IDirect3DSwapChain8() {}
	virtual ULONG Release() { delete this; return 0; }
	virtual HRESULT Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) { return D3D_OK; }
	virtual HRESULT GetBackBuffer(UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer) { *ppBackBuffer = nullptr; return D3DERR_NOTAVAILABLE; }
};

// GL-specific device state lives behind this opaque pointer so this header
// (transitively included via dx8wrapper.h into ~104 files) never has to
// include GLFW/GL headers. Defined and owned by dx8wrapper_gl.cpp.
struct PortableGLDeviceState;

class IDirect3DDevice8
{
public:
	IDirect3DDevice8();
	virtual ~IDirect3DDevice8();
	virtual ULONG Release();

	virtual HRESULT TestCooperativeLevel();
	virtual UINT GetAvailableTextureMem() { return 256u * 1024u * 1024u; }
	virtual HRESULT ResourceManagerDiscardBytes(DWORD Bytes) { return D3D_OK; }
	virtual HRESULT GetDirect3D(IDirect3D8** ppD3D8) { *ppD3D8 = nullptr; return D3D_OK; }
	virtual HRESULT GetDeviceCaps(D3DCAPS8* pCaps) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT GetDisplayMode(D3DDISPLAYMODE* pMode);
	virtual HRESULT GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap) { return D3D_OK; }
	virtual void SetCursorPosition(UINT XScreenSpace, UINT YScreenSpace, DWORD Flags) {}
	virtual BOOL ShowCursor(BOOL bShow) { return FALSE; }
	virtual HRESULT CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** pSwapChain) { *pSwapChain = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT Reset(D3DPRESENT_PARAMETERS* pPresentationParameters);
	virtual HRESULT Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
	virtual HRESULT GetBackBuffer(UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer) { *ppBackBuffer = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT GetRasterStatus(D3DRASTER_STATUS* pRasterStatus) { return D3DERR_NOTAVAILABLE; }
	virtual void SetGammaRamp(DWORD Flags, CONST D3DGAMMARAMP* pRamp) {}
	virtual void GetGammaRamp(D3DGAMMARAMP* pRamp) {}
	virtual HRESULT CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture) { *ppTexture = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture) { *ppVolumeTexture = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture) { *ppCubeTexture = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer) { *ppVertexBuffer = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer) { *ppIndexBuffer = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface) { *ppSurface = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface) { *ppSurface = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT CreateImageSurface(UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface) { *ppSurface = nullptr; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT CopyRects(IDirect3DSurface8* pSourceSurface, CONST RECT* pSourceRectsArray, UINT cRects, IDirect3DSurface8* pDestinationSurface, CONST POINT* pDestPointsArray) { return D3D_OK; }
	virtual HRESULT UpdateTexture(IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture) { return D3D_OK; }
	virtual HRESULT GetFrontBuffer(IDirect3DSurface8* pDestSurface) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT SetRenderTarget(IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil) { return D3D_OK; }
	virtual HRESULT GetRenderTarget(IDirect3DSurface8** ppRenderTarget);
	virtual HRESULT GetDepthStencilSurface(IDirect3DSurface8** ppZStencilSurface);
	virtual HRESULT BeginScene();
	virtual HRESULT EndScene();
	virtual HRESULT Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
	virtual HRESULT SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) { return D3D_OK; }
	virtual HRESULT GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) { return D3D_OK; }
	virtual HRESULT MultiplyTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) { return D3D_OK; }
	virtual HRESULT SetViewport(CONST D3DVIEWPORT8* pViewport);
	virtual HRESULT GetViewport(D3DVIEWPORT8* pViewport) { return D3D_OK; }
	virtual HRESULT SetMaterial(CONST D3DMATERIAL8* pMaterial) { return D3D_OK; }
	virtual HRESULT GetMaterial(D3DMATERIAL8* pMaterial) { return D3D_OK; }
	virtual HRESULT SetLight(DWORD Index, CONST D3DLIGHT8* pLight) { return D3D_OK; }
	virtual HRESULT GetLight(DWORD Index, D3DLIGHT8* pLight) { return D3D_OK; }
	virtual HRESULT LightEnable(DWORD Index, BOOL Enable) { return D3D_OK; }
	virtual HRESULT GetLightEnable(DWORD Index, BOOL* pEnable) { *pEnable = FALSE; return D3D_OK; }
	virtual HRESULT SetClipPlane(DWORD Index, CONST float* pPlane) { return D3D_OK; }
	virtual HRESULT GetClipPlane(DWORD Index, float* pPlane) { return D3D_OK; }
	virtual HRESULT SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) { return D3D_OK; }
	virtual HRESULT GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) { *pValue = 0; return D3D_OK; }
	virtual HRESULT BeginStateBlock() { return D3D_OK; }
	virtual HRESULT EndStateBlock(DWORD* pToken) { *pToken = 0; return D3D_OK; }
	virtual HRESULT ApplyStateBlock(DWORD Token) { return D3D_OK; }
	virtual HRESULT CaptureStateBlock(DWORD Token) { return D3D_OK; }
	virtual HRESULT DeleteStateBlock(DWORD Token) { return D3D_OK; }
	virtual HRESULT CreateStateBlock(D3DSTATEBLOCKTYPE Type, DWORD* pToken) { *pToken = 0; return D3D_OK; }
	virtual HRESULT SetClipStatus(CONST D3DCLIPSTATUS8* pClipStatus) { return D3D_OK; }
	virtual HRESULT GetClipStatus(D3DCLIPSTATUS8* pClipStatus) { return D3D_OK; }
	virtual HRESULT GetTexture(DWORD Stage, IDirect3DBaseTexture8** ppTexture) { *ppTexture = nullptr; return D3D_OK; }
	virtual HRESULT SetTexture(DWORD Stage, IDirect3DBaseTexture8* pTexture) { return D3D_OK; }
	virtual HRESULT GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) { *pValue = 0; return D3D_OK; }
	virtual HRESULT SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) { return D3D_OK; }
	virtual HRESULT ValidateDevice(DWORD* pNumPasses) { *pNumPasses = 1; return D3D_OK; }
	virtual HRESULT GetInfo(DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT SetPaletteEntries(UINT PaletteNumber, CONST PALETTEENTRY* pEntries) { return D3D_OK; }
	virtual HRESULT GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY* pEntries) { return D3D_OK; }
	virtual HRESULT SetCurrentTexturePalette(UINT PaletteNumber) { return D3D_OK; }
	virtual HRESULT GetCurrentTexturePalette(UINT* PaletteNumber) { *PaletteNumber = 0; return D3D_OK; }
	virtual HRESULT DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) { return D3D_OK; }
	virtual HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT minIndex, UINT NumVertices, UINT startIndex, UINT primCount) { return D3D_OK; }
	virtual HRESULT DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) { return D3D_OK; }
	virtual HRESULT DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) { return D3D_OK; }
	virtual HRESULT ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags) { return D3D_OK; }
	virtual HRESULT CreateVertexShader(CONST DWORD* pDeclaration, CONST DWORD* pFunction, DWORD* pHandle, DWORD Usage) { *pHandle = 0; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT SetVertexShader(DWORD Handle) { return D3D_OK; }
	virtual HRESULT GetVertexShader(DWORD* pHandle) { *pHandle = 0; return D3D_OK; }
	virtual HRESULT DeleteVertexShader(DWORD Handle) { return D3D_OK; }
	virtual HRESULT SetVertexShaderConstant(DWORD Register, CONST void* pConstantData, DWORD ConstantCount) { return D3D_OK; }
	virtual HRESULT GetVertexShaderConstant(DWORD Register, void* pConstantData, DWORD ConstantCount) { return D3D_OK; }
	virtual HRESULT GetVertexShaderDeclaration(DWORD Handle, void* pData, DWORD* pSizeOfData) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT GetVertexShaderFunction(DWORD Handle, void* pData, DWORD* pSizeOfData) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride) { return D3D_OK; }
	virtual HRESULT GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData, UINT* pStride) { *ppStreamData = nullptr; *pStride = 0; return D3D_OK; }
	virtual HRESULT SetIndices(IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex) { return D3D_OK; }
	virtual HRESULT GetIndices(IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex) { *ppIndexData = nullptr; *pBaseVertexIndex = 0; return D3D_OK; }
	virtual HRESULT CreatePixelShader(CONST DWORD* pFunction, DWORD* pHandle) { *pHandle = 0; return D3DERR_NOTAVAILABLE; }
	virtual HRESULT SetPixelShader(DWORD Handle) { return D3D_OK; }
	virtual HRESULT GetPixelShader(DWORD* pHandle) { *pHandle = 0; return D3D_OK; }
	virtual HRESULT DeletePixelShader(DWORD Handle) { return D3D_OK; }
	virtual HRESULT SetPixelShaderConstant(DWORD Register, CONST void* pConstantData, DWORD ConstantCount) { return D3D_OK; }
	virtual HRESULT GetPixelShaderConstant(DWORD Register, void* pConstantData, DWORD ConstantCount) { return D3D_OK; }
	virtual HRESULT GetPixelShaderFunction(DWORD Handle, void* pData, DWORD* pSizeOfData) { return D3DERR_NOTAVAILABLE; }
	virtual HRESULT DrawRectPatch(UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) { return D3D_OK; }
	virtual HRESULT DrawTriPatch(UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) { return D3D_OK; }
	virtual HRESULT DeletePatch(UINT Handle) { return D3D_OK; }

private:
	PortableGLDeviceState* m_GLState;
};

class IDirect3D8
{
public:
	IDirect3D8();
	virtual ~IDirect3D8();
	virtual ULONG Release() { delete this; return 0; }

	virtual HRESULT RegisterSoftwareDevice(void* pInitializeFunction) { return D3D_OK; }
	virtual UINT GetAdapterCount() { return 1; }
	virtual HRESULT GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8* pIdentifier);
	virtual UINT GetAdapterModeCount(UINT Adapter) { return 1; }
	virtual HRESULT EnumAdapterModes(UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode);
	virtual HRESULT GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode);
	virtual HRESULT CheckDeviceType(UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed) { return D3D_OK; }
	virtual HRESULT CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) { return D3D_OK; }
	virtual HRESULT CheckDeviceMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType) { return D3D_OK; }
	virtual HRESULT CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) { return D3D_OK; }
	virtual HRESULT GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps);
	virtual HMONITOR GetAdapterMonitor(UINT Adapter) { return nullptr; }
	virtual HRESULT CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface);
};

// Matches the real SDK's free function - constructs the one IDirect3D8
// instance the GL backend hands back (analogous to Windows' Direct3DCreate8
// from D3D8.DLL, minus the LoadLibrary/GetProcAddress dance).
IDirect3D8* Direct3DCreate8(UINT SDKVersion);

#endif // !_WIN32

#endif // PORTABLE_D3D8_H
