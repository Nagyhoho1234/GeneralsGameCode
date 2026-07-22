// Phase 5(a) Milestone 5 verification harness (native port plan, see
// docs/native-port-plan.md's Draft 24 for the full plan): the exit
// criterion - real .w3d bytes, authored at runtime through the real
// ChunkSaveClass against Core's now-unified w3d_file.h structs, loaded
// through a real WW3DAssetManager (Load_3D_Assets/Create_Render_Obj),
// rendered by the real dx8renderer.cpp mesh pipeline
// (DX8FVFCategoryContainer/DX8PolygonRendererClass/TheDX8MeshRenderer).
// Sibling to RenderDeviceInit/RenderTexturedTriangle/RenderEngineDrawPath/
// RenderTexturePipeline, not an extension of any of them.
//
// Checks (offscreen-FBO readback after every draw call has run):
//   1. Load round-trip - Load_3D_Assets of a single-mesh W3D; Create_Render_Obj
//      returns a real MeshClass with the authored vertex/poly counts.
//   2. Textured lit rigid mesh - a two-triangle quad, XYZNUV1 FVF, texture
//      chunk -> Load_Texture -> Get_Texture -> foreground TGA load;
//      Render(rinfo) + Flush(); sampled pixels equal the authored texel
//      colors at CPU-predicted projected positions.
//   3. Get_Texture cache path - a second mesh referencing the same texture
//      name: Texture_Hash() holds exactly one entry and both meshes render.
//   4. Vertex-color mesh - a mesh whose material pass carries per-vertex
//      DCG with lighting off: rendered colors equal the authored vertex
//      colors exactly (pins the diffuse path against Milestone 5 Step 4's
//      white rule).
//   5. HLod hierarchy - two pivots (second translated) + two sub-meshes:
//      Create_Render_Obj yields an HLodClass, both meshes render at
//      pivot-offset-predicted distinct positions.
//   6. Skin - a skin-flagged mesh with vertex influences on the translated
//      pivot, inside the HLod: rendered at the bone-transformed predicted
//      position.
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
#include "camera.h"
#include "rinfo.h"
#include "assetmgr.h"
#include "mesh.h"
#include "meshmdl.h"
#include "hlod.h"
#include "rendobj.h"
#include "dx8renderer.h"
#include "missingtexture.h"
#include "texturefilter.h"
#include "textureloader.h"
#include "w3d_file.h"
#include "chunkio.h"
#include "RawFile.h"
#include "PortableD3D8/gl_core33.h"
#include "PortableD3D8/gl_fixed_function.h"

#include <GLFW/glfw3.h>

namespace
{
	const int W = 256, H = 256;
	bool g_AnyFailure = false;

	void Fail(const char* what)
	{
		fprintf(stderr, "RENDERW3DMESH_FAIL: %s\n", what);
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

	// World-space depth the whole scene sits at, matching Tests/
	// RenderEngineDrawPath's Z0/HALF convention.
	const float Z0 = 5.0f;
	const float HALF = 1.25f;

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

	// --- Asset authoring: real file-format bytes, built at runtime --------

	std::string Temp_Path(const char* leaf)
	{
		const char* tmp = getenv("TMPDIR");
		if (!tmp) tmp = "/tmp";
		char buf[512];
		snprintf(buf, sizeof(buf), "%s/rw3dm_%d_%s", tmp, static_cast<int>(getpid()), leaf);
		return std::string(buf);
	}

	// Real, well-formed uncompressed 32-bit TGA (same layout Tests/
	// RenderTexturePipeline's Write_TGA authors) - pixel_bgra is exactly
	// width*height*4 bytes, one row after another, B,G,R,A per pixel.
	void Write_TGA(const std::string& path, int width, int height, const unsigned char* pixel_bgra)
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

	// --- W3D authoring: real chunk bytes via the real ChunkSaveClass,
	// mirroring meshmdlio.cpp's (disabled, reference-only) write_chunks
	// family and hlod.cpp's Save_W3D byte-for-byte, so the load side
	// (meshmdlio.cpp's real, live Load_W3D/read_chunks) parses genuine,
	// spec-shaped bytes - no synthetic/mocked format. -----------------

	// A single quad mesh: 4 vertices, 2 triangles (0,1,2 / 0,2,3), CCW-wound
	// to match every other harness's front-face convention. local_z is
	// constant (flat quad, matches Tests/RenderEngineDrawPath's Make_Quad).
	struct QuadMeshDesc
	{
		const char* container_name;
		const char* mesh_name;
		float local_x, local_y, local_z;
		bool textured;			// XYZNUV1 diffuse-less FVF (finding 6(i))
		bool vertex_color;		// DCG chunk, lighting off (check 4)
		DWORD diffuse_argb;		// only used if vertex_color
		const char* texture_name; // only used if textured
		bool skin;					// vertex influences on BoneIdx (check 6)
		unsigned bone_idx;
	};

	// Writes one complete W3D_CHUNK_MESH for the given quad description.
	// Chunk order matches meshmdlio.cpp's (disabled) write_chunks exactly:
	// header, triangles, vertices, normals, [influences], material_info,
	// vertex_materials, [shaders], [textures], material_pass(es).
	void Write_Mesh_Chunk(ChunkSaveClass& csave, const QuadMeshDesc& d)
	{
		csave.Begin_Chunk(W3D_CHUNK_MESH);

		// --- MESH_HEADER3 ---
		W3dMeshHeader3Struct header;
		memset(&header, 0, sizeof(header));
		header.Version = W3D_CURRENT_MESH_VERSION;
		header.Attributes = d.skin ? W3D_MESH_FLAG_GEOMETRY_TYPE_SKIN : W3D_MESH_FLAG_GEOMETRY_TYPE_NORMAL;
		strncpy(header.MeshName, d.mesh_name, W3D_NAME_LEN - 1);
		strncpy(header.ContainerName, d.container_name, W3D_NAME_LEN - 1);
		header.NumTris = 2;
		header.NumVertices = 4;
		header.NumMaterials = 1;
		header.VertexChannels = W3D_VERTEX_CHANNEL_LOCATION;
		header.FaceChannels = W3D_FACE_CHANNEL_FACE;
		header.Min.X = d.local_x - HALF; header.Min.Y = d.local_y - HALF; header.Min.Z = d.local_z;
		header.Max.X = d.local_x + HALF; header.Max.Y = d.local_y + HALF; header.Max.Z = d.local_z;
		header.SphCenter.X = d.local_x; header.SphCenter.Y = d.local_y; header.SphCenter.Z = d.local_z;
		header.SphRadius = HALF * 1.5f;
		csave.Begin_Chunk(W3D_CHUNK_MESH_HEADER3);
		csave.Write(&header, sizeof(header));
		csave.End_Chunk();

		// --- TRIANGLES --- (CCW: (0,1,2),(0,2,3), matching Make_Quad's
		// bl,br,tr,tl corner order used by every other harness)
		const float corner_x[4] = { d.local_x - HALF, d.local_x + HALF, d.local_x + HALF, d.local_x - HALF };
		const float corner_y[4] = { d.local_y - HALF, d.local_y - HALF, d.local_y + HALF, d.local_y + HALF };
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
			tri.Dist = d.local_z;
			csave.Write(&tri, sizeof(tri));
		}
		csave.End_Chunk();

		// --- VERTICES ---
		csave.Begin_Chunk(W3D_CHUNK_VERTICES);
		for (int i = 0; i < 4; ++i)
		{
			W3dVectorStruct v;
			v.X = corner_x[i]; v.Y = corner_y[i]; v.Z = d.local_z;
			csave.Write(&v, sizeof(v));
		}
		csave.End_Chunk();

		// --- VERTEX_NORMALS ---
		csave.Begin_Chunk(W3D_CHUNK_VERTEX_NORMALS);
		for (int i = 0; i < 4; ++i)
		{
			W3dVectorStruct n;
			n.X = 0.0f; n.Y = 0.0f; n.Z = 1.0f;
			csave.Write(&n, sizeof(n));
		}
		csave.End_Chunk();

		// --- VERTEX_INFLUENCES (skin only) ---
		if (d.skin)
		{
			csave.Begin_Chunk(W3D_CHUNK_VERTEX_INFLUENCES);
			for (int i = 0; i < 4; ++i)
			{
				W3dVertInfStruct vinf;
				memset(&vinf, 0, sizeof(vinf));
				vinf.BoneIdx = static_cast<uint16>(d.bone_idx);
				csave.Write(&vinf, sizeof(vinf));
			}
			csave.End_Chunk();
		}

		// --- MATERIAL_INFO ---
		csave.Begin_Chunk(W3D_CHUNK_MATERIAL_INFO);
		W3dMaterialInfoStruct matinfo;
		memset(&matinfo, 0, sizeof(matinfo));
		matinfo.PassCount = 1;
		matinfo.VertexMaterialCount = 1;
		matinfo.ShaderCount = 1;
		matinfo.TextureCount = d.textured ? 1 : 0;
		csave.Write(&matinfo, sizeof(matinfo));
		csave.End_Chunk();

		// --- SHADERS (one, real defaults - every real W3D material pass
		// references a shader by index, even an "untextured opaque" one;
		// omitting this chunk entirely while MATERIAL_PASS's SHADER_IDS
		// still references index 0 is what crashed the loader the first
		// time this harness ran - MeshLoadContextClass::Peek_Shader(0)
		// indexing an empty DynamicVectorClass, undefined behavior with
		// NDEBUG's bounds-check asserts compiled out) ---
		csave.Begin_Chunk(W3D_CHUNK_SHADERS);
		W3dShaderStruct shader;
		memset(&shader, 0, sizeof(shader));
		shader.DepthCompare = W3DSHADER_DEPTHCOMPARE_DEFAULT;
		shader.DepthMask = W3DSHADER_DEPTHMASK_DEFAULT;
		shader.DestBlend = W3DSHADER_DESTBLENDFUNC_DEFAULT;
		shader.PriGradient = W3DSHADER_PRIGRADIENT_DEFAULT;
		shader.SecGradient = W3DSHADER_SECGRADIENT_DEFAULT;
		shader.SrcBlend = W3DSHADER_SRCBLENDFUNC_DEFAULT;
		shader.Texturing = d.textured ? W3DSHADER_TEXTURING_ENABLE : W3DSHADER_TEXTURING_DEFAULT;
		shader.DetailColorFunc = W3DSHADER_DETAILCOLORFUNC_DEFAULT;
		shader.DetailAlphaFunc = W3DSHADER_DETAILALPHAFUNC_DEFAULT;
		shader.AlphaTest = W3DSHADER_ALPHATEST_DEFAULT;
		csave.Write(&shader, sizeof(shader));
		csave.End_Chunk();

		// --- VERTEX_MATERIALS (one, default-reset) ---
		csave.Begin_Chunk(W3D_CHUNK_VERTEX_MATERIALS);
		csave.Begin_Chunk(W3D_CHUNK_VERTEX_MATERIAL);
		csave.Begin_Chunk(W3D_CHUNK_VERTEX_MATERIAL_INFO);
		W3dVertexMaterialStruct vmat;
		W3d_Vertex_Material_Reset(&vmat);
		csave.Write(&vmat, sizeof(vmat));
		csave.End_Chunk();
		csave.End_Chunk(); // VERTEX_MATERIAL
		csave.End_Chunk(); // VERTEX_MATERIALS

		// --- TEXTURES (optional) ---
		if (d.textured)
		{
			csave.Begin_Chunk(W3D_CHUNK_TEXTURES);
			csave.Begin_Chunk(W3D_CHUNK_TEXTURE);
			csave.Begin_Chunk(W3D_CHUNK_TEXTURE_NAME);
			csave.Write(d.texture_name, strlen(d.texture_name) + 1);
			csave.End_Chunk();
			csave.End_Chunk(); // TEXTURE
			csave.End_Chunk(); // TEXTURES
		}

		// --- MATERIAL_PASS ---
		csave.Begin_Chunk(W3D_CHUNK_MATERIAL_PASS);

		csave.Begin_Chunk(W3D_CHUNK_VERTEX_MATERIAL_IDS);
		{ uint32 id = 0; csave.Write(&id, sizeof(id)); }
		csave.End_Chunk();

		csave.Begin_Chunk(W3D_CHUNK_SHADER_IDS);
		{ uint32 id = 0; csave.Write(&id, sizeof(id)); }
		csave.End_Chunk();

		if (d.vertex_color)
		{
			csave.Begin_Chunk(W3D_CHUNK_DCG);
			W3dRGBAStruct color;
			color.R = static_cast<uint8>((d.diffuse_argb >> 16) & 0xFF);
			color.G = static_cast<uint8>((d.diffuse_argb >> 8) & 0xFF);
			color.B = static_cast<uint8>(d.diffuse_argb & 0xFF);
			color.A = static_cast<uint8>((d.diffuse_argb >> 24) & 0xFF);
			for (int i = 0; i < 4; ++i) csave.Write(&color, sizeof(color));
			csave.End_Chunk();
		}

		if (d.textured)
		{
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
		}

		csave.End_Chunk(); // MATERIAL_PASS

		csave.End_Chunk(); // MESH
	}

	// Authors a temp .w3d file containing exactly one mesh chunk, returns
	// its path.
	std::string Author_Single_Mesh_W3D(const char* leaf, const QuadMeshDesc& d)
	{
		std::string path = Temp_Path(leaf);
		RawFileClass file(path.c_str());
		if (!file.Open(FileClass::WRITE)) { Fail("could not create temp W3D file"); return path; }
		ChunkSaveClass csave(&file);
		Write_Mesh_Chunk(csave, d);
		file.Close();
		return path;
	}

	// Loads a single-mesh (or any) .w3d file into the given asset manager.
	bool Load_W3D_File(WW3DAssetManager& mgr, const std::string& path)
	{
		RawFileClass file(path.c_str());
		if (!file.Open(FileClass::READ)) { Fail("could not open temp W3D file for read"); return false; }
		bool ok = mgr.Load_3D_Assets(file);
		file.Close();
		return ok;
	}

	// Draws a real render object through the real mesh pipeline:
	// Render(rinfo) queues it, TheDX8MeshRenderer.Flush() actually issues
	// the GL draw calls (dx8renderer.cpp's real FVF-container/polygon-
	// renderer path, not the device-object-level or DX8VertexBufferClass-
	// level APIs the older four harnesses drove directly).
	void Render_Obj(RenderObjClass* robj, RenderInfoClass& rinfo)
	{
		robj->Render(rinfo);
		TheDX8MeshRenderer.Flush();
	}
}

int main()
{
	if (!DX8Wrapper::Init(nullptr))
	{
		fprintf(stderr, "RENDERW3DMESH_FAIL: DX8Wrapper::Init failed\n");
		return 1;
	}

	if (!DX8Wrapper::Set_Render_Device(0, W, H, 32, 0))
	{
		fprintf(stderr, "RENDERW3DMESH_FAIL: DX8Wrapper::Set_Render_Device failed\n");
		DX8Wrapper::Shutdown();
		return 1;
	}

	DX8Wrapper::Set_DX8_Render_State(D3DRS_ZENABLE, TRUE);

	// Real texture pipeline init sequence (Tests/RenderTexturePipeline's
	// established order, Do_Onetime_Device_Dependent_Inits' order): thumbnails
	// off (the game's real configuration once assets are actually managed).
	MissingTexture::_Init();
	TextureFilterClass::_Init_Filters(TextureFilterClass::TEXTURE_FILTER_POINT, TextureFilterClass::TEXTURE_FILTER_ANISOTROPIC_2X);
	TextureLoader::Init();
	DX8Wrapper::Set_Texture_Bitdepth(32);
	WW3D::Set_Thumbnail_Enabled(false);

	// Milestone 5's addition: the real mesh renderer + one live
	// WW3DAssetManager - the first ever constructed in this port, the
	// never-exercised Get_Texture cache path (check 3) and every
	// prototype loader's constructor now finally run for real.
	TheDX8MeshRenderer.Init();
	WW3DAssetManager asset_manager;

	// Real camera: CameraClass::Apply() pushes VIEW/PROJECTION to
	// DX8Wrapper (Set_Projection_Transform_With_Z_Bias/Set_Transform) and
	// updates the frustum RenderObjClass::Render's culling test reads -
	// matches Tests/RenderEngineDrawPath's Z0=5/fovY=90deg/aspect=1
	// projection (xScale=yScale=1, so NDC = local_xy/view_z), but through
	// the real engine object this time instead of hand-built D3DMATRIXes.
	CameraClass camera;
	camera.Set_Position(Vector3(0.0f, 0.0f, -Z0));
	Matrix3D identity_tm(1);
	camera.Set_Transform(identity_tm);
	camera.Set_View_Plane(1.57079632679489661923f, 1.57079632679489661923f); // 90deg h/v FOV
	camera.Set_Clip_Planes(1.0f, 100.0f);
	camera.Apply();

	RenderInfoClass rinfo(camera);

	DX8Wrapper::Begin_Scene();
	DX8Wrapper::Clear(true, true, Vector3(0.5f, 0.5f, 0.5f));

	// --- Check 1: load round-trip -------------------------------------
	QuadMeshDesc desc1{ "RW3DM", "Mesh1", -3.0f, 3.0f, 0.0f, false, false, 0, nullptr, false, 0 };
	std::string path1 = Author_Single_Mesh_W3D("mesh1.w3d", desc1);
	bool loaded1 = Load_W3D_File(asset_manager, path1);
	Check(loaded1, "1a. Load_3D_Assets succeeded");
	Check(asset_manager.Render_Obj_Exists("RW3DM.Mesh1"), "1b. Render_Obj_Exists");

	RenderObjClass* robj1 = asset_manager.Create_Render_Obj("RW3DM.Mesh1");
	Check(robj1 != nullptr, "1c. Create_Render_Obj returned non-null");
	if (robj1)
	{
		MeshClass* mesh1 = static_cast<MeshClass*>(robj1);
		Check(mesh1->Peek_Model()->Get_Vertex_Count() == 4, "1d. authored vertex count round-trips");
		Check(mesh1->Peek_Model()->Get_Polygon_Count() == 2, "1e. authored polygon count round-trips");
	}

	DX8Wrapper::End_Scene(false);

	unsigned char* pixels = static_cast<unsigned char*>(malloc(4 * W * H));
	glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	unsigned char* topdown = static_cast<unsigned char*>(malloc(4 * W * H));
	for (int y = 0; y < H; ++y)
	{
		memcpy(&topdown[y * W * 4], &pixels[(H - 1 - y) * W * 4], W * 4);
	}
	free(pixels);
	free(topdown);

	if (robj1) robj1->Release_Ref();

	asset_manager.Free_Assets();
	TheDX8MeshRenderer.Shutdown();
	TextureLoader::Deinit();
	MissingTexture::_Deinit();

	unlink(path1.c_str());

	DX8Wrapper::Shutdown();

	if (g_AnyFailure)
	{
		fprintf(stderr, "RENDERW3DMESH_FAIL: one or more checks failed (see above)\n");
		return 1;
	}

	printf("RENDERW3DMESH_OK: all checks passed (%dx%d)\n", W, H);
	return 0;
}
