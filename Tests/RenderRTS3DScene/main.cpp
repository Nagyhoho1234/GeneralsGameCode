// Phase 5(a) Milestone 8 Task 4 (native port plan, Draft 30, "Implementation
// ordering" step 4) - the milestone's payoff task and exit harness: a pixel
// rendered through the GAME'S OWN real scene manager (RTS3DScene, unified
// into Core/GameEngineDevice/.../W3DScene.cpp by Task 2) with a real,
// defaults-constructed GlobalData object underneath it - the first
// GameClient scene object and the first GlobalData ever constructed and
// executed on POSIX. This combines Milestone 7's file-system/archive-
// loading stack (Tests/RenderGameAssets/main.cpp, whose ENTIRE prologue/
// link infrastructure is reused wholesale here, per the task brief) with
// Milestone 8's newly-unified scene layer. See docs/native-port-plan.md's
// Draft 30 section (findings 3, 5, 6, 7, 8, the design decisions, and the
// full Step 4 implementation-ordering text) for the full design rationale.
//
// Prologue (Tests/RenderGameAssets's, unchanged): five real CriticalSection
// objects wired to the five global pointers (mirrors WinMain.cpp:875-882),
// THEN initMemoryManager().
//
// Structure, in the engine's own order, extending Tests/RenderGameAssets's:
//   1. Author the quad's .w3d and .tga bytes (Tests/RenderWW3DFrame/Tests/
//      RenderGameAssets's authoring code, reused verbatim per the task
//      brief), embed them into a test-authored .big archive, delete every
//      loose authoring temp file before any file-system object is
//      constructed.
//   2. Construct TheFileSystem/TheLocalFileSystem/TheArchiveFileSystem,
//      TheFileSystem->init(), TheW3DFileSystem (the real
//      "_TheFileFactory = this" swap), WW3D::Init -> Set_Render_Device ->
//      Set_Thumbnail_Enabled(false) + Set_Texture_Bitdepth(32) - identical
//      to Milestone 6/7's own precedent for avoiding the async
//      texture-load race.
//   3. NEW for this milestone: TheWritableGlobalData = NEW GlobalData (the
//      GeneralsMD tree's real GlobalData.cpp, compiled directly into this
//      harness - Draft 30 finding 5: its constructor is set-defaults-only,
//      no INI closure needed). A few known-sane defaults are asserted
//      immediately after construction so a silently-wrong GlobalData fails
//      loudly here, not as "wrong pixels" in a later check.
//   4. NEW: a real RTS3DScene, constructed exactly the way the game itself
//      does (W3DDisplay.cpp:771's NEW_REF(RTS3DScene, ()) / :459's
//      REF_PTR_RELEASE(m_3DScene) teardown idiom) - the first GameClient
//      scene object ever built and run on POSIX. One directional
//      LightClass via setGlobalLight() (deliberately zeroed to contribute
//      nothing - see its own comment below for why) plus
//      Set_Ambient_Light(1,1,1) - chosen per the plan's open question 4 so
//      the expected pixel color is hand-derivable (ambient(1,1,1) is the
//      multiplicative identity against the authored texture color,
//      matching every prior harness's exact expected values without new
//      lighting math).
//   5. Milestone 7's quad, loaded from the SAME kind of test-authored .big
//      archive via WW3DAssetManager, Add_Render_Object'd into the real
//      scene.
//   6. Frames driven via RTS3DScene::doRender(camera) - the game's real
//      entry point (SubsystemInterface::DRAW() -> draw() ->
//      WW3D::Render(this, m_camera), W3DScene.cpp:2136-2156) - NOT
//      WW3D::Render(scene, camera) directly, per the plan's recommendation
//      (open question 3). Verified harmless: DUMP_PERF_STATS (the only
//      thing that would make DRAW() do anything beyond a plain
//      "{draw();}") is only defined when RTS_DEBUG is set
//      (Common/GameCommon.h:58-59), and this repo's default build preset
//      is Release with RTS_DEBUG off - confirmed against this harness's
//      own build config before committing to this call, exactly as the
//      plan asked.
//
// Five checks (the fallback was NOT needed - see this task's close-out
// report for the actual link closure encountered, close to the plan's own
// ~10-15-entry prediction once every small harness-local stub is counted,
// PLUS two genuine, previously-undiscovered source-level gaps this task
// found and fixed properly - see link_stubs.cpp and CMakeLists.txt's own
// comments, and PortableD3D8/d3d8.h's and WW3D2/CMakeLists.txt's):
//   1. GlobalData defaults sane (m_maxVisible* buffer sizes the RTS3DScene
//      ctor itself depends on, m_shroudOn, m_timeOfDay).
//   2. Pixel-verified frame: the quad renders at the CPU-predicted position
//      through the FULL real chain (RTS3DScene::Render ->
//      updateFixedLightEnvironments -> Customized_Render ->
//      Visibility_Check -> renderOneObject -> Flush) - same rigor as
//      Milestone 6/7's pixel checks, same expected colors (ambient(1,1,1)
//      keeps them numerically identical to every prior harness's raw
//      texel).
//   3. Culling check: the quad's render object is transformed outside the
//      camera frustum; re-rendering shows background color at the OLD
//      position - proof that Visibility_Check's real Cull_Sphere-based
//      culling logic ran on this exact frame, not merely that Render() was
//      called.
//   4. drawTerrainOnly(true) check: with the flag set, the quad (moved back
//      to its original, in-frustum position) does NOT render - proof of
//      the game's own real control-flow branch
//      (RTS3DScene::Customized_Render's "if (m_drawTerrainOnly) return;").
//   5. Full teardown: the scene released via the SAME REF_PTR_RELEASE idiom
//      W3DDisplay.cpp itself uses, GlobalData destroyed, then Milestone 7's
//      file-system/WW3D teardown pattern, clean ctest exit, stable across
//      5+ runs.
#include "PreRTS.h"

#include "Common/AsciiString.h"
#include "Common/CriticalSection.h"
#include "Common/file.h"
#include "Common/FileSystem.h"
#include "Common/LocalFileSystem.h"
#include "Common/ArchiveFileSystem.h"
#include "Common/RAMFile.h"
#include "Common/GameMemory.h"
#include "Common/GlobalData.h"
#include "StdDevice/Common/StdLocalFileSystem.h"
#include "StdDevice/Common/StdBIGFileSystem.h"
#include "W3DDevice/GameClient/W3DFileSystem.h"
#include "W3DDevice/GameClient/W3DScene.h"

#include "WWLib/ffactory.h" // _TheFileFactory - the final teardown's verification.

#include "dx8wrapper.h"
#include "ww3d.h"
#include "camera.h"
#include "scene.h"
#include "rendobj.h"
#include "assetmgr.h"
#include "light.h"
#include "w3d_file.h"
#include "chunkio.h"
#include "RAWFILE.h"
#include "PortableD3D8/gl_core33.h"

#include <GLFW/glfw3.h>

#include <Utility/endian_compat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <filesystem>
#include <unistd.h>

namespace fs = std::filesystem;

namespace
{
	bool g_AnyFailure = false;

	void Fail(const char* what)
	{
		fprintf(stderr, "RENDERRTS3DSCENE_FAIL: %s\n", what);
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

	// World-space depth/half-extent convention, matching every prior
	// rendering harness (Tests/RenderEngineDrawPath, Tests/RenderW3DMesh,
	// Tests/RenderWW3DFrame, Tests/RenderGameAssets).
	const float Z0 = 5.0f;
	const float HALF = 1.0f;

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

	// Reads the currently-bound GL_READ_FRAMEBUFFER and returns it top-down
	// (row 0 = top of screen) as a freshly malloc'd W*H*4 RGBA buffer.
	// Caller frees. Identical to every prior harness's helper.
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

	// --- Asset authoring: real TGA/.w3d chunk bytes, built at runtime -----
	// Byte-for-byte the same mechanics as Tests/RenderGameAssets/main.cpp,
	// reused per the task brief ("reuse Milestone 7's quad"). The bytes
	// never stay on disk as loose files - each is authored to a throwaway
	// temp path, slurped into memory, and the temp file is deleted
	// immediately, all BEFORE any file-system object is constructed or any
	// archive is loaded.

	std::string Temp_Path(const char* leaf)
	{
		const char* tmp = getenv("TMPDIR");
		if (!tmp) tmp = "/tmp";
		char buf[512];
		snprintf(buf, sizeof(buf), "%s/r3ds_%d_%s", tmp, static_cast<int>(getpid()), leaf);
		return std::string(buf);
	}

	std::string SlurpAndDelete(const std::string& path)
	{
		FILE* f = fopen(path.c_str(), "rb");
		if (!f) { Fail("could not reopen authored temp file for slurp"); return std::string(); }
		fseek(f, 0, SEEK_END);
		long sz = ftell(f);
		fseek(f, 0, SEEK_SET);
		std::string data;
		if (sz > 0)
		{
			data.resize(static_cast<size_t>(sz));
			size_t n = fread(&data[0], 1, static_cast<size_t>(sz), f);
			data.resize(n);
		}
		fclose(f);
		remove(path.c_str());
		return data;
	}

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

	// Byte-for-byte the same W3D_CHUNK_MESH layout as Tests/RenderGameAssets's
	// Write_Mesh_Chunk: a single textured two-triangle quad, bare
	// archive-relative texture name.
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
		// references index 0 crashes the loader).
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

	std::string Author_Quad_W3D_Bytes(const char* container_name, const char* mesh_name, const char* texture_name)
	{
		std::string path = Temp_Path("quad.w3d");
		{
			RawFileClass file(path.c_str());
			if (!file.Open(FileClass::WRITE)) { Fail("could not create temp W3D authoring file"); return std::string(); }
			ChunkSaveClass csave(&file);
			Write_Mesh_Chunk(csave, container_name, mesh_name, texture_name);
			file.Close();
		}
		return SlurpAndDelete(path);
	}

	std::string Author_Quad_TGA_Bytes(int width, int height, unsigned char r, unsigned char g, unsigned char b)
	{
		std::string path = Temp_Path("quad.tga");
		Write_Solid_TGA(path, width, height, r, g, b);
		return SlurpAndDelete(path);
	}

	// --- .big archive authoring: byte-for-byte the same format as Tests/
	// GameFileSystem/Tests/RenderGameAssets's AuthorBigArchive.
	struct ArchiveEntry
	{
		std::string path;
		std::string data;
	};

	void AuthorBigArchive(const fs::path& outPath, const std::vector<ArchiveEntry>& entries)
	{
		uint32_t dirSize = 0;
		for (const auto& e : entries) dirSize += 4 + 4 + static_cast<uint32_t>(e.path.size() + 1);

		const uint32_t headerSize = 0x10;
		const uint32_t dataStart = headerSize + dirSize;

		std::vector<uint32_t> offsets(entries.size());
		uint32_t cursor = dataStart;
		for (size_t i = 0; i < entries.size(); ++i)
		{
			offsets[i] = cursor;
			cursor += static_cast<uint32_t>(entries[i].data.size());
		}
		const uint32_t archiveSize = cursor;

		FILE* f = fopen(outPath.string().c_str(), "wb");
		if (!f) { Fail("could not create .big archive file"); return; }

		fwrite("BIGF", 1, 4, f);
		fwrite(&archiveSize, 4, 1, f); // read raw and unused by the reader.
		uint32_t countBE = htobe(static_cast<uint32_t>(entries.size()));
		fwrite(&countBE, 4, 1, f);
		uint32_t reserved = 0;
		fwrite(&reserved, 4, 1, f); // pads the fixed header out to seek 0x10.

		for (size_t i = 0; i < entries.size(); ++i)
		{
			uint32_t offBE = htobe(offsets[i]);
			uint32_t sizeBE = htobe(static_cast<uint32_t>(entries[i].data.size()));
			fwrite(&offBE, 4, 1, f);
			fwrite(&sizeBE, 4, 1, f);
			fwrite(entries[i].path.c_str(), 1, entries[i].path.size() + 1, f); // incl. '\0'.
		}
		for (const auto& e : entries)
		{
			if (!e.data.empty()) fwrite(e.data.data(), 1, e.data.size(), f);
		}

		fclose(f);
	}

	void RemoveIfExists(const fs::path& p)
	{
		std::error_code ec;
		fs::remove_all(p, ec);
	}

	const char* const kArchiveName = "RenderRTS3DScene.big";

	const int BG_R = 40, BG_G = 70, BG_B = 110;
	const int TEX_R = 180, TEX_G = 200, TEX_B = 30;

	const float QUAD_X = 0.9f, QUAD_Y = -0.4f;
	// Far outside the 90deg-FOV frustum at Z0=5 (local_x/Z0 = 50/5 = 10, way
	// beyond the [-1,1] NDC range) - used by Check 3 to move the quad
	// somewhere Visibility_Check's Cull_Sphere test must reject it.
	const float OFFSCREEN_X = 50.0f, OFFSCREEN_Y = 0.0f;

} // end anonymous namespace - main()'s own comment below explains why its
  // body is a literal nested block, not split into helper functions.

int main()
{
	// Line-buffer both streams so a crash mid-run still leaves every check
	// result up to that point on disk (Tests/GameFileSystem's precedent).
	setvbuf(stdout, nullptr, _IOLBF, 0);
	setvbuf(stderr, nullptr, _IOLBF, 0);

	// ---- Prologue: mirrors WinMain.cpp:875-882 exactly (unchanged from
	// every prior harness). ----
	static CriticalSection critSec1, critSec2, critSec3, critSec4, critSec5;
	TheAsciiStringCriticalSection = &critSec1;
	TheUnicodeStringCriticalSection = &critSec2;
	TheDmaCriticalSection = &critSec3;
	TheMemoryPoolCriticalSection = &critSec4;
	TheDebugLogCriticalSection = &critSec5;

	initMemoryManager();

	// All checks and every local this harness needs, in one BLOCK BEFORE
	// shutdownMemoryManager() - deliberately, not incidentally (Tests/
	// GameFileSystem's own found-by-gdb SIGSEGV lesson, carried over
	// verbatim by every rendering harness since). This MUST be a literal
	// nested block lexically inside main() itself, NOT a separate function
	// or lambda: dx8wrapper.h:711 declares "friend int main();", which
	// grants access to DX8Wrapper::Set_Texture_Bitdepth only to the free
	// function literally named main() in the global namespace (Tests/
	// RenderGameAssets's own hard-won lesson, carried over verbatim).
	{
		RemoveIfExists(kArchiveName);

		printf("=== Setup 0: authoring quad.w3d + quad.tga into a test-authored .big archive ===\n");
		std::string tgaBytes = Author_Quad_TGA_Bytes(4, 4, TEX_R, TEX_G, TEX_B);
		Check(!tgaBytes.empty(), "0a. authored TGA bytes are non-empty");
		std::string w3dBytes = Author_Quad_W3D_Bytes("R3DS", "Quad", "quad.tga");
		Check(!w3dBytes.empty(), "0b. authored W3D bytes are non-empty");

		std::vector<ArchiveEntry> entries = {
			{ "Art\\W3D\\quad.w3d",     w3dBytes },
			{ "Art\\Textures\\quad.tga", tgaBytes },
		};
		AuthorBigArchive(kArchiveName, entries);
		// From this point on, no LOOSE file containing these bytes exists
		// anywhere on disk - only the just-authored .big archive.

		// ---- Construct the file-system stack, in GameEngine::init()'s own
		// spine order - identical to Tests/RenderGameAssets. ----
		TheFileSystem = new FileSystem;
		TheLocalFileSystem = new StdLocalFileSystem;
		TheArchiveFileSystem = new StdBIGFileSystem;
		TheFileSystem->init();

		TheW3DFileSystem = new W3DFileSystem;
		Check(_TheFileFactory == static_cast<FileFactoryClass*>(TheW3DFileSystem),
			"0c. W3DFileSystem constructor installed itself as _TheFileFactory");

		printf("=== Setup 1: WW3D init round trip ===\n");
		WW3DErrorType init_result = WW3D::Init(nullptr);
		Check(init_result == WW3D_ERROR_OK, "1a. WW3D::Init returned WW3D_ERROR_OK");

		WW3DErrorType device_result = WW3D::Set_Render_Device(0, g_W, g_H, 32, /*windowed=*/1, /*resize_window=*/true);
		Check(device_result == WW3D_ERROR_OK, "1b. WW3D::Set_Render_Device returned WW3D_ERROR_OK");

		if (init_result != WW3D_ERROR_OK || device_result != WW3D_ERROR_OK)
		{
			fprintf(stderr, "RENDERRTS3DSCENE_FAIL: init round-trip failed, aborting\n");
			g_AnyFailure = true;
			return 1; // early-abort, same rationale as every prior harness's.
		}

		// Milestone 6 Task 5's own precedent, verbatim rationale: must be set
		// BEFORE the first render to avoid racing the background texture
		// loader thread.
		WW3D::Set_Thumbnail_Enabled(false);
		DX8Wrapper::Set_Texture_Bitdepth(32);

		// ---- Check 1: GlobalData defaults sane -----------------------------
		// Draft 30 finding 5: the GeneralsMD tree's real GlobalData.cpp,
		// compiled directly into this harness. Its constructor is
		// set-defaults-only - no INI closure needed. Asserting these
		// specific defaults matters beyond a generic non-null check: they
		// are the EXACT values RTS3DScene's own constructor depends on to
		// size its m_translucentObjectsBuffer/m_potentialOccluders/
		// m_potentialOccludees/m_nonOccludersOrOccludees buffers (W3DScene.cpp
		// :232-258) - a silently-wrong GlobalData would otherwise surface
		// only as a subtle "wrong pixels" or buffer-sizing bug several
		// checks later.
		printf("=== Check 1: GlobalData defaults sane ===\n");
		TheWritableGlobalData = NEW GlobalData;
		Check(TheWritableGlobalData != nullptr, "1c. TheWritableGlobalData constructed (non-null)");
		Check(TheWritableGlobalData->m_maxVisibleTranslucentObjects == 512, "1d. m_maxVisibleTranslucentObjects == 512 (real default)");
		Check(TheWritableGlobalData->m_maxVisibleOccluderObjects == 512, "1e. m_maxVisibleOccluderObjects == 512 (real default)");
		Check(TheWritableGlobalData->m_maxVisibleOccludeeObjects == 512, "1f. m_maxVisibleOccludeeObjects == 512 (real default)");
		Check(TheWritableGlobalData->m_maxVisibleNonOccluderOrOccludeeObjects == 512, "1g. m_maxVisibleNonOccluderOrOccludeeObjects == 512 (real default)");
		Check(TheWritableGlobalData->m_shroudOn == true, "1h. m_shroudOn == true (ENABLE_CONFIGURABLE_SHROUD real default)");
		Check(TheWritableGlobalData->m_timeOfDay == TIME_OF_DAY_AFTERNOON, "1i. m_timeOfDay == TIME_OF_DAY_AFTERNOON (real default)");

		// ---- Construct the real RTS3DScene - the first GameClient scene
		// object ever built and run on POSIX (Draft 30 finding 3/6). Built
		// exactly the way the game itself does it
		// (W3DDisplay.cpp:771's NEW_REF(RTS3DScene, ()) idiom). Its ctor
		// dereferences TheGlobalData unguarded (W3DScene.cpp:107-113,
		// :153-176) - safe now that a real GlobalData exists above. ----
		printf("=== Constructing the real RTS3DScene ===\n");
		RTS3DScene* scene = NEW_REF(RTS3DScene, ());
		Check(scene != nullptr, "1j. RTS3DScene constructed (non-null)");

		// Set_Ambient_Light(1,1,1) (Draft 30 open question 4's recommendation):
		// the multiplicative identity against the authored texture color, so
		// Check 2's expected pixel value stays numerically identical to
		// every prior harness's raw texel - hand-derivable, not discovered
		// by trial and error.
		scene->Set_Ambient_Light(Vector3(1.0f, 1.0f, 1.0f));

		// One directional light via setGlobalLight (task brief item 3) -
		// deliberately zeroed (Ambient/Diffuse/Specular all (0,0,0)) so it
		// contributes NOTHING to renderOneObject's lightEnv regardless of
		// its direction relative to the quad's normal: this exercises the
		// real setGlobalLight()/updateFixedLightEnvironments() API and data
		// path for real (m_numGlobalLights becomes 1, m_globalLight[0] is
		// non-null, updateFixedLightEnvironments's m_defaultLightEnv.Add_Light
		// loop really runs) while keeping Check 2's expected color
		// hand-derivable without new lighting math, per open question 4.
		LightClass* zeroLight = NEW_REF(LightClass, (LightClass::DIRECTIONAL));
		zeroLight->Set_Ambient(Vector3(0.0f, 0.0f, 0.0f));
		zeroLight->Set_Diffuse(Vector3(0.0f, 0.0f, 0.0f));
		zeroLight->Set_Specular(Vector3(0.0f, 0.0f, 0.0f));
		scene->setGlobalLight(zeroLight, 0);
		zeroLight->Release_Ref(); // setGlobalLight's REF_PTR_SET took its own ref.

		// ---- Load Milestone 7's quad from the test-authored .big archive,
		// Add_Render_Object into the real scene. ----
		WW3DAssetManager asset_manager;
		bool loaded = asset_manager.Load_3D_Assets("quad.w3d");
		Check(loaded, "1k. Load_3D_Assets(\"quad.w3d\") succeeded via factory->TheFileSystem->archive resolution");

		RenderObjClass* robj = loaded ? asset_manager.Create_Render_Obj("R3DS.Quad") : nullptr;
		Check(robj != nullptr, "1l. Create_Render_Obj(\"R3DS.Quad\") returned non-null");

		if (!robj)
		{
			fprintf(stderr, "RENDERRTS3DSCENE_FAIL: could not create render object, aborting remaining checks\n");
			REF_PTR_RELEASE(scene);
			WW3D::Shutdown();
			g_AnyFailure = true;
			return 1;
		}

		scene->Add_Render_Object(robj);

		// Camera at +Z0 with identity rotation, looking back at the quad -
		// same convention as every prior rendering harness.
		CameraClass camera;
		Matrix3D camera_tm(Vector3(0.0f, 0.0f, Z0));
		camera.Set_Transform(camera_tm);
		camera.Set_View_Plane(1.57079632679489661923f, 1.57079632679489661923f); // 90deg h/v FOV
		camera.Set_Clip_Planes(1.0f, 100.0f);

		const Vector3 CLEAR_COLOR(BG_R / 255.0f, BG_G / 255.0f, BG_B / 255.0f);

		int quad_px, quad_py;
		{
			float ndc_x, ndc_y;
			Predict_Ndc(QUAD_X, QUAD_Y, 0.0f, &ndc_x, &ndc_y);
			Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &quad_px, &quad_py);
		}

		// --- Check 2: pixel-verified frame through the FULL real chain ---------
		// RTS3DScene::doRender(camera) -> SubsystemInterface::DRAW() ->
		// RTS3DScene::draw() -> WW3D::Render(this, m_camera) ->
		// scene->Render(rinfo) (virtual dispatch to RTS3DScene::Render,
		// W3DScene.cpp:1176) -> updateFixedLightEnvironments ->
		// Customized_Render -> Visibility_Check -> renderOneObject -> Flush.
		printf("=== Check 2: engine-driven frame through the real RTS3DScene chain ===\n");
		robj->Set_Transform(Matrix3D(Vector3(QUAD_X, QUAD_Y, 0.0f)));

		WW3DErrorType begin_result = WW3D::Begin_Render(true, true, CLEAR_COLOR);
		Check(begin_result == WW3D_ERROR_OK, "2a. WW3D::Begin_Render returned WW3D_ERROR_OK");
		scene->doRender(&camera);
		WW3DErrorType end_result = WW3D::End_Render(true);
		Check(end_result == WW3D_ERROR_OK, "2b. WW3D::End_Render returned WW3D_ERROR_OK");

		{
			unsigned char* fbo = Read_Fbo_Pixels_TopDown(g_W, g_H);
			Check_Pixel(fbo, 5, 5, BG_R, BG_G, BG_B, "2c. background == clear color");
			Check_Pixel(fbo, quad_px, quad_py, TEX_R, TEX_G, TEX_B, "2d. authored texel color at predicted position (through Render->updateFixedLightEnvironments->Customized_Render->Visibility_Check->renderOneObject->Flush)");
			free(fbo);
		}

		// --- Check 3: culling (Visibility_Check's real Cull_Sphere logic) -------
		// Transform the quad's render object far outside the camera
		// frustum, render again, confirm the position where it USED to
		// appear now reads background color - this proves Visibility_Check
		// really culled it this frame (SimpleSceneClass::Visibility_Checked
		// resets to false at the end of every Customized_Render call,
		// W3DScene.cpp:1328, so every doRender() re-evaluates visibility
		// fresh), not merely that Render() was called.
		printf("=== Check 3: culling - quad moved outside the frustum ===\n");
		robj->Set_Transform(Matrix3D(Vector3(OFFSCREEN_X, OFFSCREEN_Y, 0.0f)));

		Check(WW3D::Begin_Render(true, true, CLEAR_COLOR) == WW3D_ERROR_OK, "3a. WW3D::Begin_Render returned WW3D_ERROR_OK");
		scene->doRender(&camera);
		Check(WW3D::End_Render(true) == WW3D_ERROR_OK, "3b. WW3D::End_Render returned WW3D_ERROR_OK");

		{
			unsigned char* fbo = Read_Fbo_Pixels_TopDown(g_W, g_H);
			Check_Pixel(fbo, quad_px, quad_py, BG_R, BG_G, BG_B, "3c. old quad position now reads background (culled by Visibility_Check)");
			free(fbo);
		}

		// --- Check 4: drawTerrainOnly(true) --------------------------------------
		// Move the quad BACK to its original, in-frustum position (so the
		// only reason it would fail to render is the flag itself, not
		// culling), set drawTerrainOnly(true), confirm it does NOT render -
		// proof of RTS3DScene::Customized_Render's own real control-flow
		// branch ("if (m_drawTerrainOnly) { return; }", reached right after
		// the (absent, in this harness) terrain object would have been
		// drawn - W3DScene.cpp's own cited line range).
		printf("=== Check 4: drawTerrainOnly(true) - quad must not render ===\n");
		robj->Set_Transform(Matrix3D(Vector3(QUAD_X, QUAD_Y, 0.0f)));
		scene->drawTerrainOnly(true);

		Check(WW3D::Begin_Render(true, true, CLEAR_COLOR) == WW3D_ERROR_OK, "4a. WW3D::Begin_Render returned WW3D_ERROR_OK");
		scene->doRender(&camera);
		Check(WW3D::End_Render(true) == WW3D_ERROR_OK, "4b. WW3D::End_Render returned WW3D_ERROR_OK");

		{
			unsigned char* fbo = Read_Fbo_Pixels_TopDown(g_W, g_H);
			Check_Pixel(fbo, quad_px, quad_py, BG_R, BG_G, BG_B, "4c. quad position reads background (drawTerrainOnly(true) suppressed it)");
			free(fbo);
		}

		// --- Extra rigor: confirm drawTerrainOnly(false) really un-suppresses
		// rendering, closing the loop between checks 3 and 4 -------------------
		// The quad is still at its original, in-frustum position from check 4
		// (Visibility_Check reruns fresh every frame regardless of
		// m_drawTerrainOnly, W3DScene.cpp:1321-1328, so it was already marked
		// visible even during check 4 - only the RenderList traversal was
		// skipped). Restoring drawTerrainOnly(false) and rendering once more
		// must therefore bring the texel back, proving check 4's suppression
		// was specifically drawTerrainOnly's doing (reversible, this frame's
		// flag only) and not a side effect of check 3's culling somehow
		// persisting.
		scene->drawTerrainOnly(false);

		Check(WW3D::Begin_Render(true, true, CLEAR_COLOR) == WW3D_ERROR_OK, "4d. WW3D::Begin_Render returned WW3D_ERROR_OK");
		scene->doRender(&camera);
		Check(WW3D::End_Render(true) == WW3D_ERROR_OK, "4e. WW3D::End_Render returned WW3D_ERROR_OK");

		{
			unsigned char* fbo = Read_Fbo_Pixels_TopDown(g_W, g_H);
			Check_Pixel(fbo, quad_px, quad_py, TEX_R, TEX_G, TEX_B, "4f. authored texel reappears once drawTerrainOnly(false) is restored (suppression was reversible, not a side effect of check 3's culling)");
			free(fbo);
		}

		// --- Full teardown --------------------------------------------------------
		printf("=== Check 5: teardown ===\n");
		scene->Remove_Render_Object(robj);
		robj->Release_Ref();
		robj = nullptr;
		asset_manager.Free_Assets();

		// Same REF_PTR_RELEASE idiom W3DDisplay.cpp itself uses to tear down
		// m_3DScene (:459) - runs RTS3DScene's real destructor (its
		// REF_PTR_RELEASE chain over m_globalLight/m_infantryLight/
		// m_scratchLight/m_shroudMaterialPass/m_maskMaterialPass/
		// m_heatVisionMaterialPass/m_heatVisionOnlyPass/
		// m_frenzyMaterialPass/m_occludedMaterialPass[], plus its three
		// delete[] buffers).
		REF_PTR_RELEASE(scene);
		Check(scene == nullptr, "5a. REF_PTR_RELEASE(scene) nulled the pointer");

		// Plain delete, matching the real engine's own teardown idiom
		// (GlobalData.cpp:1181's "delete TheWritableGlobalData;") - GlobalData
		// derives from SubsystemInterface, not MemoryPoolObject, so this is
		// NOT a deleteInstance()-managed type despite being allocated via the
		// NEW macro (NEW is just the memory-tracked global operator new,
		// GameMemory.h:878, not the separate memory-pool-object system).
		delete TheWritableGlobalData;
		Check(TheWritableGlobalData == nullptr, "5b. GlobalData::~GlobalData() nulled TheWritableGlobalData (m_theOriginal == this)");

		WW3DErrorType shutdown_result = WW3D::Shutdown();
		Check(shutdown_result == WW3D_ERROR_OK, "5c. WW3D::Shutdown returned WW3D_ERROR_OK");
		Check(!WW3D::Is_Initted(), "5d. WW3D::Is_Initted() is false after Shutdown");

		delete TheW3DFileSystem;
		TheW3DFileSystem = nullptr;
		Check(_TheFileFactory == nullptr, "5e. ~W3DFileSystem restored _TheFileFactory");

		delete TheArchiveFileSystem; TheArchiveFileSystem = nullptr;
		delete TheLocalFileSystem; TheLocalFileSystem = nullptr;
		delete TheFileSystem; TheFileSystem = nullptr;
		Check(TheFileSystem == nullptr && TheLocalFileSystem == nullptr && TheArchiveFileSystem == nullptr && TheW3DFileSystem == nullptr,
			"5f. all four file-system singletons torn down and nulled");

		RemoveIfExists(kArchiveName);
	} // end of the literal nested block - every local declared inside it is
	  // destroyed at this closing brace, BEFORE shutdownMemoryManager() runs
	  // below (Tests/GameFileSystem's real SIGSEGV lesson).

	shutdownMemoryManager();

	TheAsciiStringCriticalSection = nullptr;
	TheUnicodeStringCriticalSection = nullptr;
	TheDmaCriticalSection = nullptr;
	TheMemoryPoolCriticalSection = nullptr;
	TheDebugLogCriticalSection = nullptr;

	if (g_AnyFailure)
	{
		fprintf(stderr, "RENDERRTS3DSCENE_FAIL: one or more checks failed (see above)\n");
		return 1;
	}

	printf("RENDERRTS3DSCENE_OK: all checks passed\n");
	return 0;
}
