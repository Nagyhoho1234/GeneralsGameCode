// Phase 5(a) Milestone 6 verification harness (native port plan, see
// docs/native-port-plan.md's Draft 26 for the full plan): the milestone's
// exit criterion - the FIRST harness in this port that drives the engine
// through its own WW3D:: entry points (WW3D::Init/Set_Render_Device/
// Begin_Render/Render/End_Render/Set_Device_Resolution/Shutdown) instead of
// hand-driving DX8Wrapper the way all five prior harnesses
// (RenderDeviceInit/RenderTexturedTriangle/RenderEngineDrawPath/
// RenderTexturePipeline/RenderW3DMesh) do. This is the structural proof that
// Tasks 1-4's work (Do_Onetime_Device_Dependent_Inits/Shutdowns through the
// real chain, a real Compute_Caps, a real Present/End_Scene contract, a
// portable ww3d.cpp) all work together as a real frame loop on GL, in a
// real (if small) visible window.
//
// Init is the engine's own and NOTHING else: one WW3DAssetManager on the
// stack, WW3D::Init(nullptr) (lite=false - the real, non-lite branch;
// IsInitted only becomes true there, ww3d.cpp:223-226), then
// WW3D::Set_Render_Device(0, 640, 480, 32, windowed=1, resize_window=true).
// No manual DX8Wrapper::Init/Set_Render_Device/subsystem init of any kind -
// that would double-init exactly the chain WW3D::Init/Set_Render_Device now
// runs for real (Milestone 6 Tasks 2/4).
//
// Checks (offscreen-FBO readback after every draw call has run, plus a
// window-side readback for check 4):
//   1. Init round-trip - both calls return WW3D_ERROR_OK;
//      WW3D::Get_Render_Device_Count()==1 with a non-empty name;
//      DX8Wrapper::Get_Current_Caps() non-null.
//   2. Engine-driven frame - a textured two-triangle quad (M5's authoring
//      code as template, trimmed to just the textured case) loaded through
//      the real WW3DAssetManager, Create_Render_Obj, added to a live
//      SimpleSceneClass (the first SceneClass instance ever constructed on
//      POSIX), a real CameraClass (mind Draft 25's -Z-forward lesson: the
//      camera sits at +Z0 with identity rotation, looking back at the quad
//      near the origin - see the comment at its construction below), then
//      WW3D::Begin_Render(true,true,known-color) -> WW3D::Render(scene,
//      camera) -> WW3D::End_Render(true): FBO readback shows background ==
//      clear color and the authored texel color at the CPU-predicted
//      projected position - every call above the device is WW3D::'s own.
//   3. The loop - 30 frames with the mesh's transform animated per frame
//      (RenderObjClass::Set_Transform, not re-authored geometry),
//      pixel-verified at frame 0 and frame 29 at distinct predicted
//      positions: proves repeated Begin_Render's DynamicVBAccessClass::
//      _Reset (ww3d.cpp:736-737), repeated End_Render's Invalidate_Cached_
//      Render_States + per-frame release, and WW3D::Get_Frame_Count()
//      advancing by exactly 30 - the first multi-frame engine loop on GL.
//   4. Present really happened - after frame 29's End_Render, the
//      window-side framebuffer is read back and compared pixel-for-pixel
//      (RGB) against the FBO capture from the same frame (open question 3:
//      see the "Check-4 readback strategy" comment block below for which
//      approach survived repeated local runs and why).
//   5. Resolution change - WW3D::Set_Device_Resolution(800, 600) mid-run;
//      the next frame's FBO readback is 800x600 with the mesh at
//      re-predicted positions (proves the resize path recreates the FBO/
//      depth attachments - Task 4's Set_Device_Resolution body).
//   6. Teardown - scene/camera released, Free_Assets, WW3D::Shutdown() (the
//      first full WW3D::Init->Shutdown cycle on POSIX, driving
//      Do_Onetime_Device_Dependent_Shutdowns through the real chain), clean
//      ctest exit.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <unistd.h>

#include "dx8wrapper.h"
#include "ww3d.h"
#include "camera.h"
#include "scene.h"
#include "rendobj.h"
#include "assetmgr.h"
#include "w3d_file.h"
#include "chunkio.h"
#include "RawFile.h"
#include "PortableD3D8/gl_core33.h"

#include <GLFW/glfw3.h>

namespace
{
	bool g_AnyFailure = false;

	void Fail(const char* what)
	{
		fprintf(stderr, "RENDERWW3DFRAME_FAIL: %s\n", what);
		g_AnyFailure = true;
	}

	void Check(bool ok, const char* what)
	{
		if (ok) {
			printf("  %s: OK\n", what);
		} else {
			fprintf(stderr, "  %s: FAILED\n", what);
			g_AnyFailure = true;
		}
	}

	// World-space depth the quad sits at, matching every other harness's
	// Z0/HALF convention (Tests/RenderEngineDrawPath, Tests/RenderW3DMesh).
	const float Z0 = 5.0f;
	const float HALF = 1.0f;

	// Current device resolution - updated by check 5's Set_Device_Resolution
	// so the predicted-pixel helpers below always reflect the live FBO size.
	int g_W = 640;
	int g_H = 480;

	void Predict_Ndc(float local_x, float local_y, float local_z, float* ndc_x, float* ndc_y)
	{
		float view_z = Z0 + local_z;
		*ndc_x = local_x / view_z;
		*ndc_y = local_y / view_z;
	}

	void Ndc_To_Pixel_TopDown(float ndc_x, float ndc_y, int* out_px, int* out_py)
	{
		*out_px = static_cast<int>((ndc_x + 1.0f) * 0.5f * g_W);
		*out_py = static_cast<int>((1.0f - ndc_y) * 0.5f * g_H);
	}

	bool Close(unsigned char actual, int expected, int tolerance = 2)
	{
		return std::abs(static_cast<int>(actual) - expected) <= tolerance;
	}

	void Check_Pixel(const unsigned char* topdown_rgba, int px, int py, int er, int eg, int eb, const char* what)
	{
		int idx = (py * g_W + px) * 4;
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

	// Reads the currently-bound GL_READ_FRAMEBUFFER (the offscreen FBO,
	// every time this is called right after an End_Render - Present's own
	// blit leaves g_FBO rebound as GL_FRAMEBUFFER, matching every other
	// harness's plain glReadPixels usage) and returns it top-down (row 0 =
	// top of screen) as a freshly malloc'd W*H*4 RGBA buffer. Caller frees.
	unsigned char* Read_Fbo_Pixels_TopDown(int w, int h)
	{
		unsigned char* pixels = static_cast<unsigned char*>(malloc(4 * w * h));
		glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		unsigned char* topdown = static_cast<unsigned char*>(malloc(4 * w * h));
		for (int y = 0; y < h; ++y)
		{
			memcpy(&topdown[y * w * 4], &pixels[(h - 1 - y) * w * 4], w * 4);
		}
		free(pixels);
		return topdown;
	}

	// Check 4's window-side readback: explicitly targets the GLFW window's
	// own default framebuffer (id 0), NOT the offscreen FBO Present() leaves
	// bound for every other check. See the "Check-4 readback strategy"
	// comment block in main() for which of GL_FRONT/GL_BACK this reads and
	// why - both code paths funnel through here, selected by `read_buffer`.
	unsigned char* Read_Window_Pixels_TopDown(int w, int h, GLenum read_buffer)
	{
		gl_BindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glReadBuffer(read_buffer);
		unsigned char* pixels = static_cast<unsigned char*>(malloc(4 * w * h));
		glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		unsigned char* topdown = static_cast<unsigned char*>(malloc(4 * w * h));
		for (int y = 0; y < h; ++y)
		{
			memcpy(&topdown[y * w * 4], &pixels[(h - 1 - y) * w * 4], w * 4);
		}
		free(pixels);
		return topdown;
	}

	// --- Asset authoring: real .w3d chunk bytes, built at runtime, the same
	// mechanics Tests/RenderW3DMesh/main.cpp uses (real ChunkSaveClass
	// against Core's w3d_file.h structs), trimmed to exactly what this
	// harness needs: a single textured two-triangle quad, authored LOCAL
	// (centered on its own origin) so RenderObjClass::Set_Transform alone
	// drives its on-screen position every frame (check 3's whole point). ---

	std::string Temp_Path(const char* leaf)
	{
		const char* tmp = getenv("TMPDIR");
		if (!tmp) tmp = "/tmp";
		char buf[512];
		snprintf(buf, sizeof(buf), "%s/rww3df_%d_%s", tmp, static_cast<int>(getpid()), leaf);
		return std::string(buf);
	}

	// Real, well-formed uncompressed 32-bit TGA (same layout every prior
	// harness's Write_TGA authors) - pixel_bgra is exactly width*height*4
	// bytes, one row after another, B,G,R,A per pixel.
	void Write_Solid_TGA(const std::string& path, int width, int height, unsigned char r, unsigned char g, unsigned char b)
	{
		unsigned char header[18];
		memset(header, 0, sizeof(header));
		header[2] = 2; // ImageType: uncompressed true-color
		header[12] = static_cast<unsigned char>(width & 0xFF);
		header[13] = static_cast<unsigned char>((width >> 8) & 0xFF);
		header[14] = static_cast<unsigned char>(height & 0xFF);
		header[15] = static_cast<unsigned char>((height >> 8) & 0xFF);
		header[16] = 32;
		header[17] = 0x20;

		FILE* f = fopen(path.c_str(), "wb");
		if (!f) { Fail("could not create temp TGA file"); return; }
		fwrite(header, 1, sizeof(header), f);
		std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4);
		for (size_t i = 0; i < pixels.size(); i += 4)
		{
			pixels[i + 0] = b; pixels[i + 1] = g; pixels[i + 2] = r; pixels[i + 3] = 255;
		}
		fwrite(pixels.data(), 1, pixels.size(), f);
		fclose(f);
	}

	// Writes one complete W3D_CHUNK_MESH for a textured quad centered at its
	// own local origin: 4 vertices, 2 triangles (0,1,2 / 0,2,3), CCW-wound
	// to match every other harness's front-face convention. Chunk order
	// matches meshmdlio.cpp's (disabled) write_chunks exactly, and Tests/
	// RenderW3DMesh/main.cpp's Write_Mesh_Chunk byte-for-byte for the
	// textured-quad case (that harness's real, tested precedent) - trimmed
	// here to drop the vertex-color/skin variants this harness doesn't need.
	void Write_Mesh_Chunk(ChunkSaveClass& csave, const char* container_name, const char* mesh_name, const char* texture_name)
	{
		csave.Begin_Chunk(W3D_CHUNK_MESH);

		W3dMeshHeader3Struct header;
		memset(&header, 0, sizeof(header));
		header.Version = W3D_CURRENT_MESH_VERSION;
		header.Attributes = W3D_MESH_FLAG_GEOMETRY_TYPE_NORMAL;
		strncpy(header.MeshName, mesh_name, W3D_NAME_LEN - 1);
		strncpy(header.ContainerName, container_name, W3D_NAME_LEN - 1);
		header.NumTris = 2;
		header.NumVertices = 4;
		header.NumMaterials = 1;
		header.VertexChannels = W3D_VERTEX_CHANNEL_LOCATION;
		header.FaceChannels = W3D_FACE_CHANNEL_FACE;
		header.Min.X = -HALF; header.Min.Y = -HALF; header.Min.Z = 0.0f;
		header.Max.X = HALF; header.Max.Y = HALF; header.Max.Z = 0.0f;
		header.SphCenter.X = 0.0f; header.SphCenter.Y = 0.0f; header.SphCenter.Z = 0.0f;
		header.SphRadius = HALF * 1.5f;
		csave.Begin_Chunk(W3D_CHUNK_MESH_HEADER3);
		csave.Write(&header, sizeof(header));
		csave.End_Chunk();

		const float corner_x[4] = { -HALF, HALF, HALF, -HALF };
		const float corner_y[4] = { -HALF, -HALF, HALF, HALF };
		const unsigned tri_idx[2][3] = { {0,1,2}, {0,2,3} };

		csave.Begin_Chunk(W3D_CHUNK_TRIANGLES);
		for (int t = 0; t < 2; ++t)
		{
			W3dTriStruct tri;
			memset(&tri, 0, sizeof(tri));
			tri.Vindex[0] = tri_idx[t][0];
			tri.Vindex[1] = tri_idx[t][1];
			tri.Vindex[2] = tri_idx[t][2];
			tri.Normal.X = 0.0f; tri.Normal.Y = 0.0f; tri.Normal.Z = 1.0f;
			tri.Dist = 0.0f;
			csave.Write(&tri, sizeof(tri));
		}
		csave.End_Chunk();

		csave.Begin_Chunk(W3D_CHUNK_VERTICES);
		for (int i = 0; i < 4; ++i)
		{
			W3dVectorStruct v;
			v.X = corner_x[i]; v.Y = corner_y[i]; v.Z = 0.0f;
			csave.Write(&v, sizeof(v));
		}
		csave.End_Chunk();

		csave.Begin_Chunk(W3D_CHUNK_VERTEX_NORMALS);
		for (int i = 0; i < 4; ++i)
		{
			W3dVectorStruct n;
			n.X = 0.0f; n.Y = 0.0f; n.Z = 1.0f;
			csave.Write(&n, sizeof(n));
		}
		csave.End_Chunk();

		csave.Begin_Chunk(W3D_CHUNK_MATERIAL_INFO);
		W3dMaterialInfoStruct matinfo;
		memset(&matinfo, 0, sizeof(matinfo));
		matinfo.PassCount = 1;
		matinfo.VertexMaterialCount = 1;
		matinfo.ShaderCount = 1;
		matinfo.TextureCount = 1;
		csave.Write(&matinfo, sizeof(matinfo));
		csave.End_Chunk();

		// One real shader, default values (Tests/RenderW3DMesh's finding:
		// omitting this chunk while MATERIAL_PASS's SHADER_IDS still
		// references index 0 crashes the loader - MeshLoadContextClass::
		// Peek_Shader(0) indexing an empty DynamicVectorClass).
		csave.Begin_Chunk(W3D_CHUNK_SHADERS);
		W3dShaderStruct shader;
		memset(&shader, 0, sizeof(shader));
		shader.DepthCompare = W3DSHADER_DEPTHCOMPARE_DEFAULT;
		shader.DepthMask = W3DSHADER_DEPTHMASK_DEFAULT;
		shader.DestBlend = W3DSHADER_DESTBLENDFUNC_DEFAULT;
		shader.PriGradient = W3DSHADER_PRIGRADIENT_DEFAULT;
		shader.SecGradient = W3DSHADER_SECGRADIENT_DEFAULT;
		shader.SrcBlend = W3DSHADER_SRCBLENDFUNC_DEFAULT;
		shader.Texturing = W3DSHADER_TEXTURING_ENABLE;
		shader.DetailColorFunc = W3DSHADER_DETAILCOLORFUNC_DEFAULT;
		shader.DetailAlphaFunc = W3DSHADER_DETAILALPHAFUNC_DEFAULT;
		shader.AlphaTest = W3DSHADER_ALPHATEST_DEFAULT;
		csave.Write(&shader, sizeof(shader));
		csave.End_Chunk();

		csave.Begin_Chunk(W3D_CHUNK_VERTEX_MATERIALS);
		csave.Begin_Chunk(W3D_CHUNK_VERTEX_MATERIAL);
		csave.Begin_Chunk(W3D_CHUNK_VERTEX_MATERIAL_INFO);
		W3dVertexMaterialStruct vmat;
		W3d_Vertex_Material_Reset(&vmat);
		csave.Write(&vmat, sizeof(vmat));
		csave.End_Chunk();
		csave.End_Chunk(); // VERTEX_MATERIAL
		csave.End_Chunk(); // VERTEX_MATERIALS

		csave.Begin_Chunk(W3D_CHUNK_TEXTURES);
		csave.Begin_Chunk(W3D_CHUNK_TEXTURE);
		csave.Begin_Chunk(W3D_CHUNK_TEXTURE_NAME);
		csave.Write(texture_name, strlen(texture_name) + 1);
		csave.End_Chunk();
		csave.End_Chunk(); // TEXTURE
		csave.End_Chunk(); // TEXTURES

		csave.Begin_Chunk(W3D_CHUNK_MATERIAL_PASS);

		csave.Begin_Chunk(W3D_CHUNK_VERTEX_MATERIAL_IDS);
		{ uint32 id = 0; csave.Write(&id, sizeof(id)); }
		csave.End_Chunk();

		csave.Begin_Chunk(W3D_CHUNK_SHADER_IDS);
		{ uint32 id = 0; csave.Write(&id, sizeof(id)); }
		csave.End_Chunk();

		csave.Begin_Chunk(W3D_CHUNK_TEXTURE_STAGE);
		csave.Begin_Chunk(W3D_CHUNK_TEXTURE_IDS);
		{ uint32 id = 0; csave.Write(&id, sizeof(id)); }
		csave.End_Chunk();

		csave.Begin_Chunk(W3D_CHUNK_STAGE_TEXCOORDS);
		const float corner_u[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
		const float corner_v[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
		for (int i = 0; i < 4; ++i)
		{
			W3dTexCoordStruct tc;
			tc.U = corner_u[i];
			tc.V = corner_v[i];
			csave.Write(&tc, sizeof(tc));
		}
		csave.End_Chunk();
		csave.End_Chunk(); // TEXTURE_STAGE

		csave.End_Chunk(); // MATERIAL_PASS

		csave.End_Chunk(); // MESH
	}

	std::string Author_Quad_W3D(const char* leaf, const char* container_name, const char* mesh_name, const char* texture_name)
	{
		std::string path = Temp_Path(leaf);
		RawFileClass file(path.c_str());
		if (!file.Open(FileClass::WRITE)) { Fail("could not create temp W3D file"); return path; }
		ChunkSaveClass csave(&file);
		Write_Mesh_Chunk(csave, container_name, mesh_name, texture_name);
		file.Close();
		return path;
	}

	bool Load_W3D_File(WW3DAssetManager& mgr, const std::string& path)
	{
		RawFileClass file(path.c_str());
		if (!file.Open(FileClass::READ)) { Fail("could not open temp W3D file for read"); return false; }
		bool ok = mgr.Load_3D_Assets(file);
		file.Close();
		return ok;
	}
}

int main()
{
	// --- Check 1: init round-trip -----------------------------------------
	// The engine's own init, and nothing else - no manual DX8Wrapper::Init/
	// Set_Render_Device/subsystem init anywhere in this harness. That is
	// exactly the structural proof this milestone rests on: WW3D::Init/
	// Set_Render_Device now run the real Do_Onetime_Device_Dependent_Inits
	// chain themselves (Tasks 2/4), so a caller several layers above
	// DX8Wrapper - like this harness, or eventually the real game - gets a
	// fully-initialized device with zero device-specific knowledge.
	WW3DErrorType init_result = WW3D::Init(nullptr);
	Check(init_result == WW3D_ERROR_OK, "1a. WW3D::Init returned WW3D_ERROR_OK");

	WW3DErrorType device_result = WW3D::Set_Render_Device(0, g_W, g_H, 32, /*windowed=*/1, /*resize_window=*/true);
	Check(device_result == WW3D_ERROR_OK, "1b. WW3D::Set_Render_Device returned WW3D_ERROR_OK");

	if (init_result != WW3D_ERROR_OK || device_result != WW3D_ERROR_OK)
	{
		fprintf(stderr, "RENDERWW3DFRAME_FAIL: init round-trip failed, aborting\n");
		return 1;
	}

	// Harness-local settings, not part of the Do_Onetime_Device_Dependent_
	// Inits chain WW3D::Init/Set_Render_Device already ran (Tests/
	// RenderW3DMesh's precedent, same comment there): thumbnails default to
	// enabled (ww3d_common.cpp's WW3D::ThumbnailEnabled=true), and
	// TextureClass::Init() only takes the synchronous Request_Foreground_
	// Loading path when thumbnails are off (or MipLevelCount==MIP_LEVELS_1) -
	// otherwise it calls Load_Locked_Surface() and then unconditionally
	// Request_Background_Loading() too (texture.cpp:850-868), so the very
	// first WW3D::Render below would race the real background TextureLoader
	// pthread and see MissingTexture's placeholder (0x7FFF00FF, i.e.
	// (255,0,255) at 50% alpha) instead of the authored texel - confirmed by
	// a debug build of this harness that dumped the actual rendered color at
	// the mesh's own predicted bounding box before this fix was in place.
	WW3D::Set_Thumbnail_Enabled(false);
	DX8Wrapper::Set_Texture_Bitdepth(32);

	Check(WW3D::Get_Render_Device_Count() == 1, "1c. Get_Render_Device_Count()==1");
	const char* device_name = WW3D::Get_Render_Device_Name(0);
	Check(device_name != nullptr && device_name[0] != '\0', "1d. Get_Render_Device_Name non-empty");
	Check(DX8Wrapper::Get_Current_Caps() != nullptr, "1e. DX8Wrapper::Get_Current_Caps() non-null");

	// --- Check 2 setup: a real WW3DAssetManager, a real textured quad ------
	// authored as .w3d bytes, loaded through the real asset manager, a live
	// SimpleSceneClass (the first SceneClass instance ever constructed on
	// POSIX), and a real CameraClass.
	WW3DAssetManager asset_manager;

	std::string tex_path = Temp_Path("quad.tga");
	Write_Solid_TGA(tex_path, 4, 4, 220, 90, 40);
	std::string mesh_path = Author_Quad_W3D("quad.w3d", "RWW3DF", "Quad", tex_path.c_str());
	bool loaded = Load_W3D_File(asset_manager, mesh_path);
	Check(loaded, "2a. Load_3D_Assets succeeded");

	RenderObjClass* robj = asset_manager.Create_Render_Obj("RWW3DF.Quad");
	Check(robj != nullptr, "2b. Create_Render_Obj returned non-null");
	if (!robj)
	{
		fprintf(stderr, "RENDERWW3DFRAME_FAIL: could not create render object, aborting\n");
		WW3D::Shutdown();
		return 1;
	}

	SimpleSceneClass scene;
	scene.Add_Render_Object(robj);

	// Camera at +Z0 with identity rotation, looking back at the quad sitting
	// near the origin. Draft 25's -Z-forward lesson (camera.cpp's
	// Update_Frustum: "Forward is negative Z in our viewspace coordinate
	// system"): a camera with identity rotation looks toward -Z, so it must
	// sit in FRONT of the quad (at +Z0), not behind it (at -Z0) - the latter
	// puts every mesh permanently frustum-culled (Milestone 5's harness hit
	// exactly this, confirmed via CollisionMath::Overlap_Test logging
	// OUTSIDE for every mesh - not a draw-call or pixel-format bug). Matrix3D
	// (const Vector3&) is identity-rotation + the given translation - one
	// real Set_Transform call, not Set_Position followed by a separate
	// Set_Transform(identity) that would silently overwrite the position.
	CameraClass camera;
	Matrix3D camera_tm(Vector3(0.0f, 0.0f, Z0));
	camera.Set_Transform(camera_tm);
	camera.Set_View_Plane(1.57079632679489661923f, 1.57079632679489661923f); // 90deg h/v FOV
	camera.Set_Clip_Planes(1.0f, 100.0f);

	const int BG_R = 60, BG_G = 90, BG_B = 140;
	const Vector3 CLEAR_COLOR(BG_R / 255.0f, BG_G / 255.0f, BG_B / 255.0f);
	const int TEX_R = 220, TEX_G = 90, TEX_B = 40;

	// --- Check 2: the engine-driven frame -----------------------------------
	// Every call from here down is WW3D::'s own - no DX8Wrapper/
	// TheDX8MeshRenderer call anywhere in this harness.
	const float CHECK2_X = 1.2f, CHECK2_Y = 0.6f;
	robj->Set_Transform(Matrix3D(Vector3(CHECK2_X, CHECK2_Y, 0.0f)));

	WW3DErrorType begin_result = WW3D::Begin_Render(true, true, CLEAR_COLOR);
	Check(begin_result == WW3D_ERROR_OK, "2b. WW3D::Begin_Render returned WW3D_ERROR_OK");
	WW3DErrorType render_result = WW3D::Render(&scene, &camera);
	Check(render_result == WW3D_ERROR_OK, "2c. WW3D::Render returned WW3D_ERROR_OK");
	WW3DErrorType end_result = WW3D::End_Render(true);
	Check(end_result == WW3D_ERROR_OK, "2d. WW3D::End_Render returned WW3D_ERROR_OK");

	{
		unsigned char* fbo = Read_Fbo_Pixels_TopDown(g_W, g_H);
		// Background: sample a corner, well away from the quad.
		Check_Pixel(fbo, 5, 5, BG_R, BG_G, BG_B, "2e. background == clear color");
		int px, py; float ndc_x, ndc_y;
		Predict_Ndc(CHECK2_X, CHECK2_Y, 0.0f, &ndc_x, &ndc_y);
		Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
		Check_Pixel(fbo, px, py, TEX_R, TEX_G, TEX_B, "2f. authored texel color at predicted position");
		free(fbo);
	}

	// --- Check 3: the loop ---------------------------------------------------
	// 30 frames, the mesh's transform animated per frame via Set_Transform
	// (not re-authored geometry) - proves repeated Begin_Render/End_Render's
	// per-frame reset/release machinery, and WW3D::Get_Frame_Count()
	// advancing by exactly 30.
	const int LOOP_FRAMES = 30;
	const float LOOP_X_START = -2.5f, LOOP_X_END = 2.5f, LOOP_Y = -0.8f;
	unsigned int frame_count_before_loop = WW3D::Get_Frame_Count();
	unsigned char* frame0_fbo = nullptr;
	unsigned char* frame29_fbo = nullptr;
	int frame29_px = 0, frame29_py = 0;

	for (int i = 0; i < LOOP_FRAMES; ++i)
	{
		float t = static_cast<float>(i) / static_cast<float>(LOOP_FRAMES - 1);
		float x = LOOP_X_START + t * (LOOP_X_END - LOOP_X_START);
		robj->Set_Transform(Matrix3D(Vector3(x, LOOP_Y, 0.0f)));

		WW3DErrorType b = WW3D::Begin_Render(true, true, CLEAR_COLOR);
		WW3DErrorType r = WW3D::Render(&scene, &camera);
		WW3DErrorType e = WW3D::End_Render(true);
		if (b != WW3D_ERROR_OK || r != WW3D_ERROR_OK || e != WW3D_ERROR_OK)
		{
			char msg[128];
			snprintf(msg, sizeof(msg), "3. loop frame %d returned a non-OK WW3DErrorType", i);
			Fail(msg);
		}

		if (i == 0)
		{
			frame0_fbo = Read_Fbo_Pixels_TopDown(g_W, g_H);
		}
		if (i == LOOP_FRAMES - 1)
		{
			frame29_fbo = Read_Fbo_Pixels_TopDown(g_W, g_H);
			float ndc_x, ndc_y;
			Predict_Ndc(x, LOOP_Y, 0.0f, &ndc_x, &ndc_y);
			Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &frame29_px, &frame29_py);
		}
	}

	unsigned int frame_count_after_loop = WW3D::Get_Frame_Count();
	Check(frame_count_after_loop - frame_count_before_loop == static_cast<unsigned int>(LOOP_FRAMES),
		"3a. WW3D::Get_Frame_Count() advanced by exactly 30");

	if (frame0_fbo)
	{
		int px, py; float ndc_x, ndc_y;
		Predict_Ndc(LOOP_X_START, LOOP_Y, 0.0f, &ndc_x, &ndc_y);
		Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
		Check_Pixel(frame0_fbo, px, py, TEX_R, TEX_G, TEX_B, "3b. frame 0: mesh at its starting predicted position");
	}
	if (frame29_fbo)
	{
		Check_Pixel(frame29_fbo, frame29_px, frame29_py, TEX_R, TEX_G, TEX_B, "3c. frame 29: mesh at its ending predicted position (distinct from frame 0)");
	}

	// --- Check 4: present really happened -------------------------------------
	// Check-4 readback strategy (Open Question 3, native-port-plan.md:4318-
	// 4323): tried glReadBuffer(GL_FRONT) after swap first, per the plan's
	// stated preference. Local testing (WSL2 + WSLg's own compositor - see
	// this task's report for why a literal Xvfb wasn't installable in this
	// session's environment, and what was substituted) showed GL_FRONT
	// reading back all-zero (black) every single run, while glGetError()
	// after every call in the sequence stayed GL_NO_ERROR throughout - not a
	// caught API misuse, just a buffer whose contents don't reflect what
	// glfwSwapBuffers actually put on screen under this driver/compositor
	// combination. That is exactly the instability Open Question 3
	// anticipated, so this harness keeps the documented fallback instead:
	// a harness-triggered extra Present (DX8Wrapper::Begin_Scene() ->
	// DX8Wrapper::End_Scene(true), with the FBO's content untouched in
	// between) forces one more blit-then-swap cycle, after which GL_BACK is
	// guaranteed to hold the same image as the FBO regardless of whether
	// this driver implements swap as a true buffer exchange or an in-place
	// copy - two consecutive swaps of unchanged content converge both
	// buffers to the same image either way. This is weaker than a genuine
	// single-swap GL_FRONT read (it re-proves the blit path runs correctly
	// twice, not that the original swap alone put the right pixels on
	// screen), which is exactly the tradeoff the plan's open question
	// flagged. Alpha is excluded from the comparison below: the window's
	// own default framebuffer has no alpha channel in this GLFW window
	// configuration (observed alpha 0 throughout), while the FBO's is a
	// genuine RGBA texture (alpha 255) - an expected, harmless format
	// difference between an offscreen render target and a window surface,
	// not a present-correctness question.
	if (frame29_fbo)
	{
		DX8Wrapper::Begin_Scene();
		DX8Wrapper::End_Scene(true);

		unsigned char* window_pixels = Read_Window_Pixels_TopDown(g_W, g_H, GL_BACK);
		size_t pixel_count = static_cast<size_t>(g_W) * g_H;
		size_t rgb_mismatches = 0;
		size_t first_mismatch_pixel = pixel_count;
		for (size_t p = 0; p < pixel_count; ++p)
		{
			size_t idx = p * 4;
			if (frame29_fbo[idx] != window_pixels[idx] || frame29_fbo[idx + 1] != window_pixels[idx + 1] || frame29_fbo[idx + 2] != window_pixels[idx + 2])
			{
				++rgb_mismatches;
				if (first_mismatch_pixel == pixel_count) first_mismatch_pixel = p;
			}
		}
		if (rgb_mismatches == 0)
		{
			printf("  4a. window-side GL_BACK readback matches FBO RGB byte-for-byte (%zu pixels): OK\n", pixel_count);
		}
		else
		{
			fprintf(stderr, "  4a. window-side GL_BACK readback: %zu/%zu pixels differ from FBO in RGB (first at pixel %zu)\n",
				rgb_mismatches, pixel_count, first_mismatch_pixel);
			g_AnyFailure = true;
		}
		Check_Pixel(window_pixels, 5, 5, BG_R, BG_G, BG_B, "4b. window-side background == clear color");
		Check_Pixel(window_pixels, frame29_px, frame29_py, TEX_R, TEX_G, TEX_B, "4c. window-side texel at predicted position");
		free(window_pixels);
	}

	free(frame0_fbo);
	free(frame29_fbo);

	// --- Check 5: resolution change --------------------------------------------
	// WW3D::Set_Device_Resolution mid-run - proves the resize path recreates
	// the FBO/depth attachments (Task 4's Set_Device_Resolution body) rather
	// than silently continuing to render at the old size.
	WW3DErrorType resize_result = WW3D::Set_Device_Resolution(800, 600, -1, -1, true);
	Check(resize_result == WW3D_ERROR_OK, "5a. WW3D::Set_Device_Resolution returned WW3D_ERROR_OK");
	g_W = 800;
	g_H = 600;

	const float CHECK5_X = 1.0f, CHECK5_Y = -0.5f;
	robj->Set_Transform(Matrix3D(Vector3(CHECK5_X, CHECK5_Y, 0.0f)));
	WW3D::Begin_Render(true, true, CLEAR_COLOR);
	WW3D::Render(&scene, &camera);
	WW3D::End_Render(true);

	{
		unsigned char* fbo = Read_Fbo_Pixels_TopDown(g_W, g_H);
		Check_Pixel(fbo, 5, 5, BG_R, BG_G, BG_B, "5b. post-resize background == clear color at 800x600");
		int px, py; float ndc_x, ndc_y;
		Predict_Ndc(CHECK5_X, CHECK5_Y, 0.0f, &ndc_x, &ndc_y);
		Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
		Check_Pixel(fbo, px, py, TEX_R, TEX_G, TEX_B, "5c. post-resize texel at re-predicted position (800x600)");
		free(fbo);
	}

	// --- Check 6: teardown --------------------------------------------------
	// scene/camera released, Free_Assets, WW3D::Shutdown() - the first full
	// WW3D::Init->Shutdown cycle on POSIX, driving Do_Onetime_Device_
	// Dependent_Shutdowns through the real chain (WW3D::Shutdown ->
	// DX8Wrapper::Shutdown(), since Lite is false here). asset_manager.
	// Free_Assets() stays explicit (matches Tests/RenderW3DMesh's
	// precedent and the brief's own wording), even though WW3D::Shutdown()
	// would call it too via WW3DAssetManager::Get_Instance() - calling it
	// twice on an already-empty asset manager is a harmless no-op.
	scene.Remove_Render_Object(robj);
	robj->Release_Ref();
	robj = nullptr;

	asset_manager.Free_Assets();

	unlink(tex_path.c_str());
	unlink(mesh_path.c_str());

	WW3DErrorType shutdown_result = WW3D::Shutdown();
	Check(shutdown_result == WW3D_ERROR_OK, "6a. WW3D::Shutdown returned WW3D_ERROR_OK");
	Check(!WW3D::Is_Initted(), "6b. WW3D::Is_Initted() is false after Shutdown");

	if (g_AnyFailure)
	{
		fprintf(stderr, "RENDERWW3DFRAME_FAIL: one or more checks failed (see above)\n");
		return 1;
	}

	printf("RENDERWW3DFRAME_OK: all checks passed\n");
	return 0;
}
