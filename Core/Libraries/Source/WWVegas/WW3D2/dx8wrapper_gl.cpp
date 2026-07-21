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
#include "formconv.h"
#include "PortableD3D8/gl_core33.h"
#include "PortableD3D8/gl_fixed_function.h"

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cstdint>

namespace
{
	GLFWwindow* g_Window = nullptr;
	GLuint g_FBO = 0;
	GLuint g_ColorTex = 0;
	GLuint g_DepthRB = 0;
	int g_FBWidth = 0;
	int g_FBHeight = 0;

	DX8FrameStatistics g_LastFrameStatistics;

	// Draw-call plumbing device state (Milestone 2, Step 6). Fixed-function
	// pipeline only (programmable vertex/pixel shaders are a non-goal), a
	// single vertex stream (multiple streams are a non-goal), and a single
	// persistent VAO reconfigured per draw call to match whatever FVF is
	// current - simpler than a VAO-per-FVF cache and sufficient for this
	// milestone's one-draw-call test harness.
	DWORD g_CurrentFVF = 0;
	IDirect3DVertexBuffer8* g_CurrentVertexBuffer = nullptr;
	UINT g_CurrentVertexStride = 0;
	IDirect3DIndexBuffer8* g_CurrentIndexBuffer = nullptr;
	UINT g_CurrentBaseVertexIndex = 0;
	IDirect3DBaseTexture8* g_CurrentTexture0 = nullptr;
	GLuint g_VAO = 0;

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

	// GL-backed IDirect3DTexture8 (Milestone 2, Step 5). CreateTexture is
	// called directly, sidestepping DX8Wrapper::_Create_DX8_Texture (which
	// stays Windows-only/D3DX-only, untouched - see Draft 18). Scope is
	// deliberately narrow, matching the milestone's single-synthetic-
	// texture need: only Level 0 (mipmaps are a non-goal) and only
	// D3DFMT_A8R8G8B8 (the only format Milestone 2's test harness needs;
	// DDS/compressed/other uncompressed formats are non-goals) are
	// supported, both WWASSERT-enforced in CreateTexture below rather than
	// silently mishandled. LockRect ignores pRect (whole-texture locks
	// only) for the same reason - unlike the vertex/index buffer shadow
	// copy, there is no partial-texture-update use case in this milestone.
	//
	// Same malloc'd-CPU-shadow-copy design as GLShadowBuffer8: Unlock
	// re-uploads to the GL texture via glTexSubImage2D. Upload format is
	// GL_BGRA, matching D3DCOLOR_ARGB's in-memory little-endian byte order
	// (B,G,R,A) exactly - a standard, unambiguous technique, unlike the
	// vertex-diffuse-color question Step 4 deliberately left for Step 7's
	// canary check.
	class GLTexture8 : public IDirect3DTexture8
	{
	public:
		GLTexture8(UINT width, UINT height) :
			m_Width(width),
			m_Height(height),
			m_ShadowData(static_cast<BYTE*>(malloc(width * height * 4))),
			m_GLTexture(0)
		{
			glGenTextures(1, &m_GLTexture);
			glBindTexture(GL_TEXTURE_2D, m_GLTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		virtual ~GLTexture8() override
		{
			if (m_GLTexture) glDeleteTextures(1, &m_GLTexture);
			free(m_ShadowData);
		}

		virtual HRESULT LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) override
		{
			if (Level != 0) return D3DERR_NOTAVAILABLE;
			pLockedRect->Pitch = static_cast<INT>(m_Width * 4);
			pLockedRect->pBits = m_ShadowData;
			return D3D_OK;
		}

		virtual HRESULT UnlockRect(UINT Level) override
		{
			if (Level != 0) return D3DERR_NOTAVAILABLE;
			glBindTexture(GL_TEXTURE_2D, m_GLTexture);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, GL_BGRA, GL_UNSIGNED_BYTE, m_ShadowData);
			return D3D_OK;
		}

		GLuint Get_GL_Texture() const { return m_GLTexture; }

	private:
		UINT   m_Width;
		UINT   m_Height;
		BYTE*  m_ShadowData;
		GLuint m_GLTexture;
	};

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

	// Fixed GL vertex-attribute location contract shared between Step 3's
	// layout (offsets only, no location numbers) and this step's shader
	// (which must declare matching layout(location=N) inputs) and Step 6's
	// VAO setup (which binds each present FVF component to these numbers).
	enum GLAttribLocation
	{
		GL_ATTRIB_POSITION  = 0,
		GL_ATTRIB_NORMAL    = 1,
		GL_ATTRIB_DIFFUSE   = 2,
		GL_ATTRIB_TEXCOORD0 = 3,
		GL_ATTRIB_TEXCOORD1 = 4,
	};

	// One small always-on GLSL program emulating exactly D3D's default
	// stage-0 D3DTOP_MODULATE(D3DTA_TEXTURE, D3DTA_DIFFUSE) - not general
	// texture-stage-combiner emulation (Milestone 2 non-goal). Whether the
	// vertex-diffuse D3DCOLOR bytes need an R/B swizzle to read correctly as
	// this shader's vDiffuse was an open question when this program was
	// first written - resolved in Step 6's DrawIndexedPrimitive via
	// GL_BGRA as the vertex-attrib size argument (not here in the shader;
	// this fragment shader just consumes whatever vDiffuse it's given).
	// Step 7's test harness has an explicit R/B byte-order canary check
	// that exercises this for real (see docs/native-port-plan.md Draft 18).
	GLuint Compile_Shader(GLenum type, const char* src)
	{
		GLuint shader = gl_CreateShader(type);
		gl_ShaderSource(shader, 1, &src, nullptr);
		gl_CompileShader(shader);
		GLint ok = 0;
		gl_GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
		if (!ok)
		{
			char log[2048];
			gl_GetShaderInfoLog(shader, sizeof(log), nullptr, log);
			WWDEBUG_SAY(("GL shader compile failed: %s", log));
			gl_DeleteShader(shader);
			return 0;
		}
		return shader;
	}

	struct FixedFunctionProgram
	{
		GLuint Program;
		GLint LocMVP;
	};

	FixedFunctionProgram Build_Fixed_Function_Program()
	{
		FixedFunctionProgram result{ 0, -1 };

		const char* vs_src =
			"#version 330 core\n"
			"layout(location=0) in vec3 aPosition;\n"
			"layout(location=2) in vec4 aDiffuse;\n"
			"layout(location=3) in vec2 aTexCoord0;\n"
			"uniform mat4 uMVP;\n"
			"out vec4 vDiffuse;\n"
			"out vec2 vTexCoord0;\n"
			"void main() {\n"
			"    gl_Position = uMVP * vec4(aPosition, 1.0);\n"
			"    vDiffuse = aDiffuse;\n"
			"    vTexCoord0 = aTexCoord0;\n"
			"}\n";

		const char* fs_src =
			"#version 330 core\n"
			"in vec4 vDiffuse;\n"
			"in vec2 vTexCoord0;\n"
			"out vec4 FragColor;\n"
			"uniform sampler2D uTex;\n"
			"void main() {\n"
			"    FragColor = texture(uTex, vTexCoord0) * vDiffuse;\n"
			"}\n";

		GLuint vs = Compile_Shader(GL_VERTEX_SHADER, vs_src);
		if (!vs) return result;
		GLuint fs = Compile_Shader(GL_FRAGMENT_SHADER, fs_src);
		if (!fs) { gl_DeleteShader(vs); return result; }

		GLuint program = gl_CreateProgram();
		gl_AttachShader(program, vs);
		gl_AttachShader(program, fs);
		gl_LinkProgram(program);

		GLint linked = 0;
		gl_GetProgramiv(program, GL_LINK_STATUS, &linked);

		gl_DeleteShader(vs);
		gl_DeleteShader(fs);

		if (!linked)
		{
			char log[2048];
			gl_GetProgramInfoLog(program, sizeof(log), nullptr, log);
			WWDEBUG_SAY(("GL fixed-function program link failed: %s", log));
			gl_DeleteProgram(program);
			return result;
		}

		result.Program = program;
		result.LocMVP = gl_GetUniformLocation(program, "uMVP");

		// uTex is permanently bound to texture unit 0 (the only stage this
		// shader reads) - set once here rather than every draw call, since a
		// sampler uniform's texture-unit assignment does not need per-draw
		// updates.
		gl_UseProgram(program);
		gl_Uniform1i(gl_GetUniformLocation(program, "uTex"), 0);

		return result;
	}

	// Lazily builds and caches the one program instance this milestone ever
	// uses - never rebuilt, never a per-material variant (general shader-
	// program management is out of scope, see Draft 18's non-goals).
	// Program==0 if compilation/linking ever failed (logged above).
	const FixedFunctionProgram& Get_Fixed_Function_Program()
	{
		static FixedFunctionProgram s_Program = Build_Fixed_Function_Program();
		return s_Program;
	}

}

// Standard D3D-clip-space-to-GL-clip-space row remap, validated numerically
// in native-port-spike/main.cpp's validate_clip_space_conversion(): GL wants
// z' in [-w, w] where D3D produces z in [0, w], i.e. z_gl = 2*z_d3d - w_d3d
// as a matrix operation: row_z_gl = 2*row_z_d3d - row_w_d3d. Both matrices
// are plain column-major float[16] (GL uniform layout), not D3DMATRIX -
// external linkage (declared in PortableD3D8/gl_fixed_function.h) because
// Step 7's test harness (a separate CMake target, Tests/
// RenderTexturedTriangle) builds its own hardcoded D3D-style projection in
// that layout and needs to convert it here; this milestone does not wire
// DX8Wrapper::SetTransform/D3DTS_PROJECTION (still a stub, unchanged - out
// of scope, see Draft 18's non-goals).
void Convert_D3D_Projection_To_GL(const float d3d[16], float out_gl[16])
{
	for (int col = 0; col < 4; ++col)
	{
		float z_row = d3d[col * 4 + 2];
		float w_row = d3d[col * 4 + 3];
		for (int row = 0; row < 4; ++row) out_gl[col * 4 + row] = d3d[col * 4 + row];
		out_gl[col * 4 + 2] = 2.0f * z_row - w_row;
	}
}

// External entry point (same header) letting Step 7's test harness set the
// fixed-function program's MVP uniform directly, sidestepping the row-
// major/row-vector (D3D) vs column-major/column-vector (GL) matrix-
// convention conversion a real DX8Wrapper::SetTransform(D3DTS_WORLD/VIEW/
// PROJECTION, ...) implementation would need to handle generically - out of
// scope for this milestone's one hardcoded triangle (see Draft 18's
// non-goals; SetTransform itself stays the existing trivial stub).
// mvp_gl must already be in GL column-major layout, e.g. composed from
// Convert_D3D_Projection_To_GL's output the same way native-port-spike/
// main.cpp's Mat4 helpers compose projection*view*model.
void Set_Fixed_Function_MVP(const float mvp_gl[16])
{
	const FixedFunctionProgram& ffp = Get_Fixed_Function_Program();
	if (!ffp.Program) return;
	gl_UseProgram(ffp.Program);
	gl_UniformMatrix4fv(ffp.LocMVP, 1, GL_FALSE, mvp_gl);
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

HRESULT IDirect3DDevice8::CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture)
{
	WWASSERT_PRINT(Levels == 1, "CreateTexture: only Levels==1 supported (mipmaps are a Milestone 2 non-goal)");
	WWASSERT_PRINT(Format == D3DFMT_A8R8G8B8, "CreateTexture: only D3DFMT_A8R8G8B8 supported this milestone");
	*ppTexture = new GLTexture8(Width, Height);
	return D3D_OK;
}

// --- Draw-call plumbing (Milestone 2, Step 6) -------------------------------

HRESULT IDirect3DDevice8::SetVertexShader(DWORD Handle)
{
	// Fixed-function-pipeline convention, mirroring real D3D8 semantics
	// (DX8Wrapper::Set_Vertex_Shader passes DX8_FVF_XYZ.../dynamic_fvf_type
	// values straight through to this same call): a vertex-shader handle
	// with no high bit set IS the FVF. Programmable vertex shaders are a
	// non-goal, so no other interpretation of Handle is needed.
	g_CurrentFVF = Handle;
	return D3D_OK;
}

HRESULT IDirect3DDevice8::SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride)
{
	if (StreamNumber != 0) return D3D_OK; // multiple vertex streams are a non-goal
	g_CurrentVertexBuffer = pStreamData;
	g_CurrentVertexStride = Stride;
	return D3D_OK;
}

HRESULT IDirect3DDevice8::SetIndices(IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex)
{
	g_CurrentIndexBuffer = pIndexData;
	g_CurrentBaseVertexIndex = BaseVertexIndex;
	return D3D_OK;
}

HRESULT IDirect3DDevice8::SetTexture(DWORD Stage, IDirect3DBaseTexture8* pTexture)
{
	if (Stage != 0) return D3D_OK; // only stage 0 feeds this milestone's MODULATE(TEXTURE,DIFFUSE) shader
	g_CurrentTexture0 = pTexture;
	return D3D_OK;
}

HRESULT IDirect3DDevice8::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
	// Only the render states Step 7's verification plan actually exercises
	// (cull-mode and depth-test plumbing) get real bodies. Everything else
	// this milestone's fixed-function shader has no use for (lighting, fog,
	// alpha test/blend, texture-stage states, ...) is silently accepted,
	// matching real D3D8's tolerance of state a given draw call simply
	// never observes - not a gap, since nothing in this milestone reads it.
	switch (State)
	{
	case D3DRS_CULLMODE:
		switch (static_cast<D3DCULL>(Value))
		{
		case D3DCULL_NONE:
			glDisable(GL_CULL_FACE);
			break;
		case D3DCULL_CW:
			// D3DCULL_CW: cull clockwise-wound faces -> the surviving
			// (front) faces are counter-clockwise, GL's own default
			// front-face winding.
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CCW);
			break;
		case D3DCULL_CCW:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CW);
			break;
		default:
			break;
		}
		break;

	case D3DRS_ZENABLE:
		if (Value) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
		break;

	case D3DRS_ZWRITEENABLE:
		glDepthMask(Value ? GL_TRUE : GL_FALSE);
		break;

	case D3DRS_ZFUNC:
	{
		// Index 0 unused - D3DCMP_* values start at 1 (D3DCMP_NEVER).
		static const GLenum D3D_CMP_TO_GL[] = {
			0, GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS,
		};
		if (Value >= 1 && Value <= 8) glDepthFunc(D3D_CMP_TO_GL[Value]);
		break;
	}

	default:
		break;
	}

	return D3D_OK;
}

namespace
{
	GLenum Translate_D3D_Primitive_To_GL(D3DPRIMITIVETYPE type, UINT prim_count, UINT& out_index_count)
	{
		switch (type)
		{
		case D3DPT_POINTLIST:     out_index_count = prim_count;     return GL_POINTS;
		case D3DPT_LINELIST:      out_index_count = prim_count * 2; return GL_LINES;
		case D3DPT_LINESTRIP:     out_index_count = prim_count + 1; return GL_LINE_STRIP;
		case D3DPT_TRIANGLELIST:  out_index_count = prim_count * 3; return GL_TRIANGLES;
		case D3DPT_TRIANGLESTRIP: out_index_count = prim_count + 2; return GL_TRIANGLE_STRIP;
		case D3DPT_TRIANGLEFAN:   out_index_count = prim_count + 2; return GL_TRIANGLE_FAN;
		default:
			out_index_count = 0;
			return GL_TRIANGLES;
		}
	}
}

HRESULT IDirect3DDevice8::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT minIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	if (!g_CurrentVertexBuffer || !g_CurrentIndexBuffer) return D3DERR_INVALIDCALL;

	GLVertexLayout layout;
	if (!Translate_FVF_To_GL_Layout(g_CurrentFVF, &layout)) return D3DERR_INVALIDCALL;

	const FixedFunctionProgram& ffp = Get_Fixed_Function_Program();
	if (!ffp.Program) return D3DERR_INVALIDCALL;
	gl_UseProgram(ffp.Program);

	gl_BindVertexArray(g_VAO);

	// static_cast, not dynamic_cast: every IDirect3DVertexBuffer8/
	// IDirect3DIndexBuffer8/IDirect3DBaseTexture8 this backend ever hands
	// out IS the matching GL* subclass (CreateVertexBuffer/CreateIndexBuffer/
	// CreateTexture above are the only factories), so this is safe without
	// RTTI - same reasoning as everywhere else in this file.
	GLVertexBuffer8* vb = static_cast<GLVertexBuffer8*>(g_CurrentVertexBuffer);
	gl_BindBuffer(GL_ARRAY_BUFFER, vb->Get_GL_Buffer());

	gl_EnableVertexAttribArray(GL_ATTRIB_POSITION);
	gl_VertexAttribPointer(GL_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(g_CurrentVertexStride),
		reinterpret_cast<const void*>(static_cast<uintptr_t>(layout.PositionOffset)));

	// Normal is deliberately never bound - the fixed-function shader (Step
	// 4) has no "in" variable for it (no lighting this milestone), so
	// binding it would be dead work.

	if (layout.HasDiffuse)
	{
		gl_EnableVertexAttribArray(GL_ATTRIB_DIFFUSE);
		// GL_BGRA as the *size* argument (not a type) is the standard core-GL
		// (ARB_vertex_array_bgra, promoted in 3.2) technique for exactly this
		// case: D3DCOLOR_ARGB packs a little-endian DWORD whose in-memory
		// byte order is B,G,R,A (byte0=b, byte3=a - see d3d8types.h's
		// D3DCOLOR_ARGB macro). Reading those same 4 bytes with a plain
		// component count of 4 would silently swap R and B in the shader;
		// GL_BGRA tells GL to read the bytes in that order and present them
		// to the shader correctly as (R,G,B,A) - resolves the R/B question
		// Step 4/6 deliberately left open, without any shader-side swizzle.
		gl_VertexAttribPointer(GL_ATTRIB_DIFFUSE, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, static_cast<GLsizei>(g_CurrentVertexStride),
			reinterpret_cast<const void*>(static_cast<uintptr_t>(layout.DiffuseOffset)));
	}
	else
	{
		// No per-vertex diffuse stream in this FVF: GL's generic-attribute
		// default is (0,0,0,1), which would MODULATE the texture to black -
		// a known deferred gap, not exercised by Step 7's harness (which
		// always uses a diffuse-bearing FVF, per its R/B byte-order canary
		// check).
		gl_DisableVertexAttribArray(GL_ATTRIB_DIFFUSE);
	}

	if (layout.TexCoordCount >= 1)
	{
		gl_EnableVertexAttribArray(GL_ATTRIB_TEXCOORD0);
		gl_VertexAttribPointer(GL_ATTRIB_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(g_CurrentVertexStride),
			reinterpret_cast<const void*>(static_cast<uintptr_t>(layout.TexCoordOffset[0])));
	}
	else
	{
		gl_DisableVertexAttribArray(GL_ATTRIB_TEXCOORD0);
	}

	if (g_CurrentTexture0)
	{
		GLTexture8* tex = static_cast<GLTexture8*>(g_CurrentTexture0);
		gl_ActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex->Get_GL_Texture());
	}

	GLIndexBuffer8* ib = static_cast<GLIndexBuffer8*>(g_CurrentIndexBuffer);
	gl_BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->Get_GL_Buffer());

	UINT index_count = 0;
	GLenum gl_mode = Translate_D3D_Primitive_To_GL(PrimitiveType, primCount, index_count);

	// Indices are always 16-bit in this codebase's real usage - every
	// IndexBufferClass caller (dx8indexbuffer.h) exposes indices as
	// unsigned short exclusively; D3DFMT_INDEX32 support is deferred as
	// unneeded, not silently mishandled (nothing constructs a 32-bit index
	// buffer anywhere in this engine to exercise it).
	//
	// glDrawElementsBaseVertex (not plain glDrawElements) is required for
	// real D3D8 fidelity here: SetIndices' BaseVertexIndex and this call's
	// startIndex are both routinely nonzero in real callers
	// (dx8vertexbuffer.cpp/dx8indexbuffer.cpp) - basevertex is GL's exact
	// equivalent of BaseVertexIndex, applied to every fetched index after
	// the byte-offset (startIndex) has selected where in the index buffer
	// to start reading.
	gl_DrawElementsBaseVertex(
		gl_mode,
		static_cast<GLsizei>(index_count),
		GL_UNSIGNED_SHORT,
		reinterpret_cast<const void*>(static_cast<uintptr_t>(startIndex * sizeof(unsigned short))),
		static_cast<GLint>(g_CurrentBaseVertexIndex));

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

// Honest caps (native port plan Phase 5(a) Milestone 3, finding 3): only
// D3DFMT_A8R8G8B8 plain textures are genuinely supported - Milestone 2's
// CreateTexture/LockRect never implemented anything else, and no
// render-target/depth-stencil texture path exists yet either. Answering
// truthfully here (instead of PortableD3D8's usual D3D_OK accept-stub)
// makes DX8Caps::Check_Texture_Format_Support/Check_Render_To_Texture_
// Support/Check_Depth_Stencil_Support - and in turn ShaderClass's
// capability-gated fallback logic - work FOR the port instead of against
// it.
HRESULT IDirect3D8::CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat)
{
	if (RType != D3DRTYPE_TEXTURE) return D3DERR_NOTAVAILABLE;
	if (Usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL)) return D3DERR_NOTAVAILABLE;
	if (CheckFormat != D3DFMT_A8R8G8B8) return D3DERR_NOTAVAILABLE;
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

	DX8Caps::Shutdown();

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

	// Milestone 2, Step 6: the one persistent VAO DrawIndexedPrimitive
	// reconfigures per draw call (see g_VAO's declaration above).
	gl_GenVertexArrays(1, &g_VAO);

	// Trap 1 (native-port-plan.md, Phase 5(a) Milestone 1): deliberately
	// does NOT call Do_Onetime_Device_Dependent_Inits() - that pulls in
	// real texture creation and a background TextureLoader thread, both
	// out of scope for this milestone.
	D3DDevice = new IDirect3DDevice8();

	// Real CurrentCaps (native port plan Phase 5(a) Milestone 3, finding
	// 3), fabricated directly via DX8Caps' device-free D3DCAPS8 constructor
	// rather than Do_Onetime_Device_Dependent_Inits' DX8Wrapper::Compute_
	// Caps (which drives the Init_Caps overload through the device's
	// GetDeviceCaps accept-stub - the wrong ctor for this milestone, same
	// Trap 1 reasoning). Reports only what the GL backend genuinely does
	// today: TnL yes (the GPU transforms), 2 simultaneous textures
	// (matching the engine's actual 2-stage usage and the eventual
	// combiner-shader plan), NPatches/ZBias no (their DevCaps/RasterCaps
	// bits are simply left unset), shader versions 0.
	DisplayFormat = D3DFMT_A8R8G8B8;

	D3DCAPS8 caps;
	memset(&caps, 0, sizeof(caps));
	caps.DeviceType = D3DDEVTYPE_HAL;
	caps.AdapterOrdinal = 0;
	caps.DevCaps = D3DDEVCAPS_HWTRANSFORMANDLIGHT;
	caps.MaxSimultaneousTextures = 2;
	caps.TextureOpCaps =
		D3DTEXOPCAPS_DISABLE |
		D3DTEXOPCAPS_SELECTARG1 |
		D3DTEXOPCAPS_SELECTARG2 |
		D3DTEXOPCAPS_MODULATE |
		D3DTEXOPCAPS_ADD;
	caps.VertexShaderVersion = 0;
	caps.PixelShaderVersion = 0;

	// Zeroed D3DADAPTER_IDENTIFIER8 -> VENDOR_UNKNOWN (Define_Vendor(0))
	// makes every vendor-quirk path in DX8Caps::Compute_Caps correctly
	// inert (finding 3); GetAdapterIdentifier fills in the Driver/
	// Description strings only.
	D3DInterface->GetAdapterIdentifier(0, 0, &CurrentAdapterIdentifier);

	delete CurrentCaps;
	CurrentCaps = new DX8Caps(D3DInterface, caps, D3DFormat_To_WW3DFormat(DisplayFormat), CurrentAdapterIdentifier);

	return true;
}

void DX8Wrapper::Release_Device()
{
	if (D3DDevice)
	{
		D3DDevice->Release();
		D3DDevice = nullptr;
	}

	// Mirrors just the CurrentCaps piece of the real path's
	// Do_Onetime_Device_Dependent_Shutdowns() - the rest of that chain
	// (texture/mesh/etc. subsystems) is Trap 1, out of scope here.
	delete CurrentCaps;
	CurrentCaps = nullptr;

	if (g_VAO) { gl_DeleteVertexArrays(1, &g_VAO); g_VAO = 0; }
	g_CurrentVertexBuffer = nullptr;
	g_CurrentIndexBuffer = nullptr;
	g_CurrentTexture0 = nullptr;

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
