// Phase 5(a) Milestone 7 Task 4 (native port plan, Draft 28, "Implementation
// ordering" step 4) - the milestone's exit harness: every byte on screen
// comes out of a real BIG archive through the engine's own file stack -
// GameFileClass -> TheFileSystem -> StdBIGFile -> RAMFile - not a loose temp
// file. This combines Milestone 6's engine-driven frame loop
// (Tests/RenderWW3DFrame/main.cpp, reused verbatim for the render-loop half)
// with Milestone 7 Tasks 2/3's file-system stack
// (Tests/GameFileSystem/main.cpp, reused verbatim for the prologue half) for
// the first time. See docs/native-port-plan.md's Draft 28 section (findings
// 1-7, design decisions, open questions) for the full design rationale.
//
// Prologue (Task 2's, unchanged): five real CriticalSection objects wired to
// the five global pointers (mirrors WinMain.cpp:875-882), THEN
// initMemoryManager(). GameEngine file-system TUs allocate via
// newInstance/MSGNEW, which need the real memory manager up first.
//
// Structure, in the engine's own order (GameEngine.cpp:403-445's file-system
// spine, then W3DDisplay::init's TheW3DFileSystem construction,
// W3DDisplay.cpp:754 - both stages this harness reaches without needing
// W3DDisplay itself, WorldBuilder.cpp:408's real precedent for
// separability):
//   1. Author the quad's .w3d and .tga bytes (Tests/RenderWW3DFrame's
//      authoring code, trimmed and adapted), embed them into a
//      test-authored .big archive at Art\W3D\quad.w3d / Art\Textures\
//      quad.tga (backslashed, matching real retail archives' internal path
//      convention - confirmed against the user's actual Textures.big,
//      Draft 28 open question 6), delete every loose authoring temp file
//      before any file-system object is constructed - the bytes exist
//      NOWHERE on disk outside the archive from that point on.
//   2. Construct TheFileSystem/TheLocalFileSystem/TheArchiveFileSystem,
//      TheFileSystem->init() (cascades into both, loads the freshly-authored
//      .big via StdBIGFileSystem::init()'s own cwd "*.big" scan).
//   3. Construct TheW3DFileSystem - its constructor performs the
//      "_TheFileFactory = this" swap (W3DFileSystem.cpp:438-445, now
//      Core-unified), replacing WWLib's default factory every other harness
//      implicitly uses.
//   4. WW3D::Init -> Set_Render_Device(windowed=1) -> Set_Thumbnail_Enabled
//      (false) + DX8Wrapper::Set_Texture_Bitdepth(32) BEFORE the first
//      render (Milestone 6's own precedent - the async texture-load race
//      Milestone 6 Task 5 found and fixed; do not repeat that bug here).
//
// Four checks:
//   1. Init round-trip (same as Milestone 6's check 1).
//   2. Asset-manager load by BARE name ("quad.w3d") succeeds - the proof
//      that factory->TheFileSystem->archive resolution actually works, not
//      just that the file bytes happen to be readable some other way.
//   3. A pixel-verified frame with the authored texel color at the
//      CPU-predicted position (same rigor as Milestone 6's check 2) - the
//      byte-provenance claim here is stronger: these exact bytes exist
//      nowhere on disk outside the archive.
//   4. Negative control: tear down and reconstruct the file-system stack
//      (fresh FileSystem/LocalFileSystem/ArchiveFileSystem objects - the
//      old ones' ENABLE_FILESYSTEM_EXISTENCE_CACHE-cached doesFileExist
//      results must not leak into this check, GameDefines.h:145) with the
//      archive file renamed away, then re-attempt the SAME bare-name load
//      through a FRESH WW3DAssetManager and confirm it FAILS cleanly (no
//      crash, Load_3D_Assets returns false). Guards against a silent
//      fallback to some other file-loading path making check 3 pass for
//      the wrong reason.
// Then full teardown: scene/asset cleanup, WW3D::Shutdown(),
// ~W3DFileSystem's _TheFileFactory restore (verified directly against the
// real WWLib global), both file-system generations deleted.
#include "PreRTS.h"

#include "Common/AsciiString.h"
#include "Common/CriticalSection.h"
#include "Common/file.h"
#include "Common/FileSystem.h"
#include "Common/LocalFileSystem.h"
#include "Common/ArchiveFileSystem.h"
#include "Common/RAMFile.h"
#include "Common/GameMemory.h"
#include "StdDevice/Common/StdLocalFileSystem.h"
#include "StdDevice/Common/StdBIGFileSystem.h"
#include "W3DDevice/GameClient/W3DFileSystem.h"

#include "WWLib/ffactory.h" // _TheFileFactory - check 4's teardown verification.

#include "dx8wrapper.h"
#include "ww3d.h"
#include "camera.h"
#include "scene.h"
#include "rendobj.h"
#include "assetmgr.h"
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
		fprintf(stderr, "RENDERGAMEASSETS_FAIL: %s\n", what);
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
	// Tests/RenderWW3DFrame).
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

	// Reads the currently-bound GL_READ_FRAMEBUFFER (the offscreen FBO -
	// Present's own blit leaves it bound as GL_FRAMEBUFFER right after an
	// End_Render, matching every other harness's plain glReadPixels usage)
	// and returns it top-down (row 0 = top of screen) as a freshly malloc'd
	// W*H*4 RGBA buffer. Caller frees.
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
	// same mechanics as Tests/RenderWW3DFrame/main.cpp (itself templated on
	// Tests/RenderW3DMesh's real ChunkSaveClass authoring), trimmed to
	// exactly what this harness needs: a single textured two-triangle quad.
	// Unlike RenderWW3DFrame, the bytes never stay on disk as loose files -
	// each is authored to a throwaway temp path, slurped into memory, and
	// the temp file is deleted immediately, all BEFORE any file-system
	// object is constructed or any archive is loaded. From that point on,
	// the only place these exact bytes exist on disk is inside the
	// test-authored .big archive.

	std::string Temp_Path(const char* leaf)
	{
		const char* tmp = getenv("TMPDIR");
		if (!tmp) tmp = "/tmp";
		char buf[512];
		snprintf(buf, sizeof(buf), "%s/rgassets_%d_%s", tmp, static_cast<int>(getpid()), leaf);
		return std::string(buf);
	}

	// Slurps an entire file into a std::string via plain stdio (Draft 24's
	// "author/verify ground truth independently of the code under test"
	// precedent - this must NOT go through TheFileSystem/File, since the
	// whole point is to embed these exact bytes into the archive this
	// harness authors, before any engine file-system object exists), then
	// deletes the on-disk temp file so it can never satisfy any later
	// lookup by accident.
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

	// Real, well-formed uncompressed 32-bit TGA (same layout every prior
	// harness's Write_TGA authors) - width*height*4 bytes, B,G,R,A per
	// pixel, one row after another.
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
	// own local origin: 4 vertices, 2 triangles (0,1,2 / 0,2,3), CCW-wound.
	// Byte-for-byte the same chunk layout as Tests/RenderWW3DFrame's
	// Write_Mesh_Chunk, with one deliberate difference: texture_name here
	// is the BARE archive-relative leaf name ("quad.tga"), not a full local
	// path - GameFileClass::Set_Name (W3DFileSystem.cpp:184-392) is what
	// turns a bare name into "Art/Textures/quad.tga" via TGA_DIR_PATH, so
	// the embedded name must be exactly what a real .w3d ships (confirmed
	// against the retail spot check: real material chunks store bare names
	// like "aametalwall.dds", never full paths).
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

	// --- .big archive authoring: byte-for-byte the same format
	// Tests/GameFileSystem/main.cpp's AuthorBigArchive writes, matching
	// StdBIGFileSystem::openArchiveFile's exact consumption format
	// (StdBIGFileSystem.cpp:81-180, Draft 28 finding 7): "BIGF" magic,
	// archive size (read raw and unused by the reader), entry count
	// (big-endian), directory at seek 0x10, per-entry big-endian
	// offset+size + nul-terminated path. Paths use '\' separators here,
	// matching the confirmed real-retail-archive convention (Draft 28
	// open question 6's spot check: e.g. "Art\Textures\aametalwall.dds")
	// even though FileSystem.h's W3D_DIR_PATH/TGA_DIR_PATH #defines use
	// forward slashes - ArchiveFileSystem::getArchivedDirectoryInfo/
	// ArchiveFile::getArchivedFileInfo tokenize on "\\/" (both accepted),
	// so this is a genuine, verified normalization, not an assumption.
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

	const char* const kArchiveName = "RenderGameAssets.big";
	const char* const kArchiveRenamedAway = "RenderGameAssets.big.removed_for_negative_control";

	const int BG_R = 40, BG_G = 70, BG_B = 110;
	const int TEX_R = 180, TEX_G = 200, TEX_B = 30;

} // end anonymous namespace - RunAllChecksThenTearDown's body below is a
  // literal nested block inside main() itself, not a separate function (see
  // main()'s own comment for why that distinction is load-bearing).

int main()
{
	// Line-buffer both streams so a crash mid-run still leaves every check
	// result up to that point on disk (Tests/GameFileSystem's precedent).
	setvbuf(stdout, nullptr, _IOLBF, 0);
	setvbuf(stderr, nullptr, _IOLBF, 0);

	// ---- Prologue: mirrors WinMain.cpp:875-882 exactly (Task 2's, unchanged). ----
	static CriticalSection critSec1, critSec2, critSec3, critSec4, critSec5;
	TheAsciiStringCriticalSection = &critSec1;
	TheUnicodeStringCriticalSection = &critSec2;
	TheDmaCriticalSection = &critSec3;
	TheMemoryPoolCriticalSection = &critSec4;
	TheDebugLogCriticalSection = &critSec5;

	initMemoryManager();

	// All checks and every local this harness needs, in one BLOCK BEFORE
	// shutdownMemoryManager() - deliberately, not incidentally (Tests/
	// GameFileSystem's own found-by-gdb lesson: GameMemory.cpp overrides the
	// process-global operator new/delete, so any local with a non-trivial
	// destructor still in scope when main() returns would free its storage
	// AFTER shutdownMemoryManager() has torn that allocator down - a real
	// SIGSEGV that harness's first draft hit and fixed exactly this way).
	// This MUST be a literal nested block lexically inside main() itself,
	// NOT a separate function or lambda: dx8wrapper.h:711 declares
	// "friend int main();", which grants access to DX8Wrapper::
	// Set_Texture_Bitdepth (a protected static member, dx8wrapper.h:605)
	// only to the free function literally named main() in the global
	// namespace - not to any function main() calls, and not to a lambda's
	// call operator either (both are separate, non-friended entities). This
	// was found the hard way during this task's development: the very same
	// call, textually identical, compiled from inside a helper function but
	// failed with "is protected within this context"; a minimal repro
	// isolated the cause to lexical scope, not any preprocessor define or
	// include-order difference.
	{
		// ---- Stability across repeated runs: wipe every path this harness
		// authors BEFORE constructing the archive file system, so its own
		// init()-time cwd scan (StdBIGFileSystem::init ->
		// loadBigFilesFromDirectory("", "*.big")) never picks up a stale
		// archive left by a prior run. ----
		RemoveIfExists(kArchiveName);
		RemoveIfExists(kArchiveRenamedAway);

		printf("=== Authoring: quad.w3d + quad.tga bytes into a test-authored .big archive ===\n");
		std::string tgaBytes = Author_Quad_TGA_Bytes(4, 4, TEX_R, TEX_G, TEX_B);
		Check(!tgaBytes.empty(), "0a. authored TGA bytes are non-empty");
		std::string w3dBytes = Author_Quad_W3D_Bytes("RGA", "Quad", "quad.tga");
		Check(!w3dBytes.empty(), "0b. authored W3D bytes are non-empty");

		// Backslashed internal paths, matching the confirmed real-retail
		// convention - "Art\W3D\quad.w3d" / "Art\Textures\quad.tga" is
		// exactly what GameFileClass::Set_Name (W3DFileSystem.cpp:222-237)
		// builds from a bare "quad.w3d"/"quad.tga" request via
		// W3D_DIR_PATH/TGA_DIR_PATH ("Art/W3D/", "Art/Textures/").
		std::vector<ArchiveEntry> entries = {
			{ "Art\\W3D\\quad.w3d",     w3dBytes },
			{ "Art\\Textures\\quad.tga", tgaBytes },
		};
		AuthorBigArchive(kArchiveName, entries);
		// From this point on, tgaBytes/w3dBytes are still held in memory
		// (for later byte-exact comparison against the rendered pixel), but
		// no LOOSE file containing them exists anywhere on disk - only the
		// just-authored .big archive.

		// ---- Construct the file-system stack, in GameEngine::init()'s own
		// spine order (GameEngine.cpp:403-445, minus the subsystem list) -
		// GUIEdit.cpp's real, shipped precedent (:478-483), Std twins
		// swapped in for Win32's. ----
		TheFileSystem = new FileSystem;
		TheLocalFileSystem = new StdLocalFileSystem;
		TheArchiveFileSystem = new StdBIGFileSystem;
		TheFileSystem->init(); // cascades into TheLocalFileSystem->init() and
		                        // TheArchiveFileSystem->init() (FileSystem.cpp:
		                        // 143-147) - StdBIGFileSystem::init() scans
		                        // cwd for "*.big" and finds the archive
		                        // authored above.

		// ---- Construct TheW3DFileSystem - the engine's own real mechanism
		// (W3DFileSystem.cpp:438-445): the constructor sets
		// "_TheFileFactory = this", replacing WWLib's default factory every
		// other harness implicitly uses. Must happen AFTER TheArchiveFileSystem
		// exists and has loaded the archive - the constructor's
		// reprioritizeTexturesBySize() (RTS_ZEROHOUR && PRIORITIZE_TEXTURES_
		// BY_SIZE, both on by default for the GeneralsMD flavor every harness
		// uses) dereferences TheArchiveFileSystem directly. ----
		TheW3DFileSystem = new W3DFileSystem;
		Check(_TheFileFactory == static_cast<FileFactoryClass*>(TheW3DFileSystem),
			"0c. W3DFileSystem constructor installed itself as _TheFileFactory");

		// --- Check 1: init round-trip (Milestone 6's own check 1, unchanged) ---
		printf("=== Check 1: init round-trip ===\n");
		WW3DErrorType init_result = WW3D::Init(nullptr);
		Check(init_result == WW3D_ERROR_OK, "1a. WW3D::Init returned WW3D_ERROR_OK");

		WW3DErrorType device_result = WW3D::Set_Render_Device(0, g_W, g_H, 32, /*windowed=*/1, /*resize_window=*/true);
		Check(device_result == WW3D_ERROR_OK, "1b. WW3D::Set_Render_Device returned WW3D_ERROR_OK");

		if (init_result != WW3D_ERROR_OK || device_result != WW3D_ERROR_OK)
		{
			fprintf(stderr, "RENDERGAMEASSETS_FAIL: init round-trip failed, aborting\n");
			g_AnyFailure = true;
			// Early-abort return, matching Tests/RenderWW3DFrame's own
			// precedent for this class of unrecoverable setup failure:
			// still lexically inside main()'s nested block, so every local
			// constructed so far (the authored .tga/.w3d byte buffers, the
			// entries vector) is properly destroyed by normal stack
			// unwinding before this return takes effect - only the later
			// shutdownMemoryManager()/CriticalSection-nulling epilogue is
			// skipped, which is harmless (process exit reclaims everything)
			// and consistent with this being a genuine abort path, not the
			// success path check 5 exercises.
			return 1;
		}

		// Milestone 6 Task 5's own precedent, verbatim rationale: thumbnails
		// default to enabled, and TextureClass::Init only takes the
		// synchronous Request_Foreground_Loading path when thumbnails are
		// off - otherwise the very first WW3D::Render below races the real
		// background TextureLoader pthread and sees MissingTexture's
		// placeholder instead of the authored texel. Must be set BEFORE the
		// first render.
		WW3D::Set_Thumbnail_Enabled(false);
		DX8Wrapper::Set_Texture_Bitdepth(32);

		Check(WW3D::Get_Render_Device_Count() == 1, "1c. Get_Render_Device_Count()==1");
		const char* device_name = WW3D::Get_Render_Device_Name(0);
		Check(device_name != nullptr && device_name[0] != '\0', "1d. Get_Render_Device_Name non-empty");
		Check(DX8Wrapper::Get_Current_Caps() != nullptr, "1e. DX8Wrapper::Get_Current_Caps() non-null");

		// --- Check 2: asset-manager load by BARE name succeeds -----------------
		// This is the proof that factory->TheFileSystem->archive resolution
		// actually works, not just that the file bytes happen to be
		// readable some other way: "quad.w3d" is a bare name with no
		// directory component. WW3DAssetManager::Load_3D_Assets(const char*)
		// (assetmgr.cpp:630-645) calls _TheFileFactory->Get_File(filename)
		// directly - with TheW3DFileSystem installed, that's
		// W3DFileSystem::Get_File -> new GameFileClass("quad.w3d") ->
		// Set_Name maps it to "Art/W3D/quad.w3d" -> TheFileSystem->
		// doesFileExist/openFile -> local-miss -> archive-hit -> RAMFile.
		printf("=== Check 2: asset-manager load by bare name (\"quad.w3d\") ===\n");
		WW3DAssetManager asset_manager;
		bool loaded = asset_manager.Load_3D_Assets("quad.w3d");
		Check(loaded, "2a. Load_3D_Assets(\"quad.w3d\") succeeded via factory->TheFileSystem->archive resolution");

		RenderObjClass* robj = nullptr;
		if (loaded)
		{
			robj = asset_manager.Create_Render_Obj("RGA.Quad");
		}
		Check(robj != nullptr, "2b. Create_Render_Obj(\"RGA.Quad\") returned non-null");

		if (!robj)
		{
			fprintf(stderr, "RENDERGAMEASSETS_FAIL: could not create render object, aborting remaining checks\n");
			WW3D::Shutdown();
			g_AnyFailure = true;
			return 1; // same early-abort rationale as check 1's abort path above.
		}

		SimpleSceneClass scene;
		scene.Add_Render_Object(robj);

		// Camera at +Z0 with identity rotation, looking back at the quad
		// (Draft 25's -Z-forward lesson, same convention as every prior
		// rendering harness).
		CameraClass camera;
		Matrix3D camera_tm(Vector3(0.0f, 0.0f, Z0));
		camera.Set_Transform(camera_tm);
		camera.Set_View_Plane(1.57079632679489661923f, 1.57079632679489661923f); // 90deg h/v FOV
		camera.Set_Clip_Planes(1.0f, 100.0f);

		const Vector3 CLEAR_COLOR(BG_R / 255.0f, BG_G / 255.0f, BG_B / 255.0f);

		// --- Check 3: a pixel-verified frame, authored texel at predicted position ---
		printf("=== Check 3: engine-driven frame, authored texel color at predicted position ===\n");
		const float QUAD_X = 0.9f, QUAD_Y = -0.4f;
		robj->Set_Transform(Matrix3D(Vector3(QUAD_X, QUAD_Y, 0.0f)));

		WW3DErrorType begin_result = WW3D::Begin_Render(true, true, CLEAR_COLOR);
		Check(begin_result == WW3D_ERROR_OK, "3a. WW3D::Begin_Render returned WW3D_ERROR_OK");
		WW3DErrorType render_result = WW3D::Render(&scene, &camera);
		Check(render_result == WW3D_ERROR_OK, "3b. WW3D::Render returned WW3D_ERROR_OK");
		WW3DErrorType end_result = WW3D::End_Render(true);
		Check(end_result == WW3D_ERROR_OK, "3c. WW3D::End_Render returned WW3D_ERROR_OK");

		{
			unsigned char* fbo = Read_Fbo_Pixels_TopDown(g_W, g_H);
			// Background: sample a corner, well away from the quad.
			Check_Pixel(fbo, 5, 5, BG_R, BG_G, BG_B, "3d. background == clear color");
			int px, py; float ndc_x, ndc_y;
			Predict_Ndc(QUAD_X, QUAD_Y, 0.0f, &ndc_x, &ndc_y);
			Ndc_To_Pixel_TopDown(ndc_x, ndc_y, &px, &py);
			Check_Pixel(fbo, px, py, TEX_R, TEX_G, TEX_B, "3e. authored texel color at predicted position (bytes came ONLY from the archive)");
			free(fbo);
		}

		// --- Check 4: negative control -------------------------------------------
		// Release the positive-path scene/render-object/asset-manager state
		// first (nothing from here on should still reference the
		// soon-to-be-deleted first-generation file-system objects), then
		// tear down and reconstruct TheFileSystem/TheLocalFileSystem/
		// TheArchiveFileSystem from scratch - a fresh FileSystem object has
		// no ENABLE_FILESYSTEM_EXISTENCE_CACHE-cached doesFileExist results
		// (GameDefines.h:145, on by default), so this is a clean re-test,
		// not one that could silently pass due to a stale cache entry.
		// TheW3DFileSystem stays installed throughout (only its final
		// teardown happens later, in the "full teardown" section) - the
		// negative control exercises the SAME factory->TheFileSystem->
		// archive code path, just with the archive genuinely gone.
		printf("=== Check 4: negative control (archive absent) ===\n");
		scene.Remove_Render_Object(robj);
		robj->Release_Ref();
		robj = nullptr;
		asset_manager.Free_Assets();

		delete TheArchiveFileSystem; TheArchiveFileSystem = nullptr;
		delete TheLocalFileSystem; TheLocalFileSystem = nullptr;
		delete TheFileSystem; TheFileSystem = nullptr;

		std::error_code renameEc;
		fs::rename(kArchiveName, kArchiveRenamedAway, renameEc);
		Check(!renameEc, "4a. archive file renamed away without error");
		Check(!fs::exists(kArchiveName), "4b. archive file genuinely absent from its expected cwd-relative path");

		TheFileSystem = new FileSystem;
		TheLocalFileSystem = new StdLocalFileSystem;
		TheArchiveFileSystem = new StdBIGFileSystem;
		TheFileSystem->init(); // fresh cwd "*.big" scan - finds nothing now.

		WW3DAssetManager negative_asset_manager;
		bool loadedAfterRemoval = negative_asset_manager.Load_3D_Assets("quad.w3d");
		Check(!loadedAfterRemoval, "4c. Load_3D_Assets(\"quad.w3d\") FAILS cleanly (returns false, no crash) with the archive absent");
		negative_asset_manager.Free_Assets(); // no-op: nothing was loaded.

		// --- Full teardown --------------------------------------------------------
		printf("=== Teardown ===\n");
		asset_manager.Free_Assets(); // idempotent - matches Tests/RenderWW3DFrame's precedent.

		WW3DErrorType shutdown_result = WW3D::Shutdown();
		Check(shutdown_result == WW3D_ERROR_OK, "5a. WW3D::Shutdown returned WW3D_ERROR_OK");
		Check(!WW3D::Is_Initted(), "5b. WW3D::Is_Initted() is false after Shutdown");

		delete TheW3DFileSystem;
		TheW3DFileSystem = nullptr;
		Check(_TheFileFactory == nullptr, "5c. ~W3DFileSystem restored _TheFileFactory (W3DFileSystem.cpp:524-527)");

		delete TheArchiveFileSystem; TheArchiveFileSystem = nullptr;
		delete TheLocalFileSystem; TheLocalFileSystem = nullptr;
		delete TheFileSystem; TheFileSystem = nullptr;
		Check(TheFileSystem == nullptr && TheLocalFileSystem == nullptr && TheArchiveFileSystem == nullptr && TheW3DFileSystem == nullptr,
			"5d. all four file-system singletons torn down and nulled");

		RemoveIfExists(kArchiveRenamedAway);
	} // end of the literal nested block - every local declared inside it
	  // (asset_manager, negative_asset_manager, scene, camera, all the
	  // std::string/std::vector authoring buffers) is destroyed at this
	  // closing brace, BEFORE shutdownMemoryManager() runs below.

	// Mirrors WinMain.cpp:1000/1011-1013 - shutdownMemoryManager() and
	// clearing the CriticalSection globals happen last, with no heap-backed
	// local of this harness's own left alive above them (Tests/
	// GameFileSystem's real SIGSEGV lesson - see that harness's report).
	shutdownMemoryManager();

	TheAsciiStringCriticalSection = nullptr;
	TheUnicodeStringCriticalSection = nullptr;
	TheDmaCriticalSection = nullptr;
	TheMemoryPoolCriticalSection = nullptr;
	TheDebugLogCriticalSection = nullptr;

	if (g_AnyFailure)
	{
		fprintf(stderr, "RENDERGAMEASSETS_FAIL: one or more checks failed (see above)\n");
		return 1;
	}

	printf("RENDERGAMEASSETS_OK: all checks passed\n");
	return 0;
}
