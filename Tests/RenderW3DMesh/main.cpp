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
#include "hashtemplate.h"
#include "texture.h"
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
		if (d.vertex_color)
		{
			// Author an "emissive-only" vertex material (ambient=diffuse=0,
			// emissive=white) so the DCG per-vertex color renders unlit. This
			// is the engine's own supported mechanism for that, not a prelit
			// mesh (prelit rendering is not wired up this milestone): with no
			// scene lights and D3DRS_LIGHTING left TRUE, a DCG color routed to
			// DIFFUSEMATERIALSOURCE=COLOR1 is treated as a *lit* material
			// diffuse and resolves to ~black on real D3D8 (and to the GL
			// backend's uForceWhiteDiffuse placeholder, pure white). Post_Load_
			// Process (meshmatdesc.cpp) only clears D3DRS_LIGHTING when a
			// material is emissive-only across all passes - the sole branch
			// (its ~line 924-933) that calls Set_Lighting(false). Its
			// emissive-only pre-multiply is DCG *= emissive; emissive=white
			// (255) leaves the authored color exactly intact. W3d_Vertex_
			// Material_Reset defaults ambient AND diffuse to 255 (not 0), so
			// they must be explicitly zeroed here or the diffuse+ambient branch
			// wins and lighting stays on.
			vmat.Ambient.R = vmat.Ambient.G = vmat.Ambient.B = 0;
			vmat.Diffuse.R = vmat.Diffuse.G = vmat.Diffuse.B = 0;
			vmat.Emissive.R = vmat.Emissive.G = vmat.Emissive.B = 255;
		}
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
			// W3D_CHUNK_DCG is consumed by MeshModelClass::read_dcg, which reads
			// one W3dRGBAStruct (4 bytes: R,G,B,A) per vertex. It is a DIFFERENT
			// chunk/reader than the 3-byte W3dRGBStruct that read_vertex_colors
			// consumes for the pre-3.0 W3D_CHUNK_VERTEX_COLORS chunk (dispatched
			// separately at read_chunks' W3D_CHUNK_VERTEX_COLORS case) - the two
			// must not be conflated. Writing 3-byte structs into a DCG chunk
			// under-runs the per-vertex stride and misaligns vertices 1-3 on
			// load. read_dcg sets col.W = A/255; D3DCOLOR_XRGB packs A=0xFF.
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

	// Writes one complete W3D_CHUNK_HIERARCHY: header + a flat array of
	// pivots, all children of pivot 0 (the root). Matches htree.cpp's real
	// read_pivots exactly: pivot 0 must have ParentIdx=0xFFFFFFFF (root, no
	// parent - read_pivots asserts pidx==0 for that case) and an identity
	// rotation quaternion is (0,0,0,1) (W3dQuaternionStruct's Q[0..3] is
	// X,Y,Z,W, per iostruct.h). Version must be >= 3.0 (W3D_CURRENT_
	// HTREE_VERSION is) or Load_W3D takes the pre-3.0 "synthesize a root"
	// path, shifting every subsequent pivot index by one.
	void Write_Hierarchy_Chunk(ChunkSaveClass& csave, const char* tree_name, const float pivot_pos[][3], int num_pivots)
	{
		csave.Begin_Chunk(W3D_CHUNK_HIERARCHY);

		W3dHierarchyStruct header;
		memset(&header, 0, sizeof(header));
		header.Version = W3D_CURRENT_HTREE_VERSION;
		strncpy(header.Name, tree_name, W3D_NAME_LEN - 1);
		header.NumPivots = static_cast<uint32>(num_pivots);
		csave.Begin_Chunk(W3D_CHUNK_HIERARCHY_HEADER);
		csave.Write(&header, sizeof(header));
		csave.End_Chunk();

		csave.Begin_Chunk(W3D_CHUNK_PIVOTS);
		for (int i = 0; i < num_pivots; ++i)
		{
			W3dPivotStruct piv;
			memset(&piv, 0, sizeof(piv));
			snprintf(piv.Name, W3D_NAME_LEN, "Pivot%d", i);
			piv.ParentIdx = (i == 0) ? 0xFFFFFFFFu : 0u;
			piv.Translation.X = pivot_pos[i][0];
			piv.Translation.Y = pivot_pos[i][1];
			piv.Translation.Z = pivot_pos[i][2];
			piv.Rotation.Q[0] = 0.0f; piv.Rotation.Q[1] = 0.0f; piv.Rotation.Q[2] = 0.0f; piv.Rotation.Q[3] = 1.0f;
			csave.Write(&piv, sizeof(piv));
		}
		csave.End_Chunk(); // PIVOTS

		csave.End_Chunk(); // HIERARCHY
	}

	// One sub-object entry in an HLod's LOD array: which loaded render
	// object (by its full ContainerName.MeshName) attaches to which pivot.
	struct HlodSubObjectDesc
	{
		const char* full_name;
		int bone_index;
	};

	// Writes one complete W3D_CHUNK_HLOD with a single LOD level. Mirrors
	// hlod.cpp's real Load_W3D/read_header/SubObjectArrayClass::Load_W3D
	// exactly: HLOD_HEADER, then one HLOD_LOD_ARRAY wrapping a
	// HLOD_SUB_OBJECT_ARRAY_HEADER + one HLOD_SUB_OBJECT per entry. Each
	// sub-object's Name must match a mesh's full registered name exactly -
	// HLodClass's real constructor resolves it via WW3DAssetManager::
	// Get_Instance()->Create_Render_Obj(name) (hlod.cpp:1107), so the named
	// mesh(es) must already be registered by the time Create_Render_Obj is
	// called for THIS HLod (not necessarily before this HLOD chunk is
	// itself parsed - HLodDefClass::Load_W3D only stores name strings,
	// resolving them lazily at HLodClass construction time).
	void Write_Hlod_Chunk(ChunkSaveClass& csave, const char* hlod_name, const char* hierarchy_name, const HlodSubObjectDesc* subobjs, int num_subobjs)
	{
		csave.Begin_Chunk(W3D_CHUNK_HLOD);

		W3dHLodHeaderStruct header;
		memset(&header, 0, sizeof(header));
		header.Version = W3D_CURRENT_HLOD_VERSION;
		header.LodCount = 1;
		strncpy(header.Name, hlod_name, W3D_NAME_LEN - 1);
		strncpy(header.HierarchyName, hierarchy_name, W3D_NAME_LEN - 1);
		csave.Begin_Chunk(W3D_CHUNK_HLOD_HEADER);
		csave.Write(&header, sizeof(header));
		csave.End_Chunk();

		csave.Begin_Chunk(W3D_CHUNK_HLOD_LOD_ARRAY);

		W3dHLodArrayHeaderStruct arr_header;
		memset(&arr_header, 0, sizeof(arr_header));
		arr_header.ModelCount = static_cast<uint32>(num_subobjs);
		arr_header.MaxScreenSize = NO_MAX_SCREEN_SIZE;
		csave.Begin_Chunk(W3D_CHUNK_HLOD_SUB_OBJECT_ARRAY_HEADER);
		csave.Write(&arr_header, sizeof(arr_header));
		csave.End_Chunk();

		for (int i = 0; i < num_subobjs; ++i)
		{
			W3dHLodSubObjectStruct sub;
			memset(&sub, 0, sizeof(sub));
			sub.BoneIndex = static_cast<uint32>(subobjs[i].bone_index);
			strncpy(sub.Name, subobjs[i].full_name, W3D_NAME_LEN * 2 - 1);
			csave.Begin_Chunk(W3D_CHUNK_HLOD_SUB_OBJECT);
			csave.Write(&sub, sizeof(sub));
			csave.End_Chunk();
		}

		csave.End_Chunk(); // HLOD_LOD_ARRAY
		csave.End_Chunk(); // HLOD
	}

	// Authors one temp .w3d file containing a full HLod scene as sibling
	// top-level chunks: one HIERARCHY, then one MESH per entry in
	// `meshes`, then one HLOD referencing them by name. WW3DAssetManager::
	// Load_3D_Assets's real dispatch loop (assetmgr.cpp:669-689) just
	// registers each top-level chunk by name in turn - it doesn't resolve
	// any cross-references between them - so the only real ordering
	// requirement is that this whole file finishes loading before
	// Create_Render_Obj is ever called for the HLod's own name, which the
	// call site already guarantees (author+load, then separately resolve).
	std::string Author_Hlod_Scene_W3D(const char* leaf,
		const char* tree_name, const float pivot_pos[][3], int num_pivots,
		const QuadMeshDesc* meshes, int num_meshes,
		const char* hlod_name, const HlodSubObjectDesc* subobjs, int num_subobjs)
	{
		std::string path = Temp_Path(leaf);
		RawFileClass file(path.c_str());
		if (!file.Open(FileClass::WRITE)) { Fail("could not create temp HLod W3D file"); return path; }
		ChunkSaveClass csave(&file);
		Write_Hierarchy_Chunk(csave, tree_name, pivot_pos, num_pivots);
		for (int i = 0; i < num_meshes; ++i) Write_Mesh_Chunk(csave, meshes[i]);
		Write_Hlod_Chunk(csave, hlod_name, tree_name, subobjs, num_subobjs);
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

	// Milestone 6 (native port plan Phase 5(a), Draft 26 Steps 2+4
	// combined): Trap 1 ends here - Set_Render_Device's Create_Device now
	// runs the real Do_Onetime_Device_Dependent_Inits chain itself
	// (MissingTexture::_Init/TextureFilterClass::_Init_Filters/
	// TextureLoader::Init/TheDX8MeshRenderer.Init among others), so this
	// harness no longer hand-initializes them; MissingTexture::_Init()'s
	// WWASSERT(!_MissingTexture) would abort on the double-init otherwise.
	// Thumbnails off (the game's real configuration once assets are
	// actually managed) and the texture bit depth are harness-local
	// settings, not part of that chain, so they stay explicit here.
	DX8Wrapper::Set_Texture_Bitdepth(32);
	WW3D::Set_Thumbnail_Enabled(false);

	// Milestone 5's addition: one live WW3DAssetManager - the first ever
	// constructed in this port, the never-exercised Get_Texture cache path
	// (check 3) and every prototype loader's constructor now finally run
	// for real.
	WW3DAssetManager asset_manager;

	// Real camera: CameraClass::Apply() pushes VIEW/PROJECTION to
	// DX8Wrapper (Set_Projection_Transform_With_Z_Bias/Set_Transform) and
	// updates the frustum RenderObjClass::Render's culling test reads -
	// matches Tests/RenderEngineDrawPath's Z0=5/fovY=90deg/aspect=1
	// projection (xScale=yScale=1, so NDC = local_xy/view_z), but through
	// the real engine object this time instead of hand-built D3DMATRIXes.
	CameraClass camera;
	// Matrix3D(const Vector3&) is identity-rotation + the given
	// translation - a single real Set_Transform call, not Set_Position
	// followed by a SEPARATE Set_Transform(identity) that would silently
	// overwrite the position back to the origin (a real bug this
	// harness's first attempt at checks 2-4 hit).
	//
	// Camera sits at +Z0, not -Z0: camera.cpp:625's Update_Frustum
	// documents "Forward is negative Z in our viewspace coordinate
	// system" - with identity rotation, this camera looks down -Z, so
	// it must be placed in FRONT of the z=0 quads (at +Z0) to look back
	// at them across a view distance of Z0. Placing it at -Z0 (this
	// harness's second real bug: confirmed via CollisionMath::Overlap_Test
	// logging overlap==OUTSIDE for every quad, meaning they were never
	// even entering DX8MeshRendererClass's registered-mesh lists, not a
	// draw-call or pixel-format problem) puts the quads behind the
	// camera, permanently frustum-culled regardless of position/pixel math.
	Matrix3D camera_tm(Vector3(0.0f, 0.0f, Z0));
	camera.Set_Transform(camera_tm);
	camera.Set_View_Plane(1.57079632679489661923f, 1.57079632679489661923f); // 90deg h/v FOV
	camera.Set_Clip_Planes(1.0f, 100.0f);
	camera.Apply();

	// TheDX8MeshRenderer.Flush() early-outs with nothing drawn if no
	// camera has been set (dx8renderer.cpp:2176, `if (!camera) return;`) -
	// this harness's first attempt at checks 2-4 hit exactly this: every
	// Render_Obj() call succeeded but Flush() silently did nothing, so
	// the readback saw only the clear color at every predicted position.
	TheDX8MeshRenderer.Set_Camera(&camera);

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

	// --- Check 2: textured lit rigid mesh -----------------------------
	// XYZNUV1 FVF via Define_FVF (no diffuse component - finding 6(i)'s
	// white-diffuse rule), real Load_Texture -> Get_Texture -> foreground
	// TGA load.
	std::string tex_path = Temp_Path("check2.tga");
	Write_Solid_TGA(tex_path, 4, 4, 30, 200, 100);
	QuadMeshDesc desc2{ "RW3DM", "Mesh2", 3.0f, 3.0f, 0.0f, true, false, 0, tex_path.c_str(), false, 0 };
	std::string path2 = Author_Single_Mesh_W3D("mesh2.w3d", desc2);
	bool loaded2 = Load_W3D_File(asset_manager, path2);
	Check(loaded2, "2a. textured mesh Load_3D_Assets succeeded");
	RenderObjClass* robj2 = asset_manager.Create_Render_Obj("RW3DM.Mesh2");
	Check(robj2 != nullptr, "2b. Create_Render_Obj returned non-null");
	if (robj2) Render_Obj(robj2, rinfo);

	// --- Check 3: Get_Texture cache path -------------------------------
	// A second mesh referencing the SAME texture name - the never-
	// exercised WW3DAssetManager::Get_Texture cache path, now load-
	// bearing: Texture_Hash() must hold exactly one entry for it.
	QuadMeshDesc desc3{ "RW3DM", "Mesh3", -3.0f, 0.0f, 0.0f, true, false, 0, tex_path.c_str(), false, 0 };
	std::string path3 = Author_Single_Mesh_W3D("mesh3.w3d", desc3);
	bool loaded3 = Load_W3D_File(asset_manager, path3);
	Check(loaded3, "3a. second textured mesh Load_3D_Assets succeeded");
	RenderObjClass* robj3 = asset_manager.Create_Render_Obj("RW3DM.Mesh3");
	Check(robj3 != nullptr, "3b. Create_Render_Obj returned non-null");
	if (robj3) Render_Obj(robj3, rinfo);
	{
		int tex_hash_count = 0;
		HashTemplateIterator<StringClass, TextureClass*> ite(asset_manager.Texture_Hash());
		for (ite.First(); !ite.Is_Done(); ite.Next()) ++tex_hash_count;
		Check(tex_hash_count == 1, "3c. Texture_Hash holds exactly one entry (cache path)");
	}

	// --- Check 4: vertex-color mesh -------------------------------------
	// Per-vertex DCG, lighting off (the harness default - no
	// VertexMaterialClass::UseLighting call anywhere): rendered colors
	// must equal the authored vertex colors exactly, pinning the diffuse
	// path against Milestone 5 Step 4's white-under-lighting rule (the
	// discriminator check 2 alone can't provide, since diffuse-less FVFs
	// go through glVertexAttrib4f's white default either way).
	const DWORD CHECK4_DIFFUSE = D3DCOLOR_XRGB(80, 160, 220);
	QuadMeshDesc desc4{ "RW3DM", "Mesh4", 3.0f, 0.0f, 0.0f, false, true, CHECK4_DIFFUSE, nullptr, false, 0 };
	std::string path4 = Author_Single_Mesh_W3D("mesh4.w3d", desc4);
	bool loaded4 = Load_W3D_File(asset_manager, path4);
	Check(loaded4, "4a. vertex-color mesh Load_3D_Assets succeeded");
	RenderObjClass* robj4 = asset_manager.Create_Render_Obj("RW3DM.Mesh4");
	Check(robj4 != nullptr, "4b. Create_Render_Obj returned non-null");
	if (robj4) Render_Obj(robj4, rinfo);

	// --- Check 5/6: HLod hierarchy + skin -------------------------------
	// A real pivot hierarchy (root + 3 translated children) drives a real
	// HLodClass instantiation with two rigid sub-meshes (check 5) and one
	// skin mesh (check 6) - each attaches to its own pivot, so each
	// renders at a pivot-offset-predicted, non-overlapping screen
	// position. Mesh geometry here is authored LOCAL (centered at its own
	// origin, unlike checks 1-4's world-baked coords): HLodClass::
	// Add_Lod_Model/Update_Sub_Object_Transforms sets each rigid
	// sub-object's own Transform to its bone's world transform
	// (hlod.cpp:3349/3515); a skin's vertices are independently
	// transformed per-vertex by their own BoneIdx's pivot transform
	// (meshgeometry.cpp's get_deformed_vertices) - the skin mesh's own
	// base Transform plays no part in its actual rendered position.
	const char* HTREE_NAME = "RW3DM_HTree";
	const float PIVOT_POS[4][3] = {
		{ 0.0f, 0.0f, 0.0f },   // 0: root
		{ -3.0f, -3.0f, 0.0f }, // 1: check 5's first rigid sub-mesh
		{ 0.0f, -3.0f, 0.0f },  // 2: check 5's second rigid sub-mesh
		{ 3.0f, -3.0f, 0.0f },  // 3: check 6's skin mesh
	};

	std::string tex_path56 = Temp_Path("check56.tga");
	Write_Solid_TGA(tex_path56, 4, 4, 210, 60, 170);

	const DWORD CHECK5B_DIFFUSE = D3DCOLOR_XRGB(90, 220, 60);
	QuadMeshDesc mesh5A{ "RW3DM", "Mesh5A", 0.0f, 0.0f, 0.0f, true, false, 0, tex_path56.c_str(), false, 0 };
	QuadMeshDesc mesh5B{ "RW3DM", "Mesh5B", 0.0f, 0.0f, 0.0f, false, true, CHECK5B_DIFFUSE, nullptr, false, 0 };
	QuadMeshDesc mesh6Skin{ "RW3DM", "Mesh6Skin", 0.0f, 0.0f, 0.0f, true, false, 0, tex_path56.c_str(), true, 3 };
	QuadMeshDesc hlod_meshes[3] = { mesh5A, mesh5B, mesh6Skin };

	HlodSubObjectDesc subobjs[3] = {
		{ "RW3DM.Mesh5A", 1 },
		{ "RW3DM.Mesh5B", 2 },
		{ "RW3DM.Mesh6Skin", 3 },
	};

	std::string path56 = Author_Hlod_Scene_W3D("hlod56.w3d", HTREE_NAME, PIVOT_POS, 4, hlod_meshes, 3, "RW3DM_HLod", subobjs, 3);
	bool loaded56 = Load_W3D_File(asset_manager, path56);
	Check(loaded56, "5a. HLod scene Load_3D_Assets succeeded");

	RenderObjClass* robjHlod = asset_manager.Create_Render_Obj("RW3DM_HLod");
	Check(robjHlod != nullptr, "5b. Create_Render_Obj (HLod) returned non-null");
	if (robjHlod)
	{
		Check(robjHlod->Class_ID() == RenderObjClass::CLASSID_HLOD, "5c. Create_Render_Obj yields a real HLodClass");
		Check(robjHlod->Get_Num_Sub_Objects() == 3, "5d. HLod has all 3 authored sub-objects");
		Render_Obj(robjHlod, rinfo);
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

	int px, py;
	float ndc_x, ndc_y;

	Predict_Ndc(3.0f, 3.0f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 30, 200, 100, "2c. textured mesh: authored texel color");

	Predict_Ndc(-3.0f, 0.0f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 30, 200, 100, "3d. second textured mesh: same texel color");

	Predict_Ndc(3.0f, 0.0f, 0.0f, &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 80, 160, 220, "4c. vertex-color mesh: authored vertex color exactly");

	// Predicted positions come from PIVOT_POS[1..3] directly (each mesh's
	// own local vertex coords are centered at 0,0,0 - only the bone
	// transform offsets it), not from any mesh-authored local_x/y.
	Predict_Ndc(PIVOT_POS[1][0], PIVOT_POS[1][1], PIVOT_POS[1][2], &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 210, 60, 170, "5e. HLod rigid sub-mesh A: textured, at pivot 1's position");

	Predict_Ndc(PIVOT_POS[2][0], PIVOT_POS[2][1], PIVOT_POS[2][2], &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 90, 220, 60, "5f. HLod rigid sub-mesh B: vertex-colored, at pivot 2's position");

	Predict_Ndc(PIVOT_POS[3][0], PIVOT_POS[3][1], PIVOT_POS[3][2], &ndc_x, &ndc_y);
	Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
	Check_Pixel(topdown, px, py, 210, 60, 170, "6a. skin mesh: bone-transformed to pivot 3's position");

	free(topdown);

	if (robj1) robj1->Release_Ref();
	if (robj2) robj2->Release_Ref();
	if (robj3) robj3->Release_Ref();
	if (robj4) robj4->Release_Ref();
	if (robjHlod) robjHlod->Release_Ref();

	// Milestone 6: matching manual subsystem teardown removed -
	// DX8Wrapper::Shutdown() below now runs Release_Device()'s Do_Onetime_
	// Device_Dependent_Shutdowns() chain (TheDX8MeshRenderer.Shutdown()/
	// TextureLoader::Deinit()/MissingTexture::_Deinit() among others), so
	// calling them here too would double-deinit. asset_manager.Free_Assets()
	// stays explicit (not part of that chain) and must run before
	// DX8Wrapper::Shutdown() releases the mesh renderer/texture subsystems
	// its assets reference - asset_manager's own destructor (WW3DAssetManager,
	// assetmgr.cpp) calls Free_Assets() too, but only at end of main(),
	// after DX8Wrapper::Shutdown() would already have torn those down.
	asset_manager.Free_Assets();

	unlink(path1.c_str());
	unlink(path2.c_str());
	unlink(path3.c_str());
	unlink(path4.c_str());
	unlink(tex_path.c_str());
	unlink(path56.c_str());
	unlink(tex_path56.c_str());

	DX8Wrapper::Shutdown();

	if (g_AnyFailure)
	{
		fprintf(stderr, "RENDERW3DMESH_FAIL: one or more checks failed (see above)\n");
		return 1;
	}

	printf("RENDERW3DMESH_OK: all checks passed (%dx%d)\n", W, H);
	return 0;
}
