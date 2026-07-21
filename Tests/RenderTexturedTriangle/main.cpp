// Phase 5(a) Milestone 2 verification harness (native port plan, see
// docs/native-port-plan.md's Draft 18 for the full plan): "draw one
// textured triangle" through DX8Wrapper's real device-object-level buffer/
// texture/draw-call API (CreateVertexBuffer/CreateIndexBuffer/CreateTexture/
// Lock/Unlock/SetStreamSource/SetIndices/SetTexture/SetRenderState/
// DrawIndexedPrimitive) - not through real mesh loading, matching the
// milestone's scope. Sibling to Tests/RenderDeviceInit, not an extension of
// it, so that harness stays an untouched Milestone 1 regression test.
//
// Six real pixel-level checks (all via a single offscreen-FBO readback after
// every draw call has run), matching Milestone 1's verification bar - "did
// it not crash" is not enough:
//   1. Background-color integrity: an untouched sample point still matches
//      the requested clear color.
//   2. Textured-interior sampling against a 4-quadrant marker texture.
//   3. D3D top-left-origin orientation pin (the same V-flip-at-upload
//      technique native-port-spike/main.cpp validated).
//   4. An R/B byte-order canary: solid-white texture (channel-order-
//      independent) + a pure-red vertex-diffuse color, so a wrong final
//      pixel color isolates the question Step 4/6 deliberately left open -
//      does the vertex-diffuse D3DCOLOR need an R/B swizzle to read
//      correctly through GL_UNSIGNED_BYTE/normalized?
//   5. Cull-mode plumbing: the same CCW-wound triangle drawn once under
//      D3DCULL_CW (expected to survive) and once under D3DCULL_CCW
//      (expected to be culled) - proves cull state has a real, correct
//      effect, not just "always draws."
//   6. Depth-test plumbing: a near (blue) triangle drawn first, a far (red)
//      triangle drawn second at the identical footprint - GL_LESS must
//      reject the far overdraw, leaving blue. Draw order alone would give
//      the wrong (red) answer without a working depth test, so this is a
//      real negative control, not just "did something render."
//
// All geometry is authored directly in NDC via an identity MVP
// (Set_Fixed_Function_MVP), sidestepping D3D's row-major/row-vector vs GL's
// column-major/column-vector projection-matrix conversion entirely -
// Convert_D3D_Projection_To_GL exists (Step 4/6) but testing a real
// projection matrix is out of this milestone's scope (one hardcoded
// triangle, not a camera).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "dx8wrapper.h"
#include "PortableD3D8/gl_core33.h"
#include "PortableD3D8/gl_fixed_function.h"

#include <GLFW/glfw3.h>

namespace
{
	const int W = 256, H = 256;
	bool g_AnyFailure = false;

	void Fail(const char* what)
	{
		fprintf(stderr, "RENDERTEXTUREDTRIANGLE_FAIL: %s\n", what);
		g_AnyFailure = true;
	}

	// Matches PortableD3D8/d3d8types.h's D3DFVF_XYZ|D3DFVF_TEX1|D3DFVF_DIFFUSE
	// byte layout exactly (position@0, diffuse@12, texcoord0@16) - see
	// Translate_FVF_To_GL_Layout in dx8wrapper_gl.cpp (Step 3).
	struct Vertex
	{
		float x, y, z;
		DWORD diffuse;
		float u, v;
	};

	const DWORD TRIANGLE_FVF = D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_DIFFUSE;

	const float IDENTITY_MVP[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};

	// Right triangle covering the lower-right half of the square centered at
	// (cx,cy) with half-extent h, in NDC (Y-up). Always CCW-wound (GL's
	// default front face) for any h>0: v0(bottom-left)->v1(bottom-right)-
	// >v2(top-right) turns left. u/v span [0,1] across the same square
	// (v0->(0,0), v1->(1,0), v2->(1,1)) - only the lower-right texture
	// triangle is ever actually sampled by fragments this triangle covers.
	void Make_Tri(float cx, float cy, float h, float z, DWORD diffuse, Vertex out[3])
	{
		out[0] = { cx - h, cy - h, z, diffuse, 0.0f, 0.0f };
		out[1] = { cx + h, cy - h, z, diffuse, 1.0f, 0.0f };
		out[2] = { cx + h, cy + h, z, diffuse, 1.0f, 1.0f };
	}

	// Converts an NDC point (Y-up, matching Make_Tri's authoring space) to a
	// pixel coordinate in a top-down (row 0 = screen top) readback buffer.
	void Ndc_To_Pixel_TopDown(float ndc_x, float ndc_y, int* out_px, int* out_py)
	{
		*out_px = static_cast<int>((ndc_x + 1.0f) * 0.5f * W);
		*out_py = static_cast<int>((1.0f - ndc_y) * 0.5f * H);
	}

	// A point guaranteed inside Make_Tri(cx,cy,h,...)'s triangle: offset
	// (+0.3h, -0.3h) from center satisfies the triangle's inside condition
	// (X-cx) >= (Y-cy) for any h>0 (0.3h >= -0.3h).
	void Interior_Sample_Point(float cx, float cy, float h, int* out_px, int* out_py)
	{
		Ndc_To_Pixel_TopDown(cx + 0.3f * h, cy - 0.3f * h, out_px, out_py);
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

	// Writes an NxN texture directly in the GL_BGRA byte order GLTexture8
	// uploads with (Step 5) - callers specify the color they want to SEE
	// rendered (r,g,b), this function handles the byte-order translation so
	// texture-color checks in this harness are unambiguous regardless of
	// the (separately, deliberately unresolved) vertex-diffuse R/B question.
	void Fill_Solid(BYTE* pixels, int size, BYTE r, BYTE g, BYTE b)
	{
		for (int i = 0; i < size * size; ++i)
		{
			pixels[i * 4 + 0] = b;
			pixels[i * 4 + 1] = g;
			pixels[i * 4 + 2] = r;
			pixels[i * 4 + 3] = 255;
		}
	}

	// 4-quadrant orientation marker, authored top-left-origin (row 0 = top
	// row) - top-left=red, top-right=green, bottom-left=blue, bottom-
	// right=yellow. GLTexture8's Lock/Unlock has no built-in V-flip (that's
	// this test's job, matching how a real D3D8 texture loader port would
	// need to flip at upload time or UV-generation time - both equivalent,
	// see native-port-spike/main.cpp's flip_rows_rgba).
	void Fill_Quadrant_Marker(BYTE* pixels, int size)
	{
		int half = size / 2;
		for (int y = 0; y < size; ++y)
		{
			bool top = y < half;
			for (int x = 0; x < size; ++x)
			{
				bool left = x < half;
				BYTE r, g, b;
				if (top && left)       { r = 255; g = 0;   b = 0;   } // red
				else if (top && !left) { r = 0;   g = 255; b = 0;   } // green
				else if (!top && left) { r = 0;   g = 0;   b = 255; } // blue
				else                    { r = 255; g = 255; b = 0;   } // yellow

				// V-flip at upload: source row 0 (top-authored) must land in
				// the LAST uploaded row, so GL's v=1 (last row, bottom-left-
				// origin sampling) reads the top-authored color.
				int dst_y = size - 1 - y;
				int idx = (dst_y * size + x) * 4;
				pixels[idx + 0] = b;
				pixels[idx + 1] = g;
				pixels[idx + 2] = r;
				pixels[idx + 3] = 255;
			}
		}
	}

	IDirect3DTexture8* Create_Solid_Texture(IDirect3DDevice8* device, BYTE r, BYTE g, BYTE b)
	{
		IDirect3DTexture8* tex = nullptr;
		device->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex);
		D3DLOCKED_RECT rect;
		tex->LockRect(0, &rect, nullptr, 0);
		Fill_Solid(static_cast<BYTE*>(rect.pBits), 4, r, g, b);
		tex->UnlockRect(0);
		return tex;
	}

	// Draws Make_Tri(cx,cy,h,diffuse) with the given texture/cull/depth
	// state, through the same device-object API dx8vertexbuffer.cpp/
	// dx8indexbuffer.cpp themselves use (CreateVertexBuffer/CreateIndexBuffer/
	// Lock/Unlock/SetStreamSource/SetIndices), never DX8VertexBufferClass/
	// DX8IndexBufferClass (which are out of scope this milestone).
	void Draw_Tri(IDirect3DDevice8* device, float cx, float cy, float h, float z, DWORD diffuse,
		IDirect3DTexture8* tex, D3DCULL cull, bool z_enable, bool z_write)
	{
		Vertex verts[3];
		Make_Tri(cx, cy, h, z, diffuse, verts);

		IDirect3DVertexBuffer8* vb = nullptr;
		device->CreateVertexBuffer(sizeof(verts), 0, TRIANGLE_FVF, D3DPOOL_MANAGED, &vb);
		BYTE* vb_data = nullptr;
		vb->Lock(0, 0, &vb_data, 0);
		memcpy(vb_data, verts, sizeof(verts));
		vb->Unlock();

		unsigned short indices[3] = { 0, 1, 2 };
		IDirect3DIndexBuffer8* ib = nullptr;
		device->CreateIndexBuffer(sizeof(indices), 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ib);
		BYTE* ib_data = nullptr;
		ib->Lock(0, 0, &ib_data, 0);
		memcpy(ib_data, indices, sizeof(indices));
		ib->Unlock();

		device->SetVertexShader(TRIANGLE_FVF);
		device->SetStreamSource(0, vb, sizeof(Vertex));
		device->SetIndices(ib, 0);
		device->SetTexture(0, tex);
		device->SetRenderState(D3DRS_CULLMODE, cull);
		device->SetRenderState(D3DRS_ZENABLE, z_enable ? 1 : 0);
		device->SetRenderState(D3DRS_ZWRITEENABLE, z_write ? 1 : 0);
		device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
		device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 3, 0, 1);

		vb->Release();
		ib->Release();
	}
}

int main()
{
	if (!DX8Wrapper::Init(nullptr))
	{
		fprintf(stderr, "RENDERTEXTUREDTRIANGLE_FAIL: DX8Wrapper::Init failed\n");
		return 1;
	}

	if (!DX8Wrapper::Set_Render_Device(0, W, H, 32, 0))
	{
		fprintf(stderr, "RENDERTEXTUREDTRIANGLE_FAIL: DX8Wrapper::Set_Render_Device failed\n");
		DX8Wrapper::Shutdown();
		return 1;
	}

	IDirect3DDevice8* device = DX8Wrapper::_Get_D3D_Device8();

	const Vector3 clear_color(0.5f, 0.5f, 0.5f);
	DX8Wrapper::Begin_Scene();
	DX8Wrapper::Clear(true, true, clear_color);

	Set_Fixed_Function_MVP(IDENTITY_MVP);

	// Region layout (NDC, non-overlapping): a 3x2 grid, h=0.25, centers
	// spaced 0.6 apart (> 2*h) so no two regions' footprints touch.
	const float H_SIZE = 0.25f;

	// Region A: textured-interior sampling + top-left-origin orientation pin.
	IDirect3DTexture8* quadrant_tex = nullptr;
	device->CreateTexture(64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &quadrant_tex);
	{
		D3DLOCKED_RECT rect;
		quadrant_tex->LockRect(0, &rect, nullptr, 0);
		Fill_Quadrant_Marker(static_cast<BYTE*>(rect.pBits), 64);
		quadrant_tex->UnlockRect(0);
	}
	Draw_Tri(device, -0.6f, 0.6f, H_SIZE, 0.0f, 0xFFFFFFFFu, quadrant_tex, D3DCULL_CW, true, true);

	// Region B: R/B byte-order canary (solid white texture + pure-red diffuse).
	IDirect3DTexture8* white_tex = Create_Solid_Texture(device, 255, 255, 255);
	Draw_Tri(device, 0.6f, 0.6f, H_SIZE, 0.0f, D3DCOLOR_XRGB(255, 0, 0), white_tex, D3DCULL_CW, true, true);

	// Region C1: cull=D3DCULL_CW on a CCW-wound triangle -> front, drawn.
	IDirect3DTexture8* cyan_tex = Create_Solid_Texture(device, 0, 255, 255);
	Draw_Tri(device, -0.6f, -0.6f, H_SIZE, 0.0f, 0xFFFFFFFFu, cyan_tex, D3DCULL_CW, true, true);

	// Region C2: same CCW-wound triangle, cull=D3DCULL_CCW -> back, culled.
	Draw_Tri(device, 0.0f, -0.6f, H_SIZE, 0.0f, 0xFFFFFFFFu, cyan_tex, D3DCULL_CCW, true, true);

	// Region D: depth-test negative control - near (blue, NDC z=-0.5) drawn
	// first, far (red, NDC z=+0.5) drawn second at the identical footprint.
	// GL's default depth range maps NDC z=-1->0 (near) and z=+1->1 (far), so
	// with glDepthFunc(GL_LESS) the far/red draw's larger depth value must
	// be rejected against what blue already wrote. Without a working depth
	// test, draw order alone would leave red on top - this is a real
	// negative control, not just "did something render."
	IDirect3DTexture8* blue_tex = Create_Solid_Texture(device, 0, 0, 255);
	IDirect3DTexture8* red_tex = Create_Solid_Texture(device, 255, 0, 0);
	Draw_Tri(device, 0.6f, -0.6f, H_SIZE, -0.5f, 0xFFFFFFFFu, blue_tex, D3DCULL_NONE, true, true); // near
	Draw_Tri(device, 0.6f, -0.6f, H_SIZE, 0.5f, 0xFFFFFFFFu, red_tex, D3DCULL_NONE, true, true);   // far

	DX8Wrapper::End_Scene(false);

	unsigned char* pixels = static_cast<unsigned char*>(malloc(4 * W * H));
	glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// glReadPixels is bottom-up; flip once so every check above can reason
	// in top-down pixel coordinates (matching Ndc_To_Pixel_TopDown).
	unsigned char* topdown = static_cast<unsigned char*>(malloc(4 * W * H));
	for (int y = 0; y < H; ++y)
	{
		memcpy(&topdown[y * W * 4], &pixels[(H - 1 - y) * W * 4], W * 4);
	}
	free(pixels);

	int px, py;

	// 1. Background-color integrity.
	Ndc_To_Pixel_TopDown(0.0f, 0.1f, &px, &py);
	Check_Pixel(topdown, px, py, 128, 128, 128, "1. background integrity");

	// 2/3. Textured sampling + orientation: two points inside region A's
	// triangle, chosen to land in different quadrants (see Make_Tri's doc
	// comment for the u/v derivation).
	Ndc_To_Pixel_TopDown(-0.6f + 0.2f * H_SIZE, 0.6f + 0.1f * H_SIZE, &px, &py);
	Check_Pixel(topdown, px, py, 0, 255, 0, "2/3. quadrant sample (top-right / green)");
	Ndc_To_Pixel_TopDown(-0.6f + 0.4f * H_SIZE, 0.6f - 0.3f * H_SIZE, &px, &py);
	Check_Pixel(topdown, px, py, 255, 255, 0, "2/3. quadrant sample (bottom-right / yellow)");

	// 4. R/B byte-order canary.
	Interior_Sample_Point(0.6f, 0.6f, H_SIZE, &px, &py);
	Check_Pixel(topdown, px, py, 255, 0, 0, "4. R/B byte-order canary");

	// 5. Cull-mode plumbing.
	Interior_Sample_Point(-0.6f, -0.6f, H_SIZE, &px, &py);
	Check_Pixel(topdown, px, py, 0, 255, 255, "5a. cull=CW, CCW tri survives");
	Interior_Sample_Point(0.0f, -0.6f, H_SIZE, &px, &py);
	Check_Pixel(topdown, px, py, 128, 128, 128, "5b. cull=CCW, CCW tri culled (background)");

	// 6. Depth-test plumbing.
	Interior_Sample_Point(0.6f, -0.6f, H_SIZE, &px, &py);
	Check_Pixel(topdown, px, py, 0, 0, 255, "6. depth test rejects far overdraw (blue survives)");

	free(topdown);

	quadrant_tex->Release();
	white_tex->Release();
	cyan_tex->Release();
	blue_tex->Release();
	red_tex->Release();

	DX8Wrapper::Shutdown();

	if (g_AnyFailure)
	{
		fprintf(stderr, "RENDERTEXTUREDTRIANGLE_FAIL: one or more checks failed (see above)\n");
		return 1;
	}

	printf("RENDERTEXTUREDTRIANGLE_OK: all 6 checks passed (%dx%d)\n", W, H);
	return 0;
}
