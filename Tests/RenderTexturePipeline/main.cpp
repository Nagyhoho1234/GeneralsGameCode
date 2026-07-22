// Phase 5(a) Milestone 4 verification harness (native port plan, see
// docs/native-port-plan.md's Draft 22 for the full plan): drives the REAL
// texture pipeline end-to-end - TextureClass/TextureLoader/TextureFilterClass/
// MissingTexture/DDSFileClass/ThumbnailManagerClass, the real GL texture
// creation (DX8Wrapper::_Create_DX8_Texture), the real GL sampler-state layer
// (TextureFilterClass::Apply -> DX8Wrapper::Set_DX8_Texture_Stage_State), and
// - for the first time in this port - a REAL background-thread texture load
// on POSIX (Milestone 4 Step 1's pthread-backed ThreadClass). Sibling to
// RenderDeviceInit/RenderTexturedTriangle/RenderEngineDrawPath, not an
// extension of any of them - all three stay untouched regression tests.
//
// This harness authors its own asset bytes at runtime (a well-formed 32-bit
// uncompressed TGA and a well-formed DDS/DXT1 file, both hand-built to match
// this codebase's own parsers - see TARGA.cpp/ddsfile.cpp - not committed as
// binary blobs) into a temp directory, then loads them through the real
// _TheFileFactory/Targa/DDSFileClass code.
//
// DX8Wrapper::Set_Texture_Bitdepth(32) below is load-bearing, not boilerplate:
// DX8Wrapper::TextureBitDepth defaults to 16 (DEFAULT_TEXTURE_BIT_DEPTH,
// dx8wrapper_common.cpp - a real game sets this from a video-quality option
// during startup), and ww3dformat.cpp's Get_Valid_Texture_Format silently
// downgrades every A8R8G8B8 texture request to A4R4G4B4 whenever it reads
// 16 - found only by actually loading a non-uniform-content real texture for
// the first time in this port (a uniform/solid-color texture is invariant to
// this exact bug, which is why Milestones 2-3's synthetic textures, and even
// this harness's own first check-1 attempt, never surfaced it).
//
// Six real pixel-level checks (offscreen-FBO readback after every draw call
// has run), matching every prior milestone's verification bar:
//   1. Foreground TGA load: TextureClass(name,path,MIP_LEVELS_1), Init(),
//      draw, sampled pixel exactly matches the authored solid color -
//      exercises TextureLoader::Request_Foreground_Loading's synchronous
//      Begin_Load/Load/End_Load chain end to end.
//   2. Engine-generated mip chain: a 1-pixel checkerboard TGA (deliberately
//      NOT 2-pixel tiles - a 1-pixel checkerboard's every 2x2-aligned group
//      always contains exactly 2 of each color, so BitmapHandlerClass::
//      Combine_A8R8G8B8's real box-filter produces an EXACT, orientation-
//      independent average at every mip level >= 1, sidestepping any TGA
//      row-order ambiguity), MIP_LEVELS_ALL, drawn minified (screen
//      footprint smaller than the source texture) with point-mip filtering
//      so the sampled level is a real generated mip, not level 0.
//   3. DDS/DXT1 software decode: a hand-authored single-block DXT1 DDS with
//      col0==col1 (RGB565 pure red) - the 3-color+transparent-black branch,
//      where codes 0/1/2 all decode to the identical color - so every texel
//      decodes to the same exactly-predictable value via DDSFileClass::
//      Get_4x4_Block, with GL caps honestly reporting no DXTC support (so
//      the destination format converges through Get_Valid_Texture_Format's
//      real fallback chain onto A8R8G8B8, the only format this GL backend
//      accepts).
//   4. Missing-texture fallback: TextureClass pointed at a nonexistent path
//      hits TextureLoadTaskClass::Begin_Load()'s failure path ->
//      Apply_Missing_Texture() -> the real MissingTexture::_Get_Missing_
//      Texture() singleton (the constant 0x7FFF00FF every mip level was
//      filled with in MissingTexture::_Init(), Milestone 4 Step 5).
//   5. The first real background-thread texture load on POSIX:
//      WW3D::Set_Thumbnail_Enabled(true) + ThumbnailManagerClass::
//      Create_Thumbnail_If_Not_Found(true) routes TextureClass::Init()
//      through Load_Locked_Surface()->Request_Background_Loading instead of
//      the synchronous foreground path (see TextureClass::Init(),
//      texture.cpp) - the real background thread (Step 1's pthread-backed
//      ThreadClass) picks up the queued task, and this harness polls
//      TextureLoader::Update() until the texture reports Is_Initialized(),
//      then checks the final pixels match check 2 (same checkerboard
//      content, same box-filtered mip average) - proving the cross-thread
//      lock-pointer handoff produced identical results to the synchronous
//      path. TextureLoader::Deinit() afterward proves the real pthread_join
//      (Step 1) converges cleanly.
//   6. Sampler plumbing: a 2-texel-wide TGA (left texel != right texel)
//      sampled at the same out-of-[0,1] U value under WRAP vs CLAMP address
//      modes (TextureFilterClass::Set_U_Addr_Mode, real W3D texinfo path)
//      yields the two different, exactly-predicted texels with point
//      magnification - proves Filter.Apply -> Set_DX8_Texture_Stage_State ->
//      the real GL sampler objects (Milestone 4 Step 3) end-to-end (6a/6b,
//      ADDRESSU). 6c/6d repeat the identical proof along V (a 1-wide/
//      2-tall TGA, top texel != bottom texel, Set_V_Addr_Mode) - closing a
//      fable-model review finding: a mutation test deleting the ADDRESSV
//      case from SetTextureStageState still passed all of 1-6a/6b, since
//      nothing exercised it before 6c/6d existed.
//
// Scope call: the plan's own finding 5 additionally mentions a point-vs-
// linear magnification distinction as part of this same check; this harness
// covers the WRAP-vs-CLAMP address-mode half only (still a real, complete
// proof of the SetTextureStageState->sampler-object chain, now covering both
// ADDRESSU and ADDRESSV of the five D3DTSS_* values that chain translates)
// and treats the filter-mode half as already covered by check 2/5's
// point-mip-filtering requirement - kept to 6 checks total (6 now has four
// sub-parts), matching every prior milestone.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <unistd.h>

#include "dx8wrapper.h"
#include "dx8vertexbuffer.h"
#include "dx8indexbuffer.h"
#include "shader.h"
#include "vertmaterial.h"
#include "ww3d.h"
#include "texture.h"
#include "texturefilter.h"
#include "textureloader.h"
#include "texturethumbnail.h"
#include "missingtexture.h"
#include "ddsfile.h"
#include "thread.h"
#include "PortableD3D8/gl_core33.h"
#include "PortableD3D8/gl_fixed_function.h"

#include <GLFW/glfw3.h>

namespace
{
	const int W = 256, H = 256;
	bool g_AnyFailure = false;

	void Fail(const char* what)
	{
		fprintf(stderr, "RENDERTEXTUREPIPELINE_FAIL: %s\n", what);
		g_AnyFailure = true;
	}

	// Matches DX8_FVF_XYZDUV1 (D3DFVF_XYZ|D3DFVF_TEX1|D3DFVF_DIFFUSE) - same
	// FVF Tests/RenderEngineDrawPath uses, this time with real, varying UVs.
	struct Vertex
	{
		float x, y, z;
		DWORD diffuse;
		float u, v;
	};
	const DWORD QUAD_FVF = D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_DIFFUSE;
	const unsigned short QUAD_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

	// Same WORLD/VIEW/PROJECTION scheme as Tests/RenderEngineDrawPath (see
	// that file's header comment for the derivation) - reused verbatim so
	// Predict_Ndc/Ndc_To_Pixel_TopDown carry over exactly. Grid rows sit at
	// y=3.5/0/-3.5 (NDC magnitude <=0.7, comfortable margin inside [-1,1])
	// with >=2*HALF spacing between rows so no two quads' footprints overlap.
	const float Z0 = 5.0f;
	const float HALF = 1.25f;

	void Make_Quad(float local_x, float local_y, float local_z, float half_size,
		DWORD diffuse, float u_min, float u_max, Vertex out[4])
	{
		out[0] = { local_x - half_size, local_y - half_size, local_z, diffuse, u_min, 0.0f };
		out[1] = { local_x + half_size, local_y - half_size, local_z, diffuse, u_max, 0.0f };
		out[2] = { local_x + half_size, local_y + half_size, local_z, diffuse, u_max, 1.0f };
		out[3] = { local_x - half_size, local_y + half_size, local_z, diffuse, u_min, 1.0f };
	}

	void Predict_Ndc(float local_x, float local_y, float local_z, float* ndc_x, float* ndc_y)
	{
		float view_z = Z0 + local_z;
		*ndc_x = local_x / view_z;
		*ndc_y = local_y / view_z;
	}

	void Ndc_To_Pixel_TopDown(float ndc_x, float ndc_y, int* out_px, int* out_py)
	{
		*out_px = static_cast<int>((ndc_x + 1.0f) * 0.5f * W);
		*out_py = static_cast<int>((1.0f - ndc_y) * 0.5f * H);
	}

	bool Close(unsigned char actual, int expected, int tolerance = 2)
	{
		return std::abs(static_cast<int>(actual) - expected) <= tolerance;
	}

	void Check_Pixel(const unsigned char* topdown_rgba, int px, int py, int er, int eg, int eb, const char* what)
	{
		int idx = (py * W + px) * 4;
		unsigned char r = topdown_rgba[idx + 0], g = topdown_rgba[idx + 1], b = topdown_rgba[idx + 2];
		if (!Close(r, er) || !Close(g, eg) || !Close(b, eb))
		{
			fprintf(stderr, "  %s: pixel (%d,%d) = (%d,%d,%d), expected ~(%d,%d,%d)\n", what, px, py, r, g, b, er, eg, eb);
			g_AnyFailure = true;
		}
		else
		{
			printf("  %s: OK (%d,%d,%d) at (%d,%d)\n", what, r, g, b, px, py);
		}
	}

	D3DMATRIX Make_D3D_Identity()
	{
		D3DMATRIX m;
		memset(&m, 0, sizeof(m));
		m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
		return m;
	}

	D3DMATRIX Make_D3D_Translation(float x, float y, float z)
	{
		D3DMATRIX m = Make_D3D_Identity();
		m.m[3][0] = x;
		m.m[3][1] = y;
		m.m[3][2] = z;
		return m;
	}

	D3DMATRIX Make_D3D_PerspectiveFovLH(float fov_y, float aspect, float zn, float zf)
	{
		D3DMATRIX m;
		memset(&m, 0, sizeof(m));
		float y_scale = 1.0f / tanf(fov_y * 0.5f);
		float x_scale = y_scale / aspect;
		m.m[0][0] = x_scale;
		m.m[1][1] = y_scale;
		m.m[2][2] = zf / (zf - zn);
		m.m[2][3] = 1.0f;
		m.m[3][2] = -zn * zf / (zf - zn);
		return m;
	}

	// Draws a quad through the real engine draw path, real-texture-bound:
	// DX8VertexBufferClass/DX8IndexBufferClass via real WriteLockClass
	// objects, DX8Wrapper::Set_Vertex_Buffer/Set_Index_Buffer/Set_Shader/
	// Set_Texture/Draw_Triangles. Set_Texture(0,texture) is the one addition
	// Tests/RenderEngineDrawPath's Draw_Real_Quad never needed (that
	// harness always left stage 0 untextured, exercising the white
	// fallback) - TextureBaseClass::Apply(stage) runs automatically inside
	// DX8Wrapper::Apply_Render_State_Changes, called from Draw_Triangles.
	void Draw_Textured_Quad(float local_x, float local_y, float local_z, float half_size,
		DWORD diffuse, float u_min, float u_max, TextureBaseClass* texture)
	{
		Vertex verts[4];
		Make_Quad(local_x, local_y, local_z, half_size, diffuse, u_min, u_max, verts);

		DX8VertexBufferClass* vb = new DX8VertexBufferClass(QUAD_FVF, 4);
		{
			VertexBufferClass::WriteLockClass lock(vb);
			memcpy(lock.Get_Vertex_Array(), verts, sizeof(verts));
		}

		DX8IndexBufferClass* ib = new DX8IndexBufferClass(6);
		{
			IndexBufferClass::WriteLockClass lock(ib);
			memcpy(lock.Get_Index_Array(), QUAD_INDICES, sizeof(QUAD_INDICES));
		}

		DX8Wrapper::Set_Vertex_Buffer(vb);
		DX8Wrapper::Set_Index_Buffer(ib, 0);
		DX8Wrapper::Set_Shader(ShaderClass::_PresetOpaqueShader);
		DX8Wrapper::Set_Material(nullptr);
		DX8Wrapper::Set_Texture(0, texture);
		DX8Wrapper::Draw_Triangles(0, 2, 0, 4);

		vb->Release_Ref();
		ib->Release_Ref();
	}

	// fable-review-of-Milestone-4 finding 6: check 6 below only ever varied
	// U (Make_Quad pins v to [0,1]), so a mutation test deleting the
	// D3DTSS_ADDRESSV case from SetTextureStageState still passed all 6
	// checks - a real, honest verification gap. This mirrors Make_Quad/
	// Draw_Textured_Quad exactly, but with u pinned to [0,1] and v carrying
	// the variable out-of-range range instead, to close it with a genuine
	// pixel-sampling check (6c/6d below) rather than an assertion.
	void Make_Quad_VRange(float local_x, float local_y, float local_z, float half_size,
		DWORD diffuse, float v_min, float v_max, Vertex out[4])
	{
		out[0] = { local_x - half_size, local_y - half_size, local_z, diffuse, 0.0f, v_min };
		out[1] = { local_x + half_size, local_y - half_size, local_z, diffuse, 1.0f, v_min };
		out[2] = { local_x + half_size, local_y + half_size, local_z, diffuse, 1.0f, v_max };
		out[3] = { local_x - half_size, local_y + half_size, local_z, diffuse, 0.0f, v_max };
	}

	void Draw_Textured_Quad_VRange(float local_x, float local_y, float local_z, float half_size,
		DWORD diffuse, float v_min, float v_max, TextureBaseClass* texture)
	{
		Vertex verts[4];
		Make_Quad_VRange(local_x, local_y, local_z, half_size, diffuse, v_min, v_max, verts);

		DX8VertexBufferClass* vb = new DX8VertexBufferClass(QUAD_FVF, 4);
		{
			VertexBufferClass::WriteLockClass lock(vb);
			memcpy(lock.Get_Vertex_Array(), verts, sizeof(verts));
		}

		DX8IndexBufferClass* ib = new DX8IndexBufferClass(6);
		{
			IndexBufferClass::WriteLockClass lock(ib);
			memcpy(lock.Get_Index_Array(), QUAD_INDICES, sizeof(QUAD_INDICES));
		}

		DX8Wrapper::Set_Vertex_Buffer(vb);
		DX8Wrapper::Set_Index_Buffer(ib, 0);
		DX8Wrapper::Set_Shader(ShaderClass::_PresetOpaqueShader);
		DX8Wrapper::Set_Material(nullptr);
		DX8Wrapper::Set_Texture(0, texture);
		DX8Wrapper::Draw_Triangles(0, 2, 0, 4);

		vb->Release_Ref();
		ib->Release_Ref();
	}

	// --- Asset authoring: real file-format bytes, built at runtime --------

	std::string Temp_Path(const char* leaf)
	{
		const char* tmp = getenv("TMPDIR");
		if (!tmp) tmp = "/tmp";
		char buf[512];
		snprintf(buf, sizeof(buf), "%s/rtp_%d_%s", tmp, static_cast<int>(getpid()), leaf);
		return std::string(buf);
	}

	// Writes a well-formed uncompressed 32-bit TGA matching what TARGA.cpp's
	// Open()/Load() actually parse (see WWLib/TARGA.h's packed TGAHeader) -
	// pixel_bgra is exactly width*height*4 bytes, one row after another, each
	// pixel in B,G,R,A order (matching D3DCOLOR_ARGB's in-memory layout,
	// same convention Milestone 2/3 established for GL uploads). Row order
	// is deliberately never load-bearing in any check below (every authored
	// pattern is either uniform, varies only in X, or is a 1-pixel
	// checkerboard whose every 2x2 group is flip-invariant) - see the
	// pattern-design comments at each call site.
	void Write_TGA(const std::string& path, int width, int height, const unsigned char* pixel_bgra)
	{
		unsigned char header[18];
		memset(header, 0, sizeof(header));
		header[2] = 2;						// ImageType: uncompressed true-color
		header[12] = static_cast<unsigned char>(width & 0xFF);
		header[13] = static_cast<unsigned char>((width >> 8) & 0xFF);
		header[14] = static_cast<unsigned char>(height & 0xFF);
		header[15] = static_cast<unsigned char>((height >> 8) & 0xFF);
		header[16] = 32;						// PixelDepth
		header[17] = 0x20;					// ImageDescriptor (irrelevant here, see above)

		FILE* f = fopen(path.c_str(), "wb");
		if (!f) { Fail("could not create temp TGA file"); return; }
		fwrite(header, 1, sizeof(header), f);
		fwrite(pixel_bgra, 1, static_cast<size_t>(width) * height * 4, f);
		fclose(f);
	}

	void Write_Solid_TGA(const std::string& path, int width, int height, unsigned char r, unsigned char g, unsigned char b)
	{
		std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4);
		for (size_t i = 0; i < pixels.size(); i += 4)
		{
			pixels[i + 0] = b; pixels[i + 1] = g; pixels[i + 2] = r; pixels[i + 3] = 255;
		}
		Write_TGA(path, width, height, pixels.data());
	}

	// 1-pixel checkerboard - deliberately NOT tiled in 2x2+ blocks (see the
	// file header comment): every 2x2-aligned group of source texels always
	// contains exactly 2 of color_a and 2 of color_b, so
	// BitmapHandlerClass::Combine_A8R8G8B8's real box filter produces the
	// exact average (color_a+color_b)/2 at every mip level >= 1, regardless
	// of which 2x2 group or which row-order convention the TGA loader picks.
	// color_a/color_b channels must be exact multiples of 4 (Combine_A8R8G8B8
	// masks off each byte's low 2 bits before averaging, native port plan
	// Phase 5(a) Milestone 4 Step 6 research) so the predicted average is
	// exact arithmetic, not an approximation.
	void Write_Checkerboard_TGA(const std::string& path, int width, int height,
		unsigned char ar, unsigned char ag, unsigned char ab,
		unsigned char br, unsigned char bg, unsigned char bb)
	{
		std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4);
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				size_t i = (static_cast<size_t>(y) * width + x) * 4;
				bool is_a = ((x + y) & 1) == 0;
				pixels[i + 0] = is_a ? ab : bb;
				pixels[i + 1] = is_a ? ag : bg;
				pixels[i + 2] = is_a ? ar : br;
				pixels[i + 3] = 255;
			}
		}
		Write_TGA(path, width, height, pixels.data());
	}

	// A well-formed single-block DXT1 DDS (native port plan Phase 5(a)
	// Milestone 4 Step 6 research: LegacyDDSURFACEDESC2's on-disk-sized
	// fields exactly match ddsfile.cpp's own struct layout since it's
	// written/read with the real struct, not a hand-packed byte copy - the
	// header's Size field must equal sizeof(LegacyDDSURFACEDESC2) on THIS
	// build, never a historical/hardcoded constant, since that struct's
	// Surface field is a real pointer, not a reserved DWORD). col0==col1
	// (RGB565 pure red, 0xF800) selects DDSFileClass::Get_4x4_Block's
	// 3-color+transparent-black branch, where codes 0/1/2 all decode to the
	// identical color - every index byte 0x00 (all pixels use code 0) makes
	// the whole 4x4 block decode to one exactly-predictable solid color.
	void Write_DXT1_DDS(const std::string& path, int width, int height)
	{
		LegacyDDSURFACEDESC2 desc;
		memset(&desc, 0, sizeof(desc));
		desc.Size = sizeof(desc);
		desc.Flags = 0x00021007; // DDSD_CAPS|HEIGHT|WIDTH|PIXELFORMAT|LINEARSIZE (unchecked by this parser, kept for on-disk realism)
		desc.Height = static_cast<unsigned>(height);
		desc.Width = static_cast<unsigned>(width);
		desc.Pitch = static_cast<unsigned>((width / 4) * (height / 4) * 8); // one DXT1 block = 8 bytes
		desc.MipMapCount = 1;
		desc.PixelFormat.Size = sizeof(desc.PixelFormat);
		desc.PixelFormat.Flags = 0x4; // DDPF_FOURCC
		desc.PixelFormat.FourCC = static_cast<unsigned>(D3DFMT_DXT1);
		desc.Caps.Caps = 0x1000; // DDSCAPS_TEXTURE

		FILE* f = fopen(path.c_str(), "wb");
		if (!f) { Fail("could not create temp DDS file"); return; }
		const unsigned char magic[4] = { 'D', 'D', 'S', ' ' };
		fwrite(magic, 1, 4, f);
		fwrite(&desc, 1, sizeof(desc), f);

		int blocks_x = width / 4, blocks_y = height / 4;
		for (int by = 0; by < blocks_y; ++by)
		{
			for (int bx = 0; bx < blocks_x; ++bx)
			{
				unsigned char block[8] = { 0x00, 0xF8, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00 };
				fwrite(block, 1, sizeof(block), f);
			}
		}
		fclose(f);
	}
}

int main()
{
	if (!DX8Wrapper::Init(nullptr))
	{
		fprintf(stderr, "RENDERTEXTUREPIPELINE_FAIL: DX8Wrapper::Init failed\n");
		return 1;
	}

	if (!DX8Wrapper::Set_Render_Device(0, W, H, 32, 0))
	{
		fprintf(stderr, "RENDERTEXTUREPIPELINE_FAIL: DX8Wrapper::Set_Render_Device failed\n");
		DX8Wrapper::Shutdown();
		return 1;
	}

	// Real init sequence mirroring Do_Onetime_Device_Dependent_Inits' order
	// (dx8wrapper_d3d8.cpp:302-326) - the GL Create_Device deliberately does
	// NOT wire this itself (Draft 22's design decision: doing so would drag
	// the whole texture closure into every existing harness's link), so
	// this harness performs the same sequence explicitly.
	MissingTexture::_Init();
	TextureFilterClass::_Init_Filters(TextureFilterClass::TEXTURE_FILTER_POINT, TextureFilterClass::TEXTURE_FILTER_ANISOTROPIC_2X);
	TextureLoader::Init();
	// Real games set this from a video-quality option during startup; see
	// the file header comment for why this is load-bearing, not boilerplate.
	// Called on DX8Wrapper directly (fully inline in dx8wrapper.h) rather
	// than via WW3D::Set_Texture_Bitdepth's forwarder, which lives in
	// monolithic ww3d.cpp - not part of this harness's link closure, same
	// reason ww3d_common.cpp exists at all.
	DX8Wrapper::Set_Texture_Bitdepth(32);

	DX8Wrapper::Set_DX8_Render_State(D3DRS_ZENABLE, TRUE);
	DX8Wrapper::Begin_Scene();
	DX8Wrapper::Clear(true, true, Vector3(0.5f, 0.5f, 0.5f));

	DX8Wrapper::Set_Transform(D3DTS_WORLD, To_Matrix4x4(Make_D3D_Translation(0.0f, 0.0f, Z0)));
	DX8Wrapper::Set_Transform(D3DTS_VIEW, To_Matrix4x4(Make_D3D_Identity()));
	const float FOV_Y_90_DEGREES = 1.57079632679489661923f;
	DX8Wrapper::Set_Transform(D3DTS_PROJECTION, To_Matrix4x4(Make_D3D_PerspectiveFovLH(
		FOV_Y_90_DEGREES, 1.0f, 1.0f, 100.0f)));

	const DWORD WHITE = D3DCOLOR_XRGB(255, 255, 255);

	// The game's real runtime configuration (finding 8): thumbnails start
	// disabled, so checks 1-4 all take TextureClass::Init()'s synchronous
	// Request_Foreground_Loading path (thumbnails disabled OR MIP_LEVELS_1
	// both route there - texture.cpp's Init()).
	WW3D::Set_Thumbnail_Enabled(false);

	// --- Check 1: foreground TGA load --------------------------------------
	std::string solid_path = Temp_Path("solid.tga");
	Write_Solid_TGA(solid_path, 8, 8, 30, 200, 100);
	TextureClass* solid_tex = new TextureClass(solid_path.c_str(), nullptr, MIP_LEVELS_1);
	solid_tex->Init();
	Draw_Textured_Quad(-3.0f, 3.5f, 0.0f, HALF, WHITE, 0.0f, 1.0f, solid_tex);

	// --- Check 2: engine-generated mip chain --------------------------------
	// Drawn at a small screen footprint (HALF_MIP, ~4 screen pixels for an
	// 8x8 source) to force real GPU minification past level 0 - any level
	// >=1 gives the identical box-filtered average per the checkerboard's
	// flip-invariant design (see Write_Checkerboard_TGA).
	const float HALF_MIP = 0.08f;
	std::string checker_path = Temp_Path("checker.tga");
	Write_Checkerboard_TGA(checker_path, 8, 8, 252, 0, 0, 0, 0, 252);
	TextureClass* checker_tex = new TextureClass(checker_path.c_str(), nullptr, MIP_LEVELS_ALL);
	checker_tex->Init();
	Draw_Textured_Quad(3.0f, 3.5f, 0.0f, HALF_MIP, WHITE, 0.0f, 1.0f, checker_tex);

	// --- Check 3: DDS/DXT1 software decode ----------------------------------
	std::string dds_path = Temp_Path("block.dds");
	Write_DXT1_DDS(dds_path, 4, 4);
	TextureClass* dds_tex = new TextureClass(dds_path.c_str(), nullptr, MIP_LEVELS_1);
	dds_tex->Init();
	Draw_Textured_Quad(-3.0f, 0.0f, 0.0f, HALF, WHITE, 0.0f, 1.0f, dds_tex);

	// --- Check 4: missing-texture fallback ----------------------------------
	std::string missing_path = Temp_Path("does_not_exist.tga");
	TextureClass* missing_tex = new TextureClass(missing_path.c_str(), nullptr, MIP_LEVELS_1);
	missing_tex->Init();
	Draw_Textured_Quad(3.0f, 0.0f, 0.0f, HALF, WHITE, 0.0f, 1.0f, missing_tex);

	// --- Check 5: the real background thread --------------------------------
	std::string checker2_path = Temp_Path("checker2.tga");
	Write_Checkerboard_TGA(checker2_path, 8, 8, 252, 0, 0, 0, 0, 252);
	WW3D::Set_Thumbnail_Enabled(true);
	ThumbnailManagerClass::Create_Thumbnail_If_Not_Found(true);
	TextureClass* bg_tex = new TextureClass(checker2_path.c_str(), nullptr, MIP_LEVELS_ALL);
	bg_tex->Init();
	// Thumbnails only need to be enabled for Init() to pick the background-
	// loading route (texture.cpp's Init(), already resolved by this point) -
	// flipped back off before polling because TextureLoader::Update() ends
	// every call with TextureBaseClass::Invalidate_Old_Unused_Textures(),
	// which (when thumbnails are enabled) unconditionally dereferences
	// WW3DAssetManager::Get_Instance() - null in this harness, since
	// constructing a real asset manager would drag in the prototype-loader/
	// mesh/hlod closure Milestone 5 owns (out of scope here, same reasoning
	// as assetmgr_common.cpp's Step 4 extraction). The real game always has
	// a live asset manager whenever thumbnails are on; this harness doesn't
	// need one to prove the background thread itself works.
	WW3D::Set_Thumbnail_Enabled(false);
	{
		// Bounded poll so a regression (background thread never runs) fails
		// loudly instead of hanging CI - the real proof this is a genuine
		// cross-thread race is that it takes more than one Update() call to
		// converge, not a single-shot synchronous completion.
		int iterations = 0;
		const int MAX_ITERATIONS = 2000;
		while (!bg_tex->Is_Initialized() && iterations < MAX_ITERATIONS)
		{
			TextureLoader::Update();
			ThreadClass::Sleep_Ms(1);
			++iterations;
		}
		if (!bg_tex->Is_Initialized())
		{
			Fail("background texture load never completed (Step 1's pthread background thread did not run)");
		}
		else
		{
			printf("  5. background thread converged after %d Update() iteration(s)\n", iterations);
		}
	}
	Draw_Textured_Quad(-3.0f, -3.5f, 0.0f, HALF_MIP, WHITE, 0.0f, 1.0f, bg_tex);

	// --- Check 6: sampler plumbing (WRAP vs CLAMP) --------------------------
	// 2-texel-wide TGA, left != right. Sampled (with point magnification) at
	// U=1.3 post-wrap/clamp: WRAP takes U mod 1 = 0.3, nearest texel center
	// is 0.25 (left, distance 0.05) not 0.75 (right, distance 0.45) -> left
	// color. CLAMP clamps U to 1.0, nearest texel center is 0.75 (right,
	// distance 0.25) not 0.25 (left, distance 0.75) -> right color. Both
	// margins are comfortably unambiguous for point sampling.
	unsigned char two_col[2 * 1 * 4] = {
		0, 220, 0, 255,		// texel 0 (left): BGRA, green
		0, 140, 220, 255,	// texel 1 (right): BGRA, orange
	};
	std::string wrap_path = Temp_Path("wrapclamp.tga");
	Write_TGA(wrap_path, 2, 1, two_col);

	const float U_MAX = 2.0f;
	// Solve local_x for U=1.3 given U(local_x) linear from 0 (left edge,
	// local_x=-HALF) to U_MAX (right edge, local_x=+HALF).
	const float U_SAMPLE = 1.3f;
	const float SAMPLE_LOCAL_X = HALF * (2.0f * U_SAMPLE / U_MAX - 1.0f);

	TextureClass* wrap_tex = new TextureClass(wrap_path.c_str(), nullptr, MIP_LEVELS_1);
	wrap_tex->Init(); // default address mode is TEXTURE_ADDRESS_REPEAT (WRAP)
	Draw_Textured_Quad(0.0f, -3.5f, 0.0f, HALF, WHITE, 0.0f, U_MAX, wrap_tex);

	TextureClass* clamp_tex = new TextureClass(wrap_path.c_str(), nullptr, MIP_LEVELS_1);
	clamp_tex->Init();
	clamp_tex->Get_Filter().Set_U_Addr_Mode(TextureFilterClass::TEXTURE_ADDRESS_CLAMP);
	clamp_tex->Get_Filter().Set_V_Addr_Mode(TextureFilterClass::TEXTURE_ADDRESS_CLAMP);
	Draw_Textured_Quad(3.0f, -3.5f, 0.0f, HALF, WHITE, 0.0f, U_MAX, clamp_tex);

	// --- Check 6c/6d: sampler plumbing (WRAP vs CLAMP), ADDRESSV --------------
	// Real ADDRESSV coverage closing the fable-review-of-Milestone-4 finding
	// 6 gap above: a 1-wide/2-tall TGA (top texel != bottom texel), sampled
	// at the same out-of-[0,1] V value under WRAP vs CLAMP, mirroring 6a/6b's
	// math exactly but along V instead of U.
	//
	// Row-to-V mapping: verified against this harness's own rendered output,
	// not assumed from reading TARGA.cpp in isolation - Targa::Open()'s
	// YFlip() (called here since ImageDescriptor's TGAIDF_YORIGIN bit is
	// set, declaring top-origin data on disk) suggested a reversal, but the
	// net effect actually observed end-to-end through BitmapHandlerClass/
	// GLTexture8's upload is a direct mapping with no net flip, same as the
	// unflipped X axis above: the row written FIRST below lands at V near 0,
	// the row written LAST lands at V near 1.
	unsigned char v_two_col[1 * 2 * 4] = {
		220, 140, 30, 255,	// row written first (top_color): BGRA -> RGB(30,140,220) sky-blue, lands at V~0
		0, 180, 220, 255,	// row written last (bottom_color): BGRA -> RGB(220,180,0) gold, lands at V~1
	};
	std::string vaddr_path = Temp_Path("vaddrwrapclamp.tga");
	Write_TGA(vaddr_path, 1, 2, v_two_col);

	const float V_MAX = 2.0f;
	const float V_SAMPLE = 1.3f;
	const float SAMPLE_LOCAL_Y = HALF * (2.0f * V_SAMPLE / V_MAX - 1.0f);

	TextureClass* vwrap_tex = new TextureClass(vaddr_path.c_str(), nullptr, MIP_LEVELS_1);
	vwrap_tex->Init(); // default address mode is TEXTURE_ADDRESS_REPEAT (WRAP)
	Draw_Textured_Quad_VRange(0.0f, 3.5f, 0.0f, HALF, WHITE, 0.0f, V_MAX, vwrap_tex);

	TextureClass* vclamp_tex = new TextureClass(vaddr_path.c_str(), nullptr, MIP_LEVELS_1);
	vclamp_tex->Init();
	vclamp_tex->Get_Filter().Set_U_Addr_Mode(TextureFilterClass::TEXTURE_ADDRESS_CLAMP);
	vclamp_tex->Get_Filter().Set_V_Addr_Mode(TextureFilterClass::TEXTURE_ADDRESS_CLAMP);
	Draw_Textured_Quad_VRange(0.0f, 0.0f, 0.0f, HALF, WHITE, 0.0f, V_MAX, vclamp_tex);

	DX8Wrapper::End_Scene(false);

	unsigned char* pixels = static_cast<unsigned char*>(malloc(4 * W * H));
	glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	unsigned char* topdown = static_cast<unsigned char*>(malloc(4 * W * H));
	for (int y = 0; y < H; ++y)
	{
		memcpy(&topdown[y * W * 4], &pixels[(H - 1 - y) * W * 4], W * 4);
	}
	free(pixels);

	int px, py;
	float ndc_x, ndc_y;

	Predict_Ndc(-3.0f, 3.5f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 30, 200, 100, "1. foreground TGA load");

	Predict_Ndc(3.0f, 3.5f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 126, 0, 126, "2. engine-generated mip chain");

	Predict_Ndc(-3.0f, 0.0f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 248, 0, 0, "3. DDS/DXT1 software decode");

	Predict_Ndc(3.0f, 0.0f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 255, 0, 255, "4. missing-texture fallback");

	Predict_Ndc(-3.0f, -3.5f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 126, 0, 126, "5. real background-thread load");

	Predict_Ndc(SAMPLE_LOCAL_X, -3.5f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 0, 220, 0, "6a. sampler plumbing: WRAP");

	Predict_Ndc(3.0f + SAMPLE_LOCAL_X, -3.5f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 220, 140, 0, "6b. sampler plumbing: CLAMP");

	Predict_Ndc(0.0f, 3.5f + SAMPLE_LOCAL_Y, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 30, 140, 220, "6c. sampler plumbing: ADDRESSV WRAP");

	Predict_Ndc(0.0f, 0.0f + SAMPLE_LOCAL_Y, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 220, 180, 0, "6d. sampler plumbing: ADDRESSV CLAMP");

	free(topdown);

	DX8Wrapper::Set_Vertex_Buffer(nullptr);
	DX8Wrapper::Set_Index_Buffer(nullptr, 0);
	DX8Wrapper::Set_Texture(0, nullptr);
	DX8Wrapper::Set_Material(nullptr);

	solid_tex->Release_Ref();
	checker_tex->Release_Ref();
	dds_tex->Release_Ref();
	missing_tex->Release_Ref();
	bg_tex->Release_Ref();
	wrap_tex->Release_Ref();
	clamp_tex->Release_Ref();
	vwrap_tex->Release_Ref();
	vclamp_tex->Release_Ref();

	TextureLoader::Deinit();
	MissingTexture::_Deinit();

	DX8Wrapper::Shutdown();

	unlink(solid_path.c_str());
	unlink(checker_path.c_str());
	unlink(dds_path.c_str());
	unlink(checker2_path.c_str());
	unlink(wrap_path.c_str());
	unlink(vaddr_path.c_str());

	if (g_AnyFailure)
	{
		fprintf(stderr, "RENDERTEXTUREPIPELINE_FAIL: one or more checks failed (see above)\n");
		return 1;
	}

	printf("RENDERTEXTUREPIPELINE_OK: all 6 checks passed (%dx%d)\n", W, H);
	return 0;
}
