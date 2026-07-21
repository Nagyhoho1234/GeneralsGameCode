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

// GL-backed implementation of DX8Wrapper's device-lifecycle methods for
// non-Windows (native port plan Phase 5(a) Milestone 1: device init +
// clear-to-color). Mutually exclusive with dx8wrapper_d3d8.cpp - exactly
// one of the two is ever compiled into a given build.
//
// Deliberately minimal: only Init/Shutdown/Enumerate_Devices/
// Set_Render_Device/Create_Device/Release_Device/Begin_Scene/End_Scene/Clear
// get real bodies here. Everything else DX8Wrapper declares (texture
// creation, shaders, mesh drawing, ...) is out of scope for this milestone -
// see docs/native-port-plan.md's Phase 5(a) Milestone 1 plan for the full
// non-goals list. Two traps called out by that plan are structurally
// avoided by this file simply never doing what they warn about:
// - Trap 1: Create_Device() below never calls
//   Do_Onetime_Device_Dependent_Inits() (which would pull in real texture
//   loading and a background TextureLoader thread).
// - Trap 2: Begin_Scene()/End_Scene() below never reference DX8WebBrowser.
#include "dx8wrapper.h"
#include "PortableD3D8/gl_core33.h"

#include <GLFW/glfw3.h>
#include <cstdlib>

namespace
{
	GLFWwindow* g_Window = nullptr;
	GLuint g_FBO = 0;
	GLuint g_ColorTex = 0;
	GLuint g_DepthRB = 0;
	int g_FBWidth = 0;
	int g_FBHeight = 0;

	DX8FrameStatistics g_LastFrameStatistics;

	void Destroy_Framebuffer()
	{
		if (g_FBO) { gl_DeleteFramebuffers(1, &g_FBO); g_FBO = 0; }
		if (g_ColorTex) { glDeleteTextures(1, &g_ColorTex); g_ColorTex = 0; }
		if (g_DepthRB) { gl_DeleteRenderbuffers(1, &g_DepthRB); g_DepthRB = 0; }
	}
}

// IDirect3DDevice8's real GL-backed method bodies. Declared in
// PortableD3D8/d3d8.h, defined only here - this class instance IS the
// object DX8Wrapper::_Get_D3D_Device8() returns on non-Windows.
IDirect3DDevice8::IDirect3DDevice8() {}
IDirect3DDevice8::~IDirect3DDevice8() {}

ULONG IDirect3DDevice8::Release()
{
	delete this;
	return 0;
}

HRESULT IDirect3DDevice8::TestCooperativeLevel()
{
	return D3D_OK;
}

HRESULT IDirect3DDevice8::GetDisplayMode(D3DDISPLAYMODE* pMode)
{
	pMode->Width = g_FBWidth;
	pMode->Height = g_FBHeight;
	pMode->RefreshRate = 0;
	pMode->Format = D3DFMT_A8R8G8B8;
	return D3D_OK;
}

HRESULT IDirect3DDevice8::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	return D3D_OK;
}

HRESULT IDirect3DDevice8::Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
	// Offscreen FBO only in this milestone (matching native-port-spike's
	// verification approach) - nothing to swap to a visible window.
	return D3D_OK;
}

HRESULT IDirect3DDevice8::GetRenderTarget(IDirect3DSurface8** ppRenderTarget)
{
	*ppRenderTarget = nullptr;
	return D3D_OK;
}

HRESULT IDirect3DDevice8::GetDepthStencilSurface(IDirect3DSurface8** ppZStencilSurface)
{
	*ppZStencilSurface = nullptr;
	return D3D_OK;
}

HRESULT IDirect3DDevice8::BeginScene()
{
	return D3D_OK;
}

HRESULT IDirect3DDevice8::EndScene()
{
	return D3D_OK;
}

HRESULT IDirect3DDevice8::Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
	gl_BindFramebuffer(GL_FRAMEBUFFER, g_FBO);

	if (Flags & D3DCLEAR_TARGET)
	{
		float a = ((Color >> 24) & 0xff) / 255.0f;
		float r = ((Color >> 16) & 0xff) / 255.0f;
		float g = ((Color >> 8) & 0xff) / 255.0f;
		float b = (Color & 0xff) / 255.0f;
		glClearColor(r, g, b, a);
	}

	if (Flags & D3DCLEAR_ZBUFFER)
	{
		glClearDepth(Z);
	}

	GLbitfield mask = 0;
	if (Flags & D3DCLEAR_TARGET) mask |= GL_COLOR_BUFFER_BIT;
	if (Flags & D3DCLEAR_ZBUFFER) mask |= GL_DEPTH_BUFFER_BIT;
	if (mask) glClear(mask);

	return D3D_OK;
}

HRESULT IDirect3DDevice8::SetViewport(CONST D3DVIEWPORT8* pViewport)
{
	glViewport(pViewport->X, pViewport->Y, pViewport->Width, pViewport->Height);
	return D3D_OK;
}

namespace
{
	// GL-backed IDirect3DVertexBuffer8/IDirect3DIndexBuffer8 (Milestone 2,
	// Step 2). File-local: only CreateVertexBuffer/CreateIndexBuffer below
	// ever construct these, and nothing outside this TU needs to name the
	// concrete type - see the design note in PortableD3D8/d3d8.h.
	//
	// Lock/Unlock keep a malloc'd CPU shadow copy as the single source of
	// truth (never glMapBufferRange), per Draft 18's design decision: it
	// gives whole-buffer locks (SizeToLock==0), byte-offset region locks,
	// and reads of previously-written bytes within a lock "for free" -
	// all legal in D3D, the last one UB under a write-only GL mapping.
	// D3DLOCK_DISCARD vs D3DLOCK_NOOVERWRITE are both no-ops here beyond
	// that: with a full shadow copy as ground truth, the orphan-vs-append
	// distinction is a pure GL-driver performance hint with no effect on
	// correctness, so this milestone does not need to honor it specially.
	// Unlock() re-uploads exactly the locked byte range to the GL buffer
	// via glBufferSubData.
	template <typename Base, GLenum GLTarget>
	class GLShadowBuffer8 : public Base
	{
	public:
		GLShadowBuffer8(UINT length, DWORD usage) :
			m_Length(length),
			m_ShadowData(static_cast<BYTE*>(malloc(length))),
			m_GLBuffer(0),
			m_LockOffset(0),
			m_LockSize(0)
		{
			gl_GenBuffers(1, &m_GLBuffer);
			gl_BindBuffer(GLTarget, m_GLBuffer);
			gl_BufferData(GLTarget, length, nullptr, (usage & D3DUSAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
		}

		virtual ~GLShadowBuffer8() override
		{
			if (m_GLBuffer) gl_DeleteBuffers(1, &m_GLBuffer);
			free(m_ShadowData);
		}

		virtual HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) override
		{
			m_LockOffset = OffsetToLock;
			m_LockSize = (SizeToLock == 0) ? (m_Length - OffsetToLock) : SizeToLock;
			*ppbData = m_ShadowData + OffsetToLock;
			return D3D_OK;
		}

		virtual HRESULT Unlock() override
		{
			if (m_LockSize > 0)
			{
				gl_BindBuffer(GLTarget, m_GLBuffer);
				gl_BufferSubData(GLTarget, m_LockOffset, m_LockSize, m_ShadowData + m_LockOffset);
			}
			return D3D_OK;
		}

		GLuint Get_GL_Buffer() const { return m_GLBuffer; }

	private:
		UINT   m_Length;
		BYTE*  m_ShadowData;
		GLuint m_GLBuffer;
		UINT   m_LockOffset;
		UINT   m_LockSize;
	};

	using GLVertexBuffer8 = GLShadowBuffer8<IDirect3DVertexBuffer8, GL_ARRAY_BUFFER>;
	using GLIndexBuffer8 = GLShadowBuffer8<IDirect3DIndexBuffer8, GL_ELEMENT_ARRAY_BUFFER>;

	// FVF -> GL vertex-attribute layout (Milestone 2, Step 3). Table-driven:
	// exact-matches the FVF bit pattern against 10 of dx8fvf.h's 13 named
	// formats (XYZ, XYZN, XYZNUV1/2, XYZNDUV1/2 - the latter is
	// dx8vertexbuffer.h's ubiquitous dynamic_fvf_type, XYZDUV1/2, XYZUV1/2).
	// The 3 excluded formats (XYZNDUV1TG3, XYZNUV2DMAP, XYZNDCUBEMAP) use
	// exotic 1/3/4-component texcoords and tangent-space data this
	// milestone's fixed-function-emulation GLSL program (Step 4) has no use
	// for - rejected loudly via WWASSERT rather than silently mishandled.
	//
	// Deliberately reimplemented here rather than reusing dx8fvf.cpp's
	// FVFInfoClass: that class's vertex-size computation calls
	// D3DXGetFVFVertexSize, a Windows/D3DX-only dependency (dx8fvf.cpp is
	// gated behind if(WIN32) in this directory's CMakeLists.txt, unlike this
	// file) - the offset math below is otherwise equivalent.
	struct GLVertexLayout
	{
		UINT Stride;
		bool HasNormal;
		bool HasDiffuse;
		int  TexCoordCount; // 0, 1, or 2
		UINT PositionOffset;
		UINT NormalOffset;
		UINT DiffuseOffset;
		UINT TexCoordOffset[2];
	};

	bool Translate_FVF_To_GL_Layout(DWORD FVF, GLVertexLayout* out_layout)
	{
		static const DWORD SUPPORTED_FVFS[] = {
			D3DFVF_XYZ,
			D3DFVF_XYZ | D3DFVF_NORMAL,
			D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1,
			D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX2,
			D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_DIFFUSE,
			D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX2 | D3DFVF_DIFFUSE, // dynamic_fvf_type
			D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_DIFFUSE,
			D3DFVF_XYZ | D3DFVF_TEX2 | D3DFVF_DIFFUSE,
			D3DFVF_XYZ | D3DFVF_TEX1,
			D3DFVF_XYZ | D3DFVF_TEX2,
		};

		bool recognized = false;
		for (DWORD candidate : SUPPORTED_FVFS)
		{
			if (candidate == FVF) { recognized = true; break; }
		}
		if (!recognized)
		{
			WWASSERT_PRINT(false, "Translate_FVF_To_GL_Layout: unsupported FVF (exotic/shader-era format)");
			return false;
		}

		out_layout->HasNormal = (FVF & D3DFVF_NORMAL) != 0;
		out_layout->HasDiffuse = (FVF & D3DFVF_DIFFUSE) != 0;
		out_layout->TexCoordCount = static_cast<int>((FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT);

		UINT offset = 0;
		out_layout->PositionOffset = offset;
		offset += 3 * sizeof(float);

		out_layout->NormalOffset = offset;
		if (out_layout->HasNormal) offset += 3 * sizeof(float);

		out_layout->DiffuseOffset = offset;
		if (out_layout->HasDiffuse) offset += sizeof(DWORD);

		for (int i = 0; i < 2; ++i)
		{
			out_layout->TexCoordOffset[i] = offset;
			if (i < out_layout->TexCoordCount) offset += 2 * sizeof(float);
		}

		out_layout->Stride = offset;
		return true;
	}
}

HRESULT IDirect3DDevice8::CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer)
{
	*ppVertexBuffer = new GLVertexBuffer8(Length, Usage);
	return D3D_OK;
}

HRESULT IDirect3DDevice8::CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer)
{
	*ppIndexBuffer = new GLIndexBuffer8(Length, Usage);
	return D3D_OK;
}

// IDirect3D8's methods are never actually exercised on this path -
// DX8Wrapper::Enumerate_Devices()/Create_Device() below fabricate/create
// everything directly against GLFW rather than delegating through this
// object, the same way Windows delegates through the real D3D8 DLL. Only
// the one instance DX8Wrapper::D3DInterface holds needs to exist.
IDirect3D8::IDirect3D8() {}
IDirect3D8::~IDirect3D8() {}

HRESULT IDirect3D8::GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8* pIdentifier)
{
	memset(pIdentifier, 0, sizeof(D3DADAPTER_IDENTIFIER8));
	strcpy(pIdentifier->Driver, "PortableD3D8-GL");
	strcpy(pIdentifier->Description, "GL 3.3 core (native port, Phase 5(a))");
	return D3D_OK;
}

HRESULT IDirect3D8::EnumAdapterModes(UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode)
{
	pMode->Width = g_FBWidth;
	pMode->Height = g_FBHeight;
	pMode->RefreshRate = 0;
	pMode->Format = D3DFMT_A8R8G8B8;
	return D3D_OK;
}

HRESULT IDirect3D8::GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode)
{
	return EnumAdapterModes(Adapter, 0, pMode);
}

HRESULT IDirect3D8::GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps)
{
	memset(pCaps, 0, sizeof(D3DCAPS8));
	return D3D_OK;
}

HRESULT IDirect3D8::CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface)
{
	*ppReturnedDeviceInterface = new IDirect3DDevice8();
	return D3D_OK;
}

IDirect3D8* Direct3DCreate8(UINT SDKVersion)
{
	return new IDirect3D8();
}

// --- DX8Wrapper device lifecycle -------------------------------------------

bool DX8Wrapper::Init(void* hwnd, bool lite)
{
	WWASSERT(!IsInitted);

	memset(Textures, 0, sizeof(IDirect3DBaseTexture8*) * MAX_TEXTURE_STAGES);
	memset(RenderStates, 0, sizeof(unsigned) * 256);
	memset(TextureStageStates, 0, sizeof(unsigned) * 32 * MAX_TEXTURE_STAGES);
	memset(Vertex_Shader_Constants, 0, sizeof(Vector4) * MAX_VERTEX_SHADER_CONSTANTS);
	memset(Pixel_Shader_Constants, 0, sizeof(Vector4) * MAX_PIXEL_SHADER_CONSTANTS);
	memset(&render_state, 0, sizeof(RenderStateStruct));
	memset(Shadow_Map, 0, sizeof(ZTextureClass*) * MAX_SHADOW_MAPS);

	_MainThreadID = ThreadClass::_Get_Current_Thread_ID();
	CurRenderDevice = -1;
	ResolutionWidth = 640;
	ResolutionHeight = 480;
	BitDepth = 32;
	IsWindowed = false;

	for (int light = 0; light < 4; ++light) CurrentDX8LightEnables[light] = false;

	D3DInterface = nullptr;
	D3DDevice = nullptr;

	g_LastFrameStatistics = DX8FrameStatistics();
	FrameStatistics = DX8FrameStatistics();

	render_state_changed = 0;
	for (unsigned a = 0; a < sizeof(RenderStates) / sizeof(unsigned); ++a) RenderStates[a] = 0x12345678;
	for (int a = 0; a < MAX_TEXTURE_STAGES; ++a)
		for (int b = 0; b < 32; ++b)
			TextureStageStates[a][b] = 0x12345678;
	memset(&DX8Transforms, 0, sizeof(DX8Transforms));

	if (!lite)
	{
		if (!glfwInit()) return false;
		D3DInterface = Direct3DCreate8(0);
		if (D3DInterface == nullptr) return false;
		IsInitted = true;
		Enumerate_Devices();
	}

	return true;
}

void DX8Wrapper::Shutdown()
{
	if (D3DDevice)
	{
		Release_Device();
	}

	if (D3DInterface)
	{
		D3DInterface->Release();
		D3DInterface = nullptr;
	}

	glfwTerminate();
	IsInitted = false;
}

void DX8Wrapper::Enumerate_Devices()
{
	// The real Windows path enumerates real D3D8 adapters into
	// _RenderDeviceNameTable/_RenderDeviceDescriptionTable for a device-
	// selection UI. This milestone has no such UI and only ever runs
	// against Set_Render_Device(int, ...) with device index 0, so there is
	// nothing to fabricate yet - kept as an explicit hook for when a real
	// device list becomes reachable (Phase 4 windowing work).
}

bool DX8Wrapper::Set_Render_Device(int dev, int width, int height, int bits, int windowed,
									bool resize_window, bool reset_device, bool restore_assets)
{
	WWASSERT(IsInitted);
	WWASSERT(reset_device || D3DDevice == nullptr);

	if (width > 0) ResolutionWidth = width;
	if (height > 0) ResolutionHeight = height;
	if (bits > 0) BitDepth = bits;
	if (windowed != -1) IsWindowed = (windowed != 0);

	return Create_Device();
}

bool DX8Wrapper::Create_Device()
{
	WWASSERT(D3DDevice == nullptr);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_DEPTH_BITS, 0);
	glfwWindowHint(GLFW_STENCIL_BITS, 0);
	glfwWindowHint(GLFW_SAMPLES, 0);

	g_Window = glfwCreateWindow(ResolutionWidth, ResolutionHeight, "GeneralsGameCode", nullptr, nullptr);
	if (!g_Window) return false;

	glfwMakeContextCurrent(g_Window);

	if (!gl_core33_load(reinterpret_cast<void* (*)(const char*)>(&glfwGetProcAddress)))
	{
		glfwDestroyWindow(g_Window);
		g_Window = nullptr;
		return false;
	}

	g_FBWidth = ResolutionWidth;
	g_FBHeight = ResolutionHeight;

	gl_GenFramebuffers(1, &g_FBO);
	gl_BindFramebuffer(GL_FRAMEBUFFER, g_FBO);

	glGenTextures(1, &g_ColorTex);
	glBindTexture(GL_TEXTURE_2D, g_ColorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_FBWidth, g_FBHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl_FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_ColorTex, 0);

	gl_GenRenderbuffers(1, &g_DepthRB);
	gl_BindRenderbuffer(GL_RENDERBUFFER, g_DepthRB);
	gl_RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, g_FBWidth, g_FBHeight);
	gl_FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_DepthRB);

	if (gl_CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		Destroy_Framebuffer();
		glfwDestroyWindow(g_Window);
		g_Window = nullptr;
		return false;
	}

	glViewport(0, 0, g_FBWidth, g_FBHeight);

	// Trap 1 (native-port-plan.md, Phase 5(a) Milestone 1): deliberately
	// does NOT call Do_Onetime_Device_Dependent_Inits() - that pulls in
	// real texture creation and a background TextureLoader thread, both
	// out of scope for this milestone.
	D3DDevice = new IDirect3DDevice8();
	return true;
}

void DX8Wrapper::Release_Device()
{
	if (D3DDevice)
	{
		D3DDevice->Release();
		D3DDevice = nullptr;
	}

	Destroy_Framebuffer();

	if (g_Window)
	{
		glfwDestroyWindow(g_Window);
		g_Window = nullptr;
	}
}

void DX8Wrapper::Begin_Scene()
{
	// Trap 2 (native-port-plan.md, Phase 5(a) Milestone 1): deliberately
	// never references DX8WebBrowser, unlike dx8wrapper_d3d8.cpp's version.
	DX8CALL(BeginScene());
}

void DX8Wrapper::End_Scene(bool flip_frames)
{
	DX8CALL(EndScene());
}

void DX8Wrapper::Clear(bool clear_color, bool clear_z_stencil, const Vector3& color, float dest_alpha, float z, unsigned int stencil)
{
	DWORD flags = 0;
	if (clear_color) flags |= D3DCLEAR_TARGET;
	if (clear_z_stencil) flags |= D3DCLEAR_ZBUFFER;
	if (flags)
	{
		DX8CALL(Clear(0, nullptr, flags, Convert_Color(color, dest_alpha), z, stencil));
	}
}

void DX8Wrapper::Reset_Statistics()
{
	FrameStatistics = DX8FrameStatistics();
	g_LastFrameStatistics = DX8FrameStatistics();
}

void DX8Wrapper::Begin_Statistics()
{
	FrameStatistics = DX8FrameStatistics();
}

void DX8Wrapper::End_Statistics()
{
	g_LastFrameStatistics = FrameStatistics;
}

const DX8FrameStatistics& DX8Wrapper::Get_Last_Frame_Statistics()
{
	return g_LastFrameStatistics;
}
