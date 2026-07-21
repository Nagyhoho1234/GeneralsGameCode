// Phase 5(a) Milestone 3 verification harness (native port plan, see
// docs/native-port-plan.md's Draft 20 for the full plan): drives the REAL
// engine draw path end-to-end - DX8VertexBufferClass/DX8IndexBufferClass
// filled through real WriteLockClass objects, DX8Wrapper::Set_Vertex_Buffer/
// Set_Index_Buffer, DX8Wrapper::Set_Shader with real ShaderClass presets,
// DX8Wrapper::Set_Material (a real VertexMaterialClass and the null path),
// DX8Wrapper::Set_Transform(WORLD/VIEW) plus a real D3D-style perspective
// projection, DX8Wrapper::Draw_Triangles - not the device-object-level API
// Tests/RenderTexturedTriangle drove (Milestone 2's scope). Sibling to
// RenderDeviceInit/RenderTexturedTriangle, not an extension of either -
// both stay untouched regression tests.
//
// All geometry is authored in a shared "local" space: WORLD is a real
// translate-by-(0,0,Z0) matrix (Set_Transform(D3DTS_WORLD,...)), VIEW is a
// real identity matrix (Set_Transform(D3DTS_VIEW,...)), and PROJECTION is a
// real D3D-style perspective (D3DXMatrixPerspectiveFovLH's formula,
// fovY=90deg/aspect=1 so xScale=yScale=1) fed through Set_Transform(D3DTS_
// PROJECTION,...) - deliberately NOT Set_Fixed_Function_MVP (that sidesteps
// exactly the SetTransform/Apply_Render_State_Changes/GL-composition chain
// this milestone built and exists to test). Predict_Ndc below independently
// recomputes, in plain scalar CPU math, the same D3D row-vector projection
// formula the engine's own Multiply_D3DMATRIX + Convert_D3D_Projection_To_GL
// composition performs on the GPU-bound matrix - two separately-written
// implementations of the same math, not one checking itself.
//
// Six real pixel-level checks (offscreen-FBO readback after every draw call
// has run), matching Milestones 1-2's verification bar:
//   1. Background-color integrity.
//   2. An opaque quad sampled at numerically predicted projected pixel
//      positions - validates the whole real Set_Transform -> Apply_Render_
//      State_Changes -> GL SetTransform -> DrawIndexedPrimitive's MVP
//      composition -> GLSL clip-space chain against independent CPU-side
//      math (spike-style), not just "something rendered."
//   3. Vertex-diffuse color exactness through the white fallback texture
//      (Milestone 3 step 7) - proves untextured real-engine draws still
//      resolve MODULATE(TEXTURE,DIFFUSE) to the exact diffuse color.
//   4. Additive-blend sum check: two quads at the identical footprint, both
//      drawn with the real ShaderClass::_PresetAdditiveShader - proves
//      ShaderClass::Apply() (portable since step 5) drives real GL blend
//      state (step 7) through the real draw path (step 6), not a synthetic
//      SetRenderState call.
//   5. Depth occlusion with the depth states coming from the real shader
//      vocabulary (_PresetOpaqueShader's PASS_LEQUAL/DEPTH_WRITE_ENABLE
//      bits, decoded by shader.cpp, applied by Apply_Render_State_Changes) -
//      a near quad must occlude a far quad drawn second at (nearly) the
//      same footprint.
//   6. A second draw through DynamicVBAccessClass/DynamicIBAccessClass
//      (BUFFER_TYPE_DYNAMIC_DX8, dynamic_fvf_type) renders its content at
//      its own designated location - proves the VertexBufferOffset/
//      IndexBufferOffset plumbing glDrawElementsBaseVertex was built for
//      (Milestone 2) is now driven by real engine code (dazzle.cpp's own
//      DynamicVBAccessClass/DynamicIBAccessClass usage pattern), not just
//      Milestone 2's hand-rolled Draw_Sorting_IB_VB caller.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "dx8wrapper.h"
#include "dx8vertexbuffer.h"
#include "dx8indexbuffer.h"
#include "shader.h"
#include "vertmaterial.h"
#include "PortableD3D8/gl_core33.h"
#include "PortableD3D8/gl_fixed_function.h"

#include <GLFW/glfw3.h>

namespace
{
	const int W = 256, H = 256;
	bool g_AnyFailure = false;

	void Fail(const char* what)
	{
		fprintf(stderr, "RENDERENGINEDRAWPATH_FAIL: %s\n", what);
		g_AnyFailure = true;
	}

	// Matches DX8_FVF_XYZDUV1 (D3DFVF_XYZ|D3DFVF_TEX1|D3DFVF_DIFFUSE) - the
	// texcoord is never actually sampled meaningfully (every draw below
	// leaves stage 0 untextured, exercising the step 7 white fallback), so
	// (0,0)/(1,1) corners are just placeholders satisfying the FVF's layout.
	struct Vertex
	{
		float x, y, z;
		DWORD diffuse;
		float u, v;
	};
	const DWORD QUAD_FVF = D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_DIFFUSE;
	const unsigned short QUAD_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

	// World-space depth the whole scene sits at (WORLD = translate(0,0,Z0)).
	const float Z0 = 5.0f;
	// Local half-size; with xScale=yScale=1 (see Predict_Ndc), NDC half-
	// extent = HALF/Z0 = 0.25, matching Tests/RenderTexturedTriangle's
	// H_SIZE for a visually consistent-sized quad.
	const float HALF = 1.25f;

	// CCW-wound (bl,br,tr,tl) so the real ShaderClass presets' default
	// D3DCULL_CW cull mode (see shader.cpp's _PolygonCullMode) keeps these
	// as front-facing, surviving faces - GL's own default front-face
	// winding, same reasoning as Tests/RenderTexturedTriangle's Make_Tri.
	void Make_Quad(float local_x, float local_y, float local_z, DWORD diffuse, Vertex out[4])
	{
		out[0] = { local_x - HALF, local_y - HALF, local_z, diffuse, 0.0f, 0.0f };
		out[1] = { local_x + HALF, local_y - HALF, local_z, diffuse, 1.0f, 0.0f };
		out[2] = { local_x + HALF, local_y + HALF, local_z, diffuse, 1.0f, 1.0f };
		out[3] = { local_x - HALF, local_y + HALF, local_z, diffuse, 0.0f, 1.0f };
	}

	// Independent CPU-side re-derivation of this harness's exact D3D-style
	// perspective (fovY=90deg -> tan(45deg)=1 -> xScale=yScale=1) composed
	// with WORLD=translate(0,0,Z0) and VIEW=identity: view_z = Z0+local_z,
	// NDC = (local_x, local_y) / view_z. Deliberately hand-derived here, not
	// calling Multiply_D3DMATRIX/Convert_D3D_Projection_To_GL/To_Matrix4x4 -
	// see the file header comment.
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

	// --- Real D3D-style matrix construction (D3D row-vector convention,
	// v' = v*M) - converted to Matrix4x4 via WWMath's To_Matrix4x4 (an exact
	// transpose) before reaching DX8Wrapper::Set_Transform, which converts
	// back via To_D3DMATRIX (the inverse transpose), round-tripping these
	// exact values into DX8Wrapper::render_state.world/view and the GL
	// device's g_ProjectionMatrix bit-for-bit. -----------------------------

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

	// D3DXMatrixPerspectiveFovLH's documented formula.
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

	// Draws Make_Quad(local_x,local_y,local_z,diffuse) through the real
	// engine draw path: DX8VertexBufferClass/DX8IndexBufferClass filled via
	// real WriteLockClass objects, DX8Wrapper::Set_Vertex_Buffer/Set_Index_
	// Buffer/Set_Shader/Set_Material/Draw_Triangles - never the device-
	// object-level API Tests/RenderTexturedTriangle used.
	void Draw_Real_Quad(float local_x, float local_y, float local_z, DWORD diffuse,
		const ShaderClass& shader, VertexMaterialClass* material)
	{
		Vertex verts[4];
		Make_Quad(local_x, local_y, local_z, diffuse, verts);

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
		DX8Wrapper::Set_Shader(shader);
		DX8Wrapper::Set_Material(material);
		// Stage 0 is left untextured (render_state.Textures[0] starts null
		// and nothing here ever sets a real one) - exercises the step 7
		// white fallback unconditionally, no explicit Set_Texture(0,
		// nullptr) call needed (that would be a no-op against the already-
		// null default anyway).
		DX8Wrapper::Draw_Triangles(0, 2, 0, 4);

		vb->Release_Ref();
		ib->Release_Ref();
	}

	// Region E (check 6): the same quad shape, but through DynamicVBAccessClass/
	// DynamicIBAccessClass - dazzle.cpp's own usage pattern
	// (DynamicVBAccessClass(BUFFER_TYPE_DYNAMIC_DX8,dynamic_fvf_type,count),
	// DynamicIBAccessClass(BUFFER_TYPE_DYNAMIC_DX8,count)) - rather than
	// DX8VertexBufferClass/DX8IndexBufferClass.
	void Draw_Dynamic_Quad(float local_x, float local_y, float local_z, DWORD diffuse, const ShaderClass& shader)
	{
		DynamicVBAccessClass vb_access(BUFFER_TYPE_DYNAMIC_DX8, dynamic_fvf_type, 4);
		{
			DynamicVBAccessClass::WriteLockClass lock(&vb_access);
			VertexFormatXYZNDUV2* v = lock.Get_Formatted_Vertex_Array();
			const float corner_x[4] = { local_x - HALF, local_x + HALF, local_x + HALF, local_x - HALF };
			const float corner_y[4] = { local_y - HALF, local_y - HALF, local_y + HALF, local_y + HALF };
			const float corner_u[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
			const float corner_v[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
			// Set every field by name (dx8fvf.h's VertexFormatXYZNDUV2:
			// x,y,z,nx,ny,nz,diffuse,u1,v1,u2,v2) - an aggregate initializer
			// here would be one fragile mistake away from silently packing
			// the wrong field.
			for (int i = 0; i < 4; ++i)
			{
				v[i].x = corner_x[i];
				v[i].y = corner_y[i];
				v[i].z = local_z;
				v[i].nx = 0.0f; v[i].ny = 0.0f; v[i].nz = 1.0f;
				v[i].diffuse = diffuse;
				v[i].u1 = corner_u[i]; v[i].v1 = corner_v[i];
				v[i].u2 = 0.0f; v[i].v2 = 0.0f;
			}
		}

		DynamicIBAccessClass ib_access(BUFFER_TYPE_DYNAMIC_DX8, 6);
		{
			DynamicIBAccessClass::WriteLockClass lock(&ib_access);
			memcpy(lock.Get_Index_Array(), QUAD_INDICES, sizeof(QUAD_INDICES));
		}

		DX8Wrapper::Set_Vertex_Buffer(vb_access);
		DX8Wrapper::Set_Index_Buffer(ib_access, 0);
		DX8Wrapper::Set_Shader(shader);
		DX8Wrapper::Set_Material(nullptr); // the null path
		DX8Wrapper::Draw_Triangles(0, 2, 0, 4);
	}
}

int main()
{
	if (!DX8Wrapper::Init(nullptr))
	{
		fprintf(stderr, "RENDERENGINEDRAWPATH_FAIL: DX8Wrapper::Init failed\n");
		return 1;
	}

	if (!DX8Wrapper::Set_Render_Device(0, W, H, 32, 0))
	{
		fprintf(stderr, "RENDERENGINEDRAWPATH_FAIL: DX8Wrapper::Set_Render_Device failed\n");
		DX8Wrapper::Shutdown();
		return 1;
	}

	// Not driven by any ShaderClass preset (shader.cpp never touches
	// D3DRS_ZENABLE, only ZFUNC/ZWRITEENABLE) - same as Tests/
	// RenderTexturedTriangle's explicit device->SetRenderState call, just
	// through the wrapper.
	DX8Wrapper::Set_DX8_Render_State(D3DRS_ZENABLE, TRUE);

	DX8Wrapper::Begin_Scene();
	DX8Wrapper::Clear(true, true, Vector3(0.5f, 0.5f, 0.5f));

	// Real Set_Transform(WORLD/VIEW) + a real D3D-style perspective
	// projection (see the file header comment and Predict_Ndc).
	DX8Wrapper::Set_Transform(D3DTS_WORLD, To_Matrix4x4(Make_D3D_Translation(0.0f, 0.0f, Z0)));
	DX8Wrapper::Set_Transform(D3DTS_VIEW, To_Matrix4x4(Make_D3D_Identity()));
	const float FOV_Y_90_DEGREES = 1.57079632679489661923f; // pi/2, avoids relying on M_PI's portability
	DX8Wrapper::Set_Transform(D3DTS_PROJECTION, To_Matrix4x4(Make_D3D_PerspectiveFovLH(
		FOV_Y_90_DEGREES, 1.0f, 1.0f, 100.0f)));

	VertexMaterialClass* material = new VertexMaterialClass();
	material->Set_Ambient(1.0f, 1.0f, 1.0f);
	material->Set_Diffuse(1.0f, 1.0f, 1.0f);

	// Region layout: 3x2 grid in local (pre-WORLD-translate) space, chosen
	// so Predict_Ndc's NDC centers land on the same (+-0.6,+-0.6)/(0,-0.6)
	// grid Tests/RenderTexturedTriangle uses (local = ndc * Z0).

	// Region A (-3.0, 3.0): check 2, real Set_Material.
	Draw_Real_Quad(-3.0f, 3.0f, 0.0f, D3DCOLOR_XRGB(200, 100, 50), ShaderClass::_PresetOpaqueShader, material);

	// Region B (3.0, 3.0): check 3, white-fallback diffuse exactness, the null Set_Material path.
	Draw_Real_Quad(3.0f, 3.0f, 0.0f, D3DCOLOR_XRGB(255, 0, 0), ShaderClass::_PresetOpaqueShader, nullptr);

	// Region C (-3.0, -3.0): check 4, additive-blend sum - two draws, same
	// footprint. Deliberately low intensities (64, not 128): additive
	// blending sums onto whatever is already in the framebuffer, including
	// the gray (127,127,127) Clear() left behind, and 127+128+128 would
	// clamp to 255 - losing the "did it actually add, not just overdraw"
	// signal the check exists to prove.
	Draw_Real_Quad(-3.0f, -3.0f, 0.0f, D3DCOLOR_XRGB(64, 0, 0), ShaderClass::_PresetAdditiveShader, nullptr);
	Draw_Real_Quad(-3.0f, -3.0f, 0.0f, D3DCOLOR_XRGB(0, 64, 0), ShaderClass::_PresetAdditiveShader, nullptr);

	// Region D (0.0, -3.0): check 5, depth occlusion - near (blue) drawn
	// first, far (red) drawn second at a footprint offset only in local Z
	// (small relative to Z0, so the two footprints still overlap almost
	// exactly in screen space - see the file header comment).
	Draw_Real_Quad(0.0f, -3.0f, -0.1f, D3DCOLOR_XRGB(0, 0, 255), ShaderClass::_PresetOpaqueShader, nullptr); // near
	Draw_Real_Quad(0.0f, -3.0f, 0.1f, D3DCOLOR_XRGB(255, 0, 0), ShaderClass::_PresetOpaqueShader, nullptr);  // far

	// Region E (3.0, -3.0): check 6, DynamicVBAccessClass/DynamicIBAccessClass.
	Draw_Dynamic_Quad(3.0f, -3.0f, 0.0f, D3DCOLOR_XRGB(0, 255, 255), ShaderClass::_PresetOpaqueShader);

	DX8Wrapper::End_Scene(false);

	unsigned char* pixels = static_cast<unsigned char*>(malloc(4 * W * H));
	glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// glReadPixels is bottom-up; flip once so every check below can reason
	// in top-down pixel coordinates (matching Ndc_To_Pixel_TopDown).
	unsigned char* topdown = static_cast<unsigned char*>(malloc(4 * W * H));
	for (int y = 0; y < H; ++y)
	{
		memcpy(&topdown[y * W * 4], &pixels[(H - 1 - y) * W * 4], W * 4);
	}
	free(pixels);

	int px, py;
	float ndc_x, ndc_y;

	// 1. Background-color integrity.
	Ndc_To_Pixel_TopDown(0.0f, 0.9f, &px, &py);
	Check_Pixel(topdown, px, py, 128, 128, 128, "1. background integrity");

	// 2. Real perspective projection: numerically predicted pixel position.
	Predict_Ndc(-3.0f, 3.0f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 200, 100, 50, "2. real projection: predicted position + color");

	// 3. White-fallback diffuse exactness.
	Predict_Ndc(3.0f, 3.0f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 255, 0, 0, "3. white-fallback diffuse exactness");

	// 4. Additive-blend sum: background(127,127,127) + (64,0,0) + (0,64,0) = (191,191,127).
	Predict_Ndc(-3.0f, -3.0f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 191, 191, 127, "4. additive-blend sum via real ShaderClass preset");

	// 5. Depth occlusion: near (blue) must survive over far (red).
	Predict_Ndc(0.0f, -3.0f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 0, 0, 255, "5. depth occlusion via real shader vocabulary (near survives)");

	// 6. Dynamic-buffer draw's content at its own location.
	Predict_Ndc(3.0f, -3.0f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 0, 255, 255, "6. DynamicVBAccessClass/DynamicIBAccessClass offset plumbing");

	free(topdown);

	DX8Wrapper::Set_Vertex_Buffer(nullptr);
	DX8Wrapper::Set_Index_Buffer(nullptr, 0);
	DX8Wrapper::Set_Material(nullptr);
	material->Release_Ref();

	DX8Wrapper::Shutdown();

	if (g_AnyFailure)
	{
		fprintf(stderr, "RENDERENGINEDRAWPATH_FAIL: one or more checks failed (see above)\n");
		return 1;
	}

	printf("RENDERENGINEDRAWPATH_OK: all 6 checks passed (%dx%d)\n", W, H);
	return 0;
}
