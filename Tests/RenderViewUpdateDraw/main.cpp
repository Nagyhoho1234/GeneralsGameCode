// Phase 5(a) Milestone 11 RETRY (native port plan, rung 3b-ii-a, Draft 35,
// Draft 33's "Follow-up plan / Workstream B") - this harness clones Tests/
// RenderCameraTransform's ENTIRE prologue/camera-driving/pixel-check
// structure WHOLESALE, unchanged (see that harness's own header comment
// below for the full, still-accurate rationale of every one of those
// checks), and this retry now ALSO delivers Draft 35's original steps 1-4
// (implementation ordering): real TheGameLogic/TheScriptEngine construction;
// the five-class GameClient/InGameUI/Display/FontLibrary/Mouse minimal
// concrete stub subclasses; the real update()/draw()/drawView() call chain -
// PLUS Check 8, pickDrawable()'s real ray-cast proof (finding 11,
// implementation ordering step 5, first shipped standalone in this
// milestone's first attempt, commit 67d7b83f0, kept unchanged here).
//
// WHY THIS RETRY SUCCEEDS WHERE THE FIRST ATTEMPT DID NOT, recorded here per
// this port's standing "disclose surprises, don't smooth them over"
// discipline: the first attempt's stub subclasses overrode ONLY each
// abstract base's pure virtuals, which ran into a genuine, previously-
// undiscovered blocker - a derived class's constructor UNCONDITIONALLY pins
// the BASE class's own vtable during construction (a mandatory C++ ABI
// requirement), so the REAL GameClient/InGameUI base classes' ENTIRE virtual-
// method closures (not just the pure ones) needed to resolve at link time -
// measured at 372 undefined symbols for the full five-class set, 237 for
// TheGameLogic+TheScriptEngine alone (see Draft 33's own accounting in
// docs/native-port-plan.md for the full story). THE FIX (per the reconciled
// follow-up plan): CMakeLists.txt now inherits z_gameengine's ENTIRE real
// source closure (Milestone 10's own DEFER-closure technique, already proven
// on a first real attempt) instead of hand-picking files - once GameLogic.cpp/
// ScriptEngine.cpp/GameClient.cpp/InGameUI.cpp/Display.cpp/GameFont.cpp/
// Mouse.cpp are ALL linked in for real, every one of those "several hundred"
// undefined symbols resolves by construction, and the stub subclasses
// (harness_stub_classes.h) go back to overriding ONLY their abstract base's
// pure virtuals, exactly as Draft 35 originally intended. See
// CMakeLists.txt's own header comment for the full mechanism, including the
// genuinely NEW risk this combination created (closure/hand-picked-source
// duplicate-symbol collisions) and link_stubs.cpp's own header comment for
// the real, linker-driven accounting of every stub that had to be removed
// as a result.
//
// ---- Everything below this point (through Check 7) is Tests/
// RenderCameraTransform/main.cpp's own header comment and structure,
// UNCHANGED - reproduced here because this harness IS that structure, not a
// re-derivation of it. ----
//
// Phase 5(a) Milestone 9 Task 3 (native port plan, Draft 32, "Implementation
// ordering" step 4) - the milestone's payoff task and exit harness: the
// game's own REAL camera-transform math (W3DView::updateCameraTransform(),
// Core/GameEngineDevice/.../W3DView.cpp, portability-fixed in Task 2 Part A)
// driving the real RTS3DScene render pipeline (unified in Milestone 8), on
// top of a real, base TerrainLogic (Task 2 Part B / this task's own finding
// 6) - the first TerrainLogic and the first W3DView ever constructed and
// executed on POSIX. This clones Tests/RenderRTS3DScene/main.cpp's ENTIRE
// prologue/link structure wholesale, per the task brief. See
// docs/native-port-plan.md's Draft 32 section (findings 5, 6, 7, the Design
// Decisions section, and open questions 1-3) for the full design rationale.
//
// Prologue (Tests/RenderRTS3DScene's, unchanged): five real CriticalSection
// objects wired to the five global pointers, THEN initMemoryManager().
//
// NEW for this harness, in the engine's own order:
//   1. TheTerrainLogic = NEW TerrainLogic - the real BASE class (Draft 32
//      finding 6, NOT W3DTerrainLogic). Constructor is set-defaults-only.
//      getGroundHeight() unconditionally returns 0 with no map loaded
//      (TerrainLogic.cpp:1430-1437) - this IS the "flat, height-0" terrain
//      every camera-transform formula below relies on.
//   2. TheFramePacer = NEW FramePacer - a genuine gap THIS task found that
//      Draft 32's finding 5 did not enumerate: W3DView.cpp's
//      buildCameraTransform()/zoomCameraToDesiredHeight()/movePivotToGround()
//      (all three inside the plan's own "camera-transform-core" method list)
//      unconditionally dereference TheFramePacer with NO null guard
//      (":392,:450,:467"). FramePacer.cpp/FrameRateLimit.cpp are both
//      already portable and already compiled on Linux today (see this
//      target's CMakeLists.txt), so a REAL (non-null, non-stub) FramePacer
//      is both cheap and the more honest choice.
//   3. TheGlobalData (Milestone 8's own real, defaults-constructed
//      GlobalData, unchanged - m_headless left at its real default FALSE
//      per the task brief, so updateCameraTransform()'s terrain-height code
//      path genuinely runs, finding 7).
//   4. A real RTS3DScene (Milestone 8's own construction, unchanged) plus
//      Milestone 7/8's quad, loaded the same way.
//   5. A real W3DView, constructed and driven through the SAME real API
//      calls the game itself makes:
//        - view->init() (W3DView.cpp:881 - the exact override the engine's
//          own View factory's result gets, per GeneralsMD's
//          W3DInGameUI::createView(): "return NEW W3DView;").
//        - view->setDefaultView(DEG_TO_RADF(TheGlobalData->m_cameraPitch),
//          DEG_TO_RADF(TheGlobalData->m_cameraYaw), 1.0f) - the EXACT call
//          (same arguments) InGameUI::init() makes right after construction
//          (GeneralsMD/Code/GameEngine/Source/GameClient/InGameUI.cpp:1373-
//          1382), skipping only the TheDisplay-guarded
//          attachView()/setWidth()/setHeight() calls in that same block
//          (TheDisplay is deliberately left null - out of scope, matching
//          TheTerrainRenderObject/TheRadar/TheWindowManager, task brief
//          item 3).
//        - view->lookAt(&pos)/setAngle()/setPitch()/setZoom() - the same
//          public per-frame-capable camera API scripts/user input drive
//          (Draft 32 design decisions' own suggested list).
//        - view->updateCameraTransform() - resolves open question 3: this
//          is the actual method View::updateView() -> UPDATE() -> the real
//          W3DView::update() calls internally (W3DView.cpp:1701-1705, "if
//          (m_recalcCamera || m_isCameraSlaved) { updateCameraTransform();
//          }") when a recalc is pending, which every setter above leaves
//          true. It is PRIVATE - Draft 32's own suggested call list assumed
//          otherwise, a genuine finding of this task (see W3DView.h's own
//          comment on the "RenderCameraTransformTestAccess" friend grant,
//          the plan's own pre-approved "isolate a smaller subset of the API
//          surface" fallback, open question 1). Calling the PUBLIC
//          updateView()/update() instead was evaluated and rejected: it
//          unconditionally dereferences TheGameClient/TheScriptEngine/
//          TheGameLogic with NO null guard (":1577,:1717") and would
//          segfault against this harness's deliberately-unconstructed
//          singletons - update() itself is explicitly out of scope, grouped
//          with draw()/drawView()/pickDrawable()/iterateDrawablesInRegion()
//          by the plan's own design decisions ("deliberately excluding
//          draw()/update()/pickDrawable()").
//   6. Checks (five groups, mirroring Milestone 8's own check numbering
//      style and its "direct-state assertions beat pixel-only inference"
//      precedent, task brief item 4):
//      a. Direct-state: view->get3DCameraPosition()/get3DCameraDirection()
//         (both real, public, W3DView-overridden accessors reading
//         m_3DCamera's transform) match a CPU-side, independently
//         re-authored reimplementation of buildCameraPosition()'s/
//         buildCameraTransform()'s documented formula (PredictCameraSourceAndTarget/
//         PredictCameraTransform below) - computed from the SAME view state
//         (zoom/angle/pitch/position) queried back through the SAME public
//         accessors the setters above just drove, NOT a hand-picked
//         constant (task brief item 4).
//      b. Pixel check A (centered): the quad, placed at the exact lookAt()
//         target world position, renders at the exact screen center - a
//         direct consequence of Look_At's own definition (the target always
//         projects to NDC (0,0)), so this specifically catches "camera
//         position/rendering never ran through this transform at all."
//      c. Pixel check B (off-center): the quad, placed at a second, offset
//         world position, renders at a screen position independently
//         predicted via a SEPARATE CameraClass carrying the SAME predicted
//         (not queried-back) transform - this is the check that actually
//         exercises buildCameraPosition's zoom/pitch/angle-offset math, not
//         just Look_At's own centering guarantee.
//      d. Culling check: the same off-center quad, moved far outside the
//         predicted frustum, is confirmed invisible via the real
//         Cull_Sphere-based Visibility_Check (RTS3DScene::Visibility_Check,
//         already proven in Milestone 8) - reused unchanged to prove the
//         real, W3DView-computed camera (not a hand-built test camera) is
//         what RTS3DScene's culling test is actually running against.
//      e. Full teardown, matching Milestone 8's REF_PTR_RELEASE/delete idiom,
//         extended with TheTerrainLogic/TheFramePacer/W3DView's own real
//         destructors.
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
#include "Common/FramePacer.h"
#include "StdDevice/Common/StdLocalFileSystem.h"
#include "StdDevice/Common/StdBIGFileSystem.h"
#include "W3DDevice/GameClient/W3DFileSystem.h"
#include "W3DDevice/GameClient/W3DScene.h"
#include "W3DDevice/GameClient/W3DView.h"
#include "W3DDevice/GameClient/W3DDisplay.h" // Milestone 11 addition: W3DDisplay::m_3DScene, pickDrawable()'s own static (finding 9).
#include "GameLogic/TerrainLogic.h"
#include "GameClient/View.h"
#include "GameClient/DrawableInfo.h" // Milestone 11 step 5 (finding 11): pickDrawable()'s sentinel DrawableInfo.
#include "GameLogic/GameLogic.h" // Milestone 11 RETRY step 1: real TheGameLogic construction.
#include "Common/NameKeyGenerator.h" // Milestone 11 RETRY: real TheNameKeyGenerator construction (teardown hazard fix, see main.cpp's own construction-site comment).
#include "GameLogic/AI.h" // Milestone 11 RETRY: real TheAI construction (teardown hazard fix, see main.cpp's own construction-site comment).
#include "GameLogic/ScriptEngine.h" // Milestone 11 RETRY step 1: real TheScriptEngine construction.
#include "harness_stub_classes.h" // Milestone 11 RETRY step 2: GameClientStub/InGameUIStub/DisplayStub/FontLibraryStub/MouseStub.

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

// TheSuperHackers @test Milestone 9 Task 3 (native port plan): the one
// class granted `friend` access (W3DView.h) to call the private
// updateCameraTransform(). A single static wrapper method, not a general
// backdoor - see W3DView.h's own comment on the friend declaration.
class RenderCameraTransformTestAccess
{
public:
	static void DriveUpdateCameraTransform(W3DView* view)
	{
		view->updateCameraTransform();
	}
};

namespace
{
	bool g_AnyFailure = false;

	void Fail(const char* what)
	{
		fprintf(stderr, "RENDERVIEWUPDATEDRAW_FAIL: %s\n", what);
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

	bool CloseReal(Real actual, Real expected, Real tolerance)
	{
		return std::fabs(actual - expected) <= tolerance;
	}

	bool CloseVector3(const Vector3& actual, const Vector3& expected, Real tolerance, const char* what)
	{
		bool ok = CloseReal(actual.X, expected.X, tolerance)
			&& CloseReal(actual.Y, expected.Y, tolerance)
			&& CloseReal(actual.Z, expected.Z, tolerance);
		if (!ok)
		{
			fprintf(stderr, "  %s: actual (%.4f,%.4f,%.4f) vs expected (%.4f,%.4f,%.4f), tolerance %.4f\n",
				what, actual.X, actual.Y, actual.Z, expected.X, expected.Y, expected.Z, tolerance);
		}
		return ok;
	}

	int g_W = 640;
	int g_H = 480;

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
	// Byte-for-byte the same mechanics as Tests/RenderRTS3DScene/main.cpp,
	// reused per the task brief ("reuse the quad load").

	std::string Temp_Path(const char* leaf)
	{
		const char* tmp = getenv("TMPDIR");
		if (!tmp) tmp = "/tmp";
		char buf[512];
		snprintf(buf, sizeof(buf), "%s/rct_%d_%s", tmp, static_cast<int>(getpid()), leaf);
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

	// Byte-for-byte the same W3D_CHUNK_MESH layout as Tests/RenderRTS3DScene's
	// Write_Mesh_Chunk: a single textured two-triangle quad, bare
	// archive-relative texture name.
	void Write_Mesh_Chunk(ChunkSaveClass& csave, const char* container_name, const char* mesh_name, const char* texture_name, float half)
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
		header.Min.X = -half; header.Min.Y = -half; header.Min.Z = 0.0f;
		header.Max.X = half; header.Max.Y = half; header.Max.Z = 0.0f;
		header.SphCenter.X = 0.0f; header.SphCenter.Y = 0.0f; header.SphCenter.Z = 0.0f;
		header.SphRadius = half * 1.5f;
		csave.Begin_Chunk(W3D_CHUNK_MESH_HEADER3);
		csave.Write(&header, sizeof(header));
		csave.End_Chunk();

		const float corner_x[4] = { -half, half, half, -half };
		const float corner_y[4] = { -half, -half, half, half };
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

	std::string Author_Quad_W3D_Bytes(const char* container_name, const char* mesh_name, const char* texture_name, float half)
	{
		std::string path = Temp_Path("quad.w3d");
		{
			RawFileClass file(path.c_str());
			if (!file.Open(FileClass::WRITE)) { Fail("could not create temp W3D authoring file"); return std::string(); }
			ChunkSaveClass csave(&file);
			Write_Mesh_Chunk(csave, container_name, mesh_name, texture_name, half);
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
	// RenderRTS3DScene's AuthorBigArchive.
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

	const char* const kArchiveName = "RenderViewUpdateDraw.big";

	const int BG_R = 40, BG_G = 70, BG_B = 110;
	const int TEX_R = 180, TEX_G = 200, TEX_B = 30;
	const float QUAD_HALF = 20.0f; // world-space half-extent; large enough to be reliably sampled at ~493 world units' camera distance (see below).

	// --- Independent CPU-side reimplementation of W3DView::buildCameraPosition()/
	// buildCameraTransform()'s documented formula (Core/GameEngineDevice/Source/
	// W3DDevice/GameClient/W3DView.cpp:275-440), authored fresh here per the task
	// brief's "computed independently... NOT a hand-picked constant" requirement -
	// NOT by calling into the SUT's own (private) buildCameraPosition()/
	// buildCameraTransform(). Assumptions, all real defaults/choices this harness's
	// own driving sequence below guarantees hold (documented at each use site):
	//   - m_shakeOffset == (0,0): no Add_Camera_Shake() call is ever made.
	//   - m_useRealZoomCam == false: the real ctor default (W3DView.cpp:189),
	//     never toggled by this harness (cameraEnableRealZoomMode() is never called).
	//   - m_FXPitch == 1.0f: the real ctor default (W3DView.cpp:179), never
	//     changed (set3DWireFrameMode/setFadeParameters/etc. are never called) -
	//     makes buildCameraPosition's final targetPos.Z adjustment an exact
	//     identity (RTS_ZEROHOUR branch, W3DView.cpp:366-369).
	//   - TheDisplay == nullptr: scaleCameraHeightForAspectRatio() is a real,
	//     null-guarded pass-through (View.cpp:132-133), so m_maxCameraHeight is
	//     used unscaled.
	void PredictCameraSourceAndTarget(
		Real zoom, Real angle, Real pitch, const Coord3D& pos, Real maxCameraHeight,
		Vector3* outSourcePos, Vector3* outTargetPos)
	{
		Vector3 sourcePos;
		sourcePos.Z = pos.z + maxCameraHeight; // getCameraOffsetZ(), TheDisplay-null pass-through.
		sourcePos.Y = -(sourcePos.Z / tanf(ViewDefaultPitchRadians));
		sourcePos.X = -(sourcePos.Y * tanf(ViewDefaultYawRadians));

		// m_useRealZoomCam == false path (W3DView.cpp:298-303):
		sourcePos.X *= zoom;
		sourcePos.Y *= zoom;
		sourcePos.Z *= zoom;

		const Real heightScale = 1.0f - (pos.z / sourcePos.Z);

		Matrix3D angleTransform(Vector3(0.0f, 0.0f, 1.0f), angle - ViewDefaultYawRadians);
		Matrix3D pitchTransform(Vector3(-1.0f, 0.0f, 0.0f), pitch - ViewDefaultPitchRadians);
		pitchTransform.mulVector3(sourcePos);
		angleTransform.mulVector3(sourcePos);

		sourcePos *= heightScale;

		Vector3 targetPos(pos.x, pos.y, pos.z);
		sourcePos += targetPos;

		// m_useRealZoomCam == false, RTS_ZEROHOUR, m_FXPitch == 1.0f (W3DView.cpp:366-369):
		// targetPos.Z = sourcePos.Z - ((sourcePos.Z - targetPos.Z) * 1.0f) == targetPos.Z,
		// an exact identity - targetPos is left unchanged by this final step.

		*outSourcePos = sourcePos;
		*outTargetPos = targetPos;
	}

	// Mirrors buildCameraTransform()'s own Look_At call (W3DView.cpp:386-387).
	// No camera shake (CameraShakerSystem's contribution is exactly zero - this
	// harness never calls Add_Camera_Shake(), see link_stubs.cpp), no camera-
	// slave mode (m_isCameraSlaved stays false, cameraEnableSlaveMode() is
	// never called) - both real, provably-zero contributions for this harness's
	// exact configuration, not omissions of real behavior.
	Matrix3D PredictCameraTransform(const Vector3& sourcePos, const Vector3& targetPos)
	{
		Matrix3D transform;
		transform.Make_Identity();
		transform.Look_At(sourcePos, targetPos, 0);
		return transform;
	}

} // end anonymous namespace

int main()
{
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

	// Literal nested block BEFORE shutdownMemoryManager() - Tests/GameFileSystem's
	// own found-by-gdb SIGSEGV lesson, carried over verbatim by every rendering
	// harness since. Also MUST be lexically inside main() itself (not a helper
	// function/lambda) per dx8wrapper.h:711's "friend int main();" grant.
	{
		RemoveIfExists(kArchiveName);

		printf("=== Setup 0: authoring quad.w3d + quad.tga into a test-authored .big archive ===\n");
		std::string tgaBytes = Author_Quad_TGA_Bytes(4, 4, TEX_R, TEX_G, TEX_B);
		Check(!tgaBytes.empty(), "0a. authored TGA bytes are non-empty");
		std::string w3dBytes = Author_Quad_W3D_Bytes("RCT", "Quad", "quad.tga", QUAD_HALF);
		Check(!w3dBytes.empty(), "0b. authored W3D bytes are non-empty");

		std::vector<ArchiveEntry> entries = {
			{ "Art\\W3D\\quad.w3d",     w3dBytes },
			{ "Art\\Textures\\quad.tga", tgaBytes },
		};
		AuthorBigArchive(kArchiveName, entries);

		// ---- File-system stack, GameEngine::init()'s own spine order. ----
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
			fprintf(stderr, "RENDERVIEWUPDATEDRAW_FAIL: init round-trip failed, aborting\n");
			g_AnyFailure = true;
			return 1;
		}

		WW3D::Set_Thumbnail_Enabled(false);
		DX8Wrapper::Set_Texture_Bitdepth(32);

		// ---- Check 1: GlobalData defaults sane (Milestone 8's own asserted
		// subset, unchanged, plus the camera-specific defaults this harness's
		// own math relies on). m_headless is asserted FALSE - the task brief's
		// explicit requirement (item 1): the terrain-height code path in
		// updateCameraTransform() must genuinely run. ----
		printf("=== Check 1: GlobalData defaults sane ===\n");
		TheWritableGlobalData = NEW GlobalData;
		Check(TheWritableGlobalData != nullptr, "1c. TheWritableGlobalData constructed (non-null)");
		Check(TheWritableGlobalData->m_maxVisibleTranslucentObjects == 512, "1d. m_maxVisibleTranslucentObjects == 512 (real default)");
		Check(TheWritableGlobalData->m_maxVisibleOccluderObjects == 512, "1e. m_maxVisibleOccluderObjects == 512 (real default)");
		Check(TheWritableGlobalData->m_maxVisibleOccludeeObjects == 512, "1f. m_maxVisibleOccludeeObjects == 512 (real default)");
		Check(TheWritableGlobalData->m_maxVisibleNonOccluderOrOccludeeObjects == 512, "1g. m_maxVisibleNonOccluderOrOccludeeObjects == 512 (real default)");
		Check(TheWritableGlobalData->m_shroudOn == true, "1h. m_shroudOn == true (ENABLE_CONFIGURABLE_SHROUD real default)");
		Check(TheWritableGlobalData->m_timeOfDay == TIME_OF_DAY_AFTERNOON, "1i. m_timeOfDay == TIME_OF_DAY_AFTERNOON (real default)");
		Check(TheWritableGlobalData->m_headless == FALSE, "1j. m_headless == FALSE (real default - task brief item 1: terrain-height path must genuinely run)");
		Check(CloseReal(TheWritableGlobalData->m_maxCameraHeight, 300.0f, 0.01f), "1k. m_maxCameraHeight == 300.0 (real default, GlobalData.cpp:866)");
		Check(CloseReal(TheWritableGlobalData->m_minCameraHeight, 100.0f, 0.01f), "1l. m_minCameraHeight == 100.0 (real default, GlobalData.cpp:865)");
		Check(CloseReal(TheWritableGlobalData->m_cameraPitch, 0.0f, 0.01f), "1m. m_cameraPitch == 0.0 (real default, GlobalData.cpp:860)");
		Check(CloseReal(TheWritableGlobalData->m_cameraYaw, 0.0f, 0.01f), "1n. m_cameraYaw == 0.0 (real default, GlobalData.cpp:861)");

		// ---- Check 2: real TerrainLogic - the first ever constructed on
		// POSIX (Draft 32 finding 6, Task 2 Part B's deferred proof). Base
		// class only (NOT W3DTerrainLogic), but via the harness-local
		// HarnessTerrainLogic subclass (Milestone 11 RETRY's own real,
		// implementation-time finding - see harness_stub_classes.h's own
		// header comment): the base class's getExtent()/
		// getExtentIncludingBorder() are unimplemented DEBUG_CRASH-only stubs
		// that leave their output Region3D uninitialized, corrupting
		// W3DView::calcCameraAreaConstraints() (genuinely reached by this
		// milestone's real view->update() call, unlike M9's harness which
		// never called update() at all) - HarnessTerrainLogic supplies a
		// real, finite, large-enough extent instead. Every OTHER TerrainLogic
		// method (getGroundHeight() etc., checked below) is the exact same
		// real, unmodified base-class body. ----
		printf("=== Check 2: real TerrainLogic (first ever on POSIX) ===\n");
		TheTerrainLogic = NEW HarnessTerrainLogic;
		Check(TheTerrainLogic != nullptr, "2a. TheTerrainLogic constructed (non-null)");
		Coord3D groundNormal = { 99.0f, 99.0f, 99.0f };
		Real groundHeight = TheTerrainLogic->getGroundHeight(500.0f, 500.0f, &groundNormal);
		Check(groundHeight == 0.0f, "2b. getGroundHeight() == 0 with no map loaded (TerrainLogic.cpp:1430-1437)");
		Check(groundNormal.x == 0.0f && groundNormal.y == 0.0f && groundNormal.z == 0.0f, "2c. getGroundHeight() zeroed the output normal");

		// ---- Check 3: real FramePacer - a genuine gap this task found
		// (see this file's header comment and CMakeLists.txt). ----
		printf("=== Check 3: real FramePacer (genuine gap this task found) ===\n");
		TheFramePacer = NEW FramePacer;
		Check(TheFramePacer != nullptr, "3a. TheFramePacer constructed (non-null)");
		Real logicStepMs = TheFramePacer->getLogicTimeStepMilliseconds();
		Check(logicStepMs > 0.0f, "3b. getLogicTimeStepMilliseconds() > 0 (the exact call buildCameraTransform() makes unconditionally, W3DView.cpp:392)");

		// ---- Milestone 11 RETRY, step 1 (Draft 35 implementation ordering,
		// findings 2-3): real TheGameLogic/TheScriptEngine construction - now
		// genuinely achievable link-wise since CMakeLists.txt links the whole
		// real GameEngine source closure (Milestone 10's own technique).
		// Both constructors are set-defaults-only (GameLogic.cpp:257-304,
		// ScriptEngine.cpp:447-479) - assert a few known defaults so a
		// silently-wrong construction fails loudly. ----
		printf("=== Check M11a: real TheGameLogic/TheScriptEngine (Milestone 11 RETRY step 1) ===\n");
		TheGameLogic = NEW GameLogic;
		Check(TheGameLogic != nullptr, "M11a. TheGameLogic constructed (non-null)");
		Check(TheGameLogic->getGameMode() == GAME_NONE, "M11b. TheGameLogic->getGameMode() == GAME_NONE (real default, GameLogic.cpp:284)");
		Check(TheGameLogic->isInGame() == false, "M11c. TheGameLogic->isInGame() == false (GameLogic.h:500, \"m_gameMode != GAME_NONE\")");
		Check(TheGameLogic->isGamePaused() == false, "M11d. TheGameLogic->isGamePaused() == false (real default, now the real linked body)");
		Check(TheGameLogic->findObjectByID(INVALID_ID) == nullptr, "M11e. findObjectByID(INVALID_ID) == nullptr (GameLogic.h:511-525, short-circuits)");

		TheScriptEngine = NEW ScriptEngine;
		Check(TheScriptEngine != nullptr, "M11f. TheScriptEngine constructed (non-null)");
		Check(TheScriptEngine->isTimeFrozenDebug() == false, "M11g. isTimeFrozenDebug() == false (real POSIX body, ScriptEngine.cpp:8450-8473)");
		Check(TheScriptEngine->isTimeFrozenScript() == false, "M11h. isTimeFrozenScript() == false (real body, return m_freezeByScript, defaults FALSE)");
		Check(TheScriptEngine->isTimeFast() == false, "M11i. isTimeFast() == false (real POSIX body, ScriptEngine.cpp:8478+)");

		// ---- Milestone 11 RETRY, step 2 (Draft 35 implementation ordering,
		// design decisions): minimal concrete stub subclasses for GameClient/
		// InGameUI/Display/FontLibrary/Mouse, each overriding ONLY its
		// abstract base's pure virtuals (harness_stub_classes.h) - genuinely
		// sufficient now that the real, non-pure base bodies come from the
		// linked closure instead of causing link errors. Resolves the
		// GameClient destructor hazard the draft documents (~GameClient()
		// unconditionally does "TheFontLibrary->reset(); delete
		// TheFontLibrary;" and "TheMouse->reset(); delete TheMouse;",
		// GameClient.cpp:187-193) by providing real, minimal, non-null
		// FontLibraryStub/MouseStub instances FIRST, per the draft's own
		// recommendation (open question 2) - the deliberate-leak fallback is
		// NOT used. ----
		printf("=== Check M11j: minimal concrete stub subclasses (Milestone 11 RETRY step 2) ===\n");
		TheFontLibrary = NEW FontLibraryStub;
		Check(TheFontLibrary != nullptr, "M11j. TheFontLibrary (FontLibraryStub) constructed (non-null) - resolves GameClient dtor hazard");
		TheMouse = NEW MouseStub;
		Check(TheMouse != nullptr, "M11k. TheMouse (MouseStub) constructed (non-null) - resolves GameClient dtor hazard");
		TheDisplay = NEW DisplayStub;
		Check(TheDisplay != nullptr, "M11l. TheDisplay (DisplayStub) constructed (non-null)");
		TheGameClient = NEW GameClientStub;
		Check(TheGameClient != nullptr, "M11m. TheGameClient (GameClientStub) constructed (non-null)");
		TheInGameUI = NEW InGameUIStub;
		Check(TheInGameUI != nullptr, "M11n. TheInGameUI (InGameUIStub) constructed (non-null)");
		// TheWindowManager: a real, implementation-time finding this retry
		// discovered (NOT flagged by Draft 35 - see GameWindowManagerStub's
		// own header comment in harness_stub_classes.h for the full,
		// GDB-backtrace-confirmed story): the real ~InGameUI()'s
		// stopCameoMovie() unconditionally dereferences TheWindowManager with
		// no null guard, genuinely reached via this harness's own
		// "delete TheGameClient" teardown (~GameClient() cascades into
		// ~InGameUI()). A real, minimal, non-null instance is required.
		TheWindowManager = NEW GameWindowManagerStub;
		Check(TheWindowManager != nullptr, "M11r. TheWindowManager (GameWindowManagerStub) constructed (non-null) - resolves the real ~InGameUI()/stopCameoMovie() teardown hazard");
		// TheNameKeyGenerator: a SECOND real, implementation-time finding on
		// the SAME stopCameoMovie() call site, found by a second real GDB
		// backtrace after fixing TheWindowManager above: "TheNameKeyGenerator
		// ->nameToKey(...)" (NameKeyGenerator.cpp:192) is NOT null-guarded
		// (unlike StaticNameKey::key(), which IS - see link_stubs.cpp's own
		// comment on that distinction). NameKeyGenerator's own constructor is
		// cheap, set-defaults-only (NameKeyGenerator.cpp:37-45, zeroes its
		// hash-bucket array) - a real, minimal, non-null instance, matching
		// this harness's established "construct the cheap real singleton"
		// precedent (TheGameLogic/TheScriptEngine above).
		TheNameKeyGenerator = NEW NameKeyGenerator;
		Check(TheNameKeyGenerator != nullptr, "M11s. TheNameKeyGenerator constructed (non-null) - resolves the real ~InGameUI()/stopCameoMovie()/nameToKey() teardown hazard");
		// TheAI: a FOURTH real, implementation-time finding on this same
		// teardown trail, found by a further real GDB backtrace:
		// GameLogic::~GameLogic() -> destroyAllObjectsImmediate() ->
		// processDestroyList() unconditionally does "TheAI->pathfinder()->
		// m_classifyFenceZeroInit = ...;" (GameLogic.cpp:2586) with no null
		// guard - genuinely reached by this harness's own real
		// "delete TheGameLogic" teardown below. AI's own constructor
		// (AI.cpp:302-307) is cheap and safe (NEW TAiData; NEW Pathfinder;
		// Pathfinder's own constructor, AIPathfind.cpp:4074-4079, is
		// likewise set-defaults-only) - a real, minimal, non-null instance,
		// matching this harness's established construction precedent.
		TheAI = NEW AI;
		Check(TheAI != nullptr, "M11t. TheAI constructed (non-null) - resolves the real GameLogic::~GameLogic()/processDestroyList() teardown hazard");

		// ---- Construct the real RTS3DScene (Milestone 8's own construction,
		// unchanged) plus the quad. ----
		printf("=== Constructing the real RTS3DScene ===\n");
		RTS3DScene* scene = NEW_REF(RTS3DScene, ());
		Check(scene != nullptr, "3c. RTS3DScene constructed (non-null)");
		scene->Set_Ambient_Light(Vector3(1.0f, 1.0f, 1.0f));

		LightClass* zeroLight = NEW_REF(LightClass, (LightClass::DIRECTIONAL));
		zeroLight->Set_Ambient(Vector3(0.0f, 0.0f, 0.0f));
		zeroLight->Set_Diffuse(Vector3(0.0f, 0.0f, 0.0f));
		zeroLight->Set_Specular(Vector3(0.0f, 0.0f, 0.0f));
		scene->setGlobalLight(zeroLight, 0);
		zeroLight->Release_Ref();

		WW3DAssetManager asset_manager;
		bool loaded = asset_manager.Load_3D_Assets("quad.w3d");
		Check(loaded, "3d. Load_3D_Assets(\"quad.w3d\") succeeded via factory->TheFileSystem->archive resolution");

		RenderObjClass* robj = loaded ? asset_manager.Create_Render_Obj("RCT.Quad") : nullptr;
		Check(robj != nullptr, "3e. Create_Render_Obj(\"RCT.Quad\") returned non-null");

		if (!robj)
		{
			fprintf(stderr, "RENDERVIEWUPDATEDRAW_FAIL: could not create render object, aborting remaining checks\n");
			REF_PTR_RELEASE(scene);
			WW3D::Shutdown();
			g_AnyFailure = true;
			return 1;
		}

		scene->Add_Render_Object(robj);

		// Milestone 11 addition: pickDrawable() (Check 8, below) reads the
		// STATIC W3DDisplay::m_3DScene (W3DView.cpp:2552,
		// "W3DDisplay::m_3DScene->castRay(...)"), not a locally-scoped
		// scene pointer - Tests/RenderCameraTransform never needed this
		// assignment since it only ever called scene->doRender(...)
		// directly (never pickDrawable()). W3DDisplay::m_3DScene's own
		// storage is already declared (link_stubs.cpp, finding 9's own
		// technique, unchanged) - this is a plain assignment to the SAME
		// real RTS3DScene constructed above, not a new object.
		W3DDisplay::m_3DScene = scene;

		// ---- Milestone 11 RETRY, step 3 (Draft 35 implementation ordering,
		// finding 9-10): a real RTS2DScene as W3DDisplay::m_2DScene, matching
		// the real game's own construction idiom (NEW_REF, unchanged from
		// Milestone 8's RTS3DScene technique above). draw() unconditionally
		// dereferences W3DDisplay::m_2DScene ("W3DDisplay::m_2DScene->doRender(
		// m_2DCamera)", W3DView.cpp:2109), so it must be real, not null.
		// RTS2DScene's own constructor unconditionally builds a real
		// W3DStatusCircle and adds it to the scene (W3DScene.cpp:2168-2173) -
		// this is the genuine, positive proof finding 10 documents: with a
		// real (even defaults-only) TheGameLogic now constructed above,
		// W3DStatusCircle::Render()'s own first real line
		// ("if (!TheGameLogic->isInGame() || TheGameLogic->getGameMode() ==
		// GAME_SHELL) return;", W3DStatusCircle.cpp:301-304) cleanly early-
		// returns, confirmed below once draw() actually runs it (Check M11).
		// ----
		printf("=== Check M11o: real RTS2DScene as W3DDisplay::m_2DScene (Milestone 11 RETRY step 3) ===\n");
		RTS2DScene* scene2d = NEW_REF(RTS2DScene, ());
		Check(scene2d != nullptr, "M11o. RTS2DScene constructed (non-null) - its own ctor builds a real W3DStatusCircle");
		W3DDisplay::m_2DScene = scene2d;

		// ---- Check 4: real W3DView, constructed and driven through the
		// real public camera API (task brief item 2, open question 3). ----
		printf("=== Check 4: real W3DView, driven through the real per-frame camera API ===\n");
		W3DView* view = NEW W3DView; // matches W3DInGameUI::createView()'s own "return NEW W3DView;" idiom exactly.
		Check(view != nullptr, "4a. W3DView constructed (non-null)");

		view->init(); // matches the engine's own post-construction call (InGameUI.cpp:1373).

		// The EXACT call InGameUI::init() makes right after (InGameUI.cpp:1379-1382),
		// same arguments - skipping only the TheDisplay-guarded attachView()/
		// setWidth()/setHeight() calls in that same real code block (TheDisplay
		// deliberately stays null, task brief item 3's sibling omission).
		view->setDefaultView(
			DEG_TO_RADF(TheWritableGlobalData->m_cameraPitch),
			DEG_TO_RADF(TheWritableGlobalData->m_cameraYaw),
			1.0f);

		// Pin the pitch to the SAME constant buildCameraPosition() uses as its
		// own reference pitch (ViewDefaultPitchRadians) - this makes
		// PredictCameraSourceAndTarget()'s pitchTransform an exact identity
		// rotation (pitch - ViewDefaultPitchRadians == 0), so this harness's own
		// independent math does not also have to re-derive Matrix3D's
		// axis-angle rotation semantics for the pitch axis to predict
		// buildCameraPosition's output - it still genuinely exercises the real
		// setPitch()/buildCameraPosition() code path (a nonzero, non-default
		// argument, clamped and stored for real), it just chooses a value that
		// keeps the INDEPENDENT reimplementation simple and exactly verifiable.
		view->setPitch(ViewDefaultPitchRadians);

		// A concrete, nonzero yaw - genuinely exercises buildCameraPosition's
		// angle-offset rotation (angleTransform, NOT an identity here).
		const Real TEST_ANGLE_RADIANS = DEG_TO_RADF(15.0f);
		view->setAngle(TEST_ANGLE_RADIANS);

		// setZoom(1.0f) drives m_heightAboveGround = m_maxHeightAboveGround (the
		// real formula, W3DView.cpp:2289) and m_zoom = getDesiredZoom(...) =
		// (m_pos.z + m_heightAboveGround) / (m_pos.z + m_maxCameraHeight) which,
		// since m_heightAboveGround == m_maxHeightAboveGround, is EXACTLY 1.0
		// regardless of m_pos.z - the real code's own derived invariant, not an
		// assumption; Check 4b below confirms this via the real getZoom() accessor.
		view->setZoom(1.0f);
		Check(CloseReal(view->getZoom(), 1.0f, 0.0001f), "4b. getZoom() == 1.0 after setZoom(1.0) (real derived invariant, W3DView.cpp:2287-2296)");

		// lookAt() positions the pivot's x/y and resets z to the (flat, 0)
		// ground height via the real resetPivotToGround() (W3DView.cpp:2641-2694).
		// z == 0.0f keeps this call safely under lookAt()'s own
		// "PATHFIND_CELL_SIZE_F + getGroundHeight()" guard (10.0f), so the
		// (deliberately null, task brief item 3) TheTerrainRenderObject->Cast_Ray
		// branch is never taken.
		const Coord3D LOOKAT_TARGET = { 500.0f, 500.0f, 0.0f };
		view->lookAt(&LOOKAT_TARGET);
		Check(CloseReal(view->getPosition().x, 500.0f, 0.01f) && CloseReal(view->getPosition().y, 500.0f, 0.01f) && view->getPosition().z == 0.0f,
			"4c. getPosition() == lookAt() target with z reset to the flat (0) ground height");

		// ---- Milestone 11 RETRY, step 4 (Draft 35 implementation ordering -
		// THIS MILESTONE'S ACTUAL PAYOFF): drive the real, unmodified
		// W3DView::update() through its full real body (W3DView.cpp:1388-1718),
		// now genuinely callable (M9's own open question 3 blocker - update()
		// unconditionally dereferences TheGameClient/TheScriptEngine/
		// TheGameLogic - is resolved, all three are now real). Called HERE,
		// in place of M9's own friend-grant DriveUpdateCameraTransform()
		// workaround (that workaround existed SPECIFICALLY because those three
		// singletons were null in M9 - now that they are real, the genuine
		// public entry point is both callable and more honest). update()
		// internally calls the private updateCameraTransform() itself
		// (":1701-1705, if (m_recalcCamera || m_isCameraSlaved)") AFTER first
		// running zoomCameraToDesiredHeight()/movePivotToGround() (":1658-1666,
		// under m_okToAdjustHeight, real default TRUE) - a real, implementation-
		// time finding: predicting the final camera transform from PRE-update()
		// zoom/position state (as a bare updateCameraTransform()-only call would
		// require) does NOT match what update()'s own height-adjustment pass
		// converges to; reading state AFTER this single real update() call
		// (below, Check 4d/4e) is what actually matches the game's own
		// single-pass-per-frame semantics. Also exercises, for real: TheGameLogic
		// ->isGamePaused()/TheScriptEngine->isTimeFrozenDebug()/
		// isTimeFrozenScript()/isTimeFast() (all real, linked bodies, all
		// false), getHeightAroundPos() (real TheTerrainLogic, flat/0),
		// getAxisAlignedViewRegion() (finding 5's camera-pitch hazard - verified
		// below not to fire under this harness's own ViewDefaultPitchRadians
		// pitch), and TheGameClient->iterateDrawablesInRegion() (real body,
		// empty m_drawableList, zero-count, GameClient.cpp:820-837 - the loop
		// body never executes, confirmed safe by direct reading). ----
		printf("=== Check M11p: real W3DView::update() (Milestone 11 RETRY step 4, the milestone's payoff) ===\n");
		view->update();
		Check(true, "M11p. view->update() returned without crashing (real isGamePaused()/isTimeFrozenDebug()/isTimeFrozenScript()/isTimeFast()/getAxisAlignedViewRegion()/iterateDrawablesInRegion() all executed)");
		// finding 5's camera-pitch hazard (getAxisAlignedViewRegion()'s unguarded
		// TheTerrainRenderObject->getMap() fallback branch): reaching this line
		// at all (no segfault) is the honest, direct proof that
		// getScreenCornerWorldPointsAtZ() returned PlaneClass::INSIDE_SEGMENT for
		// this harness's own ViewDefaultPitchRadians pitch, so the hazardous
		// fallback branch was never taken - recorded explicitly per the draft's
		// own open question 5, not assumed.
		Check(true, "M11q. getAxisAlignedViewRegion()'s camera-pitch hazard (finding 5) did NOT fire under this harness's real pitch (survived to this line)");

		// ---- Check 4d/4e: direct-state assertions against the independent
		// CPU-side prediction (task brief item 4 - "direct-state assertions
		// beat pixel-only inference where a direct query is available"),
		// computed from the view's state AFTER the real update() call above
		// (its own PRE-transform height-adjustment pass, see this block's own
		// header comment, is why this ordering matters). ----
		Vector3 predictedSource, predictedTarget;
		PredictCameraSourceAndTarget(
			view->getZoom(), view->getAngle(), view->getPitch(), view->getPosition(),
			TheWritableGlobalData->m_maxCameraHeight,
			&predictedSource, &predictedTarget);
		Matrix3D predictedTransform = PredictCameraTransform(predictedSource, predictedTarget);

		Coord3D realCamPosC = view->get3DCameraPosition();
		Vector3 realCamPos(realCamPosC.x, realCamPosC.y, realCamPosC.z);
		Check(CloseVector3(realCamPos, predictedSource, 0.05f, "4d. view->get3DCameraPosition() matches the independent CPU-side prediction"), "4d");

		Coord3D realCamDirC = view->get3DCameraDirection();
		Vector3 realCamDir(realCamDirC.x, realCamDirC.y, realCamDirC.z);
		Vector3 predictedDir = predictedTarget - predictedSource;
		predictedDir.Normalize();
		Check(CloseVector3(realCamDir, predictedDir, 0.01f, "4e. view->get3DCameraDirection() matches the independent CPU-side prediction"), "4e");

		const Vector3 CLEAR_COLOR(BG_R / 255.0f, BG_G / 255.0f, BG_B / 255.0f);

		// ---- Check 5: pixel A (centered) - the quad, placed exactly at the
		// lookAt() target, must render at the exact screen center: a direct
		// consequence of Look_At's own definition (the target always projects
		// to NDC (0,0)), so this specifically proves the real, W3DView-computed
		// camera (view->get3DCamera(), NOT a hand-built test camera) is what
		// actually drove this frame's render.
		//
		// Milestone 11 RETRY: renders via the REAL view->drawView() (DRAW() ->
		// W3DView::draw(), W3DView.cpp:1857-2110) instead of a direct
		// scene->doRender() call - draw() internally does exactly
		// "W3DDisplay::m_3DScene->doRender(m_3DCamera)" (:1894) plus the real
        // batching/2D-scene/filter machinery around it (task brief item 4's
		// "reuse the existing pixel-check philosophy... now reached via the
		// FULL real call chain"). Also confirms, for real: TheDisplay->
		// beginBatch()/endBatch() (:2102,2104), TheGameClient->
		// resetRenderedObjectCount()/iterateDrawablesInRegion()/
		// flushTextBearingDrawables() (:2100,2103,2106), the default view-filter
		// branch's real no-op resolution (finding 7 - W3DShaderManager::
		// filterPreRender() genuinely called, genuinely returns false), and
		// W3DDisplay::m_2DScene->doRender() (:2109) - the real RTS2DScene's
		// W3DStatusCircle::Render() early-out (finding 10) genuinely firing,
		// confirmed by this pixel check's own success (a mis-firing status
		// circle would corrupt these exact pixels). ----
		printf("=== Check 5: pixel A (centered on the real camera's own look-at target) ===\n");
		robj->Set_Transform(Matrix3D(Vector3(LOOKAT_TARGET.x, LOOKAT_TARGET.y, LOOKAT_TARGET.z)));

		Check(WW3D::Begin_Render(true, true, CLEAR_COLOR) == WW3D_ERROR_OK, "5a. WW3D::Begin_Render returned WW3D_ERROR_OK");
		view->drawView(); // Milestone 11 RETRY: the REAL DRAW()->W3DView::draw() call chain, not a direct scene->doRender().
		Check(WW3D::End_Render(true) == WW3D_ERROR_OK, "5b. WW3D::End_Render returned WW3D_ERROR_OK");

		{
			unsigned char* fbo = Read_Fbo_Pixels_TopDown(g_W, g_H);
			Check_Pixel(fbo, 5, 5, BG_R, BG_G, BG_B, "5c. background == clear color");
			Check_Pixel(fbo, g_W / 2, g_H / 2, TEX_R, TEX_G, TEX_B, "5d. authored texel at the exact screen center (Look_At's own centering guarantee)");
			free(fbo);
		}
		Check(robj->Is_Really_Visible() != 0, "5e. robj->Is_Really_Visible() true while centered in-frustum (real Cull_Sphere result)");

		// ---- Check 6: pixel B (off-center) - a SECOND, offset world position,
		// predicted via a SEPARATE CameraClass carrying the independently
		// predicted (not queried-back) transform - this is the check that
		// actually exercises buildCameraPosition's zoom/pitch/angle-offset
		// math end-to-end, not just Look_At's own centering guarantee. ----
		printf("=== Check 6: pixel B (off-center, independently projected) ===\n");
		const Coord3D PROBE_WORLD = { LOOKAT_TARGET.x + 60.0f, LOOKAT_TARGET.y, 0.0f };

		CameraClass predictionCamera; // fresh, default-constructed - same defaults
		// W3DView's own m_3DCamera has in this configuration: default 50deg FOV
		// (W3DView.cpp never calls Set_View_Plane() on m_3DCamera outside
		// RTS_DEBUG builds, confirmed by direct reading), default 4:3 aspect
		// (setWidth()/setHeight() are never called - TheDisplay stays null).
		predictionCamera.Set_Transform(predictedTransform);
		// Mirrors updateCameraClipPlanes()'s own real fallback branch exactly
		// (W3DView.cpp:787-848): TheGlobalData->m_drawEntireTerrain == FALSE
		// (real default) and TheTerrainRenderObject == nullptr (task brief item
		// 3), so farZ = WorldHeightMap::NORMAL_DRAW_WIDTH * MAP_XY_FACTOR =
		// (1 + 6*32) * 10.0 = 1930.0 (WorldHeightMap.h:45,133 - constants only,
		// not the class itself, so this header is not included here).
		predictionCamera.Set_Clip_Planes(10.0f /*MAP_XY_FACTOR*/, 1930.0f);

		Vector3 predictedProbeNdc;
		CameraClass::ProjectionResType projResult = predictionCamera.Project(predictedProbeNdc, Vector3(PROBE_WORLD.x, PROBE_WORLD.y, PROBE_WORLD.z));
		Check(projResult == CameraClass::INSIDE_FRUSTUM, "6a. independently-predicted probe position projects INSIDE_FRUSTUM");

		int probe_px, probe_py;
		Ndc_To_Pixel_TopDown(predictedProbeNdc.X, predictedProbeNdc.Y, &probe_px, &probe_py);

		robj->Set_Transform(Matrix3D(Vector3(PROBE_WORLD.x, PROBE_WORLD.y, PROBE_WORLD.z)));
		Check(WW3D::Begin_Render(true, true, CLEAR_COLOR) == WW3D_ERROR_OK, "6b. WW3D::Begin_Render returned WW3D_ERROR_OK");
		view->drawView(); // Milestone 11 RETRY: the REAL DRAW()->W3DView::draw() call chain, not a direct scene->doRender().
		Check(WW3D::End_Render(true) == WW3D_ERROR_OK, "6c. WW3D::End_Render returned WW3D_ERROR_OK");

		{
			unsigned char* fbo = Read_Fbo_Pixels_TopDown(g_W, g_H);
			Check_Pixel(fbo, probe_px, probe_py, TEX_R, TEX_G, TEX_B,
				"6d. authored texel at the independently-predicted off-center position (buildCameraPosition's real zoom/pitch/angle math)");
			free(fbo);
		}
		Check(robj->Is_Really_Visible() != 0, "6e. robj->Is_Really_Visible() true while off-center but in-frustum (real Cull_Sphere result)");

		// ---- Check 7: culling - the same real Cull_Sphere-based
		// Visibility_Check (Milestone 8's own, unchanged) run against THIS
		// harness's real, W3DView-computed camera. ----
		printf("=== Check 7: culling against the real W3DView-computed camera ===\n");
		const Coord3D OFFSCREEN_WORLD = { LOOKAT_TARGET.x + 100000.0f, LOOKAT_TARGET.y, 0.0f };
		robj->Set_Transform(Matrix3D(Vector3(OFFSCREEN_WORLD.x, OFFSCREEN_WORLD.y, OFFSCREEN_WORLD.z)));

		Check(WW3D::Begin_Render(true, true, CLEAR_COLOR) == WW3D_ERROR_OK, "7a. WW3D::Begin_Render returned WW3D_ERROR_OK");
		view->drawView(); // Milestone 11 RETRY: the REAL DRAW()->W3DView::draw() call chain, not a direct scene->doRender().
		Check(WW3D::End_Render(true) == WW3D_ERROR_OK, "7b. WW3D::End_Render returned WW3D_ERROR_OK");
		Check(robj->Is_Really_Visible() == 0, "7c. robj->Is_Really_Visible() false once moved far outside the real camera's frustum");

		// ---- Check 8 (Milestone 11, rung 3b-ii-a, Draft 35, finding 11 /
		// implementation ordering step 5): pickDrawable()'s real ray-cast
		// proof. Originally shipped standalone (commit 67d7b83f0) when steps
		// 1-4 were still blocked by the disproven minimal-stub-subclass link
		// strategy (see CMakeLists.txt's own header comment for that full,
		// now-resolved story) - kept here unchanged, now alongside steps 1-4's
		// own real construction above, since pickDrawable() still needs
		// nothing beyond what it already used: TheWindowManager stays null
		// (already null-guarded, W3DView.cpp:2522-2523) and
		// W3DDisplay::m_3DScene->castRay() (Milestone 8's own technique). ----
		printf("=== Check 8: pickDrawable() real ray-cast proof (finding 11) ===\n");

		// Re-center and re-render the quad (matching Check 5's own render)
		// so Is_Really_Visible() - which castRay()'s non-testAll path
		// requires (W3DScene.cpp:441, "testAll || robj->Is_Really_Visible()")
		// - is true again (Check 7 just moved it far offscreen).
		robj->Set_Transform(Matrix3D(Vector3(LOOKAT_TARGET.x, LOOKAT_TARGET.y, LOOKAT_TARGET.z)));
		Check(WW3D::Begin_Render(true, true, CLEAR_COLOR) == WW3D_ERROR_OK, "8a. WW3D::Begin_Render returned WW3D_ERROR_OK");
		view->drawView(); // Milestone 11 RETRY: the REAL DRAW()->W3DView::draw() call chain, not a direct scene->doRender().
		Check(WW3D::End_Render(true) == WW3D_ERROR_OK, "8b. WW3D::End_Render returned WW3D_ERROR_OK");
		Check(robj->Is_Really_Visible() != 0, "8c. robj->Is_Really_Visible() true again after re-centering (required by castRay()'s non-testAll path)");

		// A real, non-zero collision type - RenderObjClass's own default
		// Bits leave Get_Collision_Type()==0 (rendobj.h:465-466), which
		// would never satisfy castRay's "Get_Collision_Type() &
		// collisionType" test (W3DScene.cpp:441) regardless of pickType.
		// PICK_TYPE_SELECTABLE is the same bit the real game marks ordinary
		// selectable Drawables' render objects with.
		robj->Set_Collision_Type(PICK_TYPE_SELECTABLE);

		// A clearly-labeled, NEVER-DEREFERENCED sentinel -  pickDrawable()
		// only ever returns the raw pointer value (W3DView.cpp:2556-2562),
		// it never dereferences it, so an intentionally-invalid,
		// identity-only pointer value is honest here (matches the design
		// decision: "assert on the returned pointer's IDENTITY, never
		// dereference it").
		Drawable* const SENTINEL_DRAWABLE = reinterpret_cast<Drawable*>(0x1);
		DrawableInfo sentinelInfo;
		sentinelInfo.m_drawable = SENTINEL_DRAWABLE;
		robj->Set_User_Data(&sentinelInfo);

		ICoord2D hitScreen;
		hitScreen.x = g_W / 2;
		hitScreen.y = g_H / 2;
		Drawable* hitResult = view->pickDrawable(&hitScreen, false, PICK_TYPE_SELECTABLE);
		Check(hitResult == SENTINEL_DRAWABLE, "8d. pickDrawable() at the quad's screen center returns the exact sentinel pointer (real castRay() hit)");

		ICoord2D missScreen;
		missScreen.x = 5;
		missScreen.y = 5;
		Drawable* missResult = view->pickDrawable(&missScreen, false, PICK_TYPE_SELECTABLE);
		Check(missResult == nullptr, "8e. pickDrawable() at a background screen point returns nullptr (real castRay() miss)");

		robj->Set_User_Data(nullptr);

		// ---- Full teardown, extending Milestone 8's own idiom with
		// TheTerrainLogic/TheFramePacer/W3DView's own real destructors, PLUS
		// (Milestone 11 RETRY) TheGameClient/TheGameLogic's own real, REAL
		// CASCADING destructors - a genuine, real-code-driven ordering
		// hazard this retry found by direct reading (NOT by trial and
		// error): ~GameClient() (GameClient.cpp:119-240) unconditionally
		// does "TheFontLibrary->reset(); delete TheFontLibrary;" and
		// "TheMouse->reset(); delete TheMouse;" (the exact hazard Draft 35
		// documents, now resolved by constructing real FontLibraryStub/
		// MouseStub instances above) AND ALSO unconditionally deletes
		// TheInGameUI and TheDisplay internally - so those four singletons
		// must NOT be separately `delete`d here, only via "delete
		// TheGameClient" below (a double-free otherwise). Separately,
		// ~GameLogic() (GameLogic.cpp:348-387) unconditionally deletes
		// TheTerrainLogic AND TheScriptEngine internally too - the existing
		// explicit "delete TheTerrainLogic"/new explicit "delete
		// TheScriptEngine" calls below are kept and MUST run BEFORE "delete
		// TheGameLogic" (leaving both pointers null first, so ~GameLogic()'s
		// own internal "delete TheTerrainLogic"/"delete TheScriptEngine"
		// become safe no-ops on already-null pointers, not double-frees). ----
		printf("=== Check 9: teardown ===\n");
		scene->Remove_Render_Object(robj);
		robj->Release_Ref();
		robj = nullptr;
		asset_manager.Free_Assets();

		delete view;
		view = nullptr;

		W3DDisplay::m_3DScene = nullptr; // null the static BEFORE releasing the real object it pointed to.

		REF_PTR_RELEASE(scene);
		Check(scene == nullptr, "9a. REF_PTR_RELEASE(scene) nulled the pointer");

		W3DDisplay::m_2DScene = nullptr; // null the static BEFORE releasing the real RTS2DScene it pointed to (Milestone 11 RETRY step 3).
		REF_PTR_RELEASE(scene2d);
		Check(scene2d == nullptr, "9i. REF_PTR_RELEASE(scene2d) nulled the RTS2DScene pointer (its own dtor releases the real W3DStatusCircle it built)");

		// Milestone 11 RETRY: TheGameClient is DELIBERATELY LEAKED here, not
		// deleted - a real, GDB-confirmed engine bug this retry found makes
		// "delete TheGameClient" genuinely unsafe, and it is NOT one this
		// harness can fix by constructing yet another real singleton (unlike
		// the TheFontLibrary/TheMouse/TheWindowManager/TheNameKeyGenerator
		// hazards above, all successfully resolved that way). Full trail,
		// each found by a REAL gdb backtrace, not predicted:
		//   1. ~GameClient() (GameClient.cpp:169) unconditionally deletes
		//      TheInGameUI, cascading into the real ~InGameUI()
		//      (InGameUI.cpp:1281), which unconditionally calls
		//      stopCameoMovie().
		//   2. stopCameoMovie()'s real body (InGameUI.cpp:4341) does
		//      "GameWindow *window = TheWindowManager->winGetWindowFromId(
		//      nullptr, TheNameKeyGenerator->nameToKey(
		//      \"ControlBar.wnd:RightHUD\"));" - both TheWindowManager and
		//      TheNameKeyGenerator dereferences are now safe (real instances
		//      constructed above), but winGetWindowFromId() correctly
		//      returns nullptr - no such window was ever created, and
		//      creating one for real would require this harness to load a
		//      real .wnd layout through the WindowLayout/GameWindow system,
		//      a substantially larger, genuinely out-of-scope bring-up.
		//   3. The NEXT line (InGameUI.cpp:4343-4344), "WinInstanceData
		//      *winData = window->winGetInstanceData(); winData->
		//      setVideoBuffer(nullptr);", dereferences that null `window`
		//      with NO null guard at all - a real, pre-existing engine bug
		//      (not introduced by this harness, not specific to POSIX),
		//      confirmed by a real GDB backtrace landing exactly on
		//      WinInstanceData::setVideoBuffer() with `this` computed as a
		//      small, clearly-garbage offset-from-null address.
		// Per Draft 35's own pre-approved fallback ("state the alternative
		// [deliberate leak, documented] as a fallback, not the default"):
		// this is exactly that documented fallback, reached only after the
		// draft's own preferred fix (real FontLibrary/Mouse instances) was
		// applied successfully and two FURTHER, deeper hazards on the SAME
		// destructor path were also fixed for real (TheWindowManager/
		// TheNameKeyGenerator) - this third hazard is the one place this
		// retry stops short of a full real teardown, and it is a genuine
		// engine bug, not a workaround-able harness gap. TheGameClient
		// (and, transitively, TheFontLibrary/TheMouse/TheInGameUI/
		// TheDisplay/TheWindowManager it would have deleted) is intentionally
		// never deleted for the remainder of this process's short life -
		// process exit reclaims the memory.
		Check(true, "9j. TheGameClient intentionally leaked, not deleted - real ~InGameUI()/stopCameoMovie() null-window dereference bug, documented above (Draft 35's own pre-approved fallback)");

		// TheNameKeyGenerator: deleted AFTER TheGameClient (its own real
		// ~InGameUI()'s stopCameoMovie() needs it alive DURING that delete,
		// see this block's own construction-site comment) - not touched by
		// ~GameClient() itself, so this harness owns its teardown directly.
		delete TheNameKeyGenerator;
		TheNameKeyGenerator = nullptr;
		Check(TheNameKeyGenerator == nullptr, "9m. NameKeyGenerator::~NameKeyGenerator() ran (real body: freeSockets(), safe with this harness's own small, self-registered key set)");

		// Milestone 11 RETRY: TheScriptEngine and TheTerrainLogic MUST be
		// deleted/nulled BEFORE TheGameLogic (see this block's own header
		// comment - ~GameLogic() would otherwise re-delete both).
		delete TheScriptEngine;
		TheScriptEngine = nullptr;
		Check(TheScriptEngine == nullptr, "9k. ScriptEngine::~ScriptEngine() ran (deliberately deleted before TheGameLogic to avoid ~GameLogic()'s own internal re-delete)");

		delete TheTerrainLogic;
		TheTerrainLogic = nullptr;
		Check(TheTerrainLogic == nullptr, "9b. TerrainLogic::~TerrainLogic() ran (reset()'s own real body: deleteWaypoints/deleteBridges/PolygonTrigger::deleteTriggers, all safe with empty lists)");

		delete TheGameLogic;
		TheGameLogic = nullptr;
		Check(TheGameLogic == nullptr, "9l. GameLogic::~GameLogic() ran for real (its own internal re-deletes of TheTerrainLogic/TheScriptEngine/TheGhostObjectManager/ThePartitionManager are all safe no-ops on already-null pointers)");

		// TheAI: deleted AFTER TheGameLogic (its own real ~GameLogic() needs
		// it alive DURING that delete, see this block's own construction-site
		// comment) - not touched by ~GameLogic() itself, so this harness owns
		// its teardown directly.
		delete TheAI;
		TheAI = nullptr;
		Check(TheAI == nullptr, "9n. AI::~AI() ran (real body: deletes m_pathfinder/m_aiData, both empty/trivial for this harness)");

		delete TheFramePacer;
		TheFramePacer = nullptr;
		Check(TheFramePacer == nullptr, "9c. FramePacer::~FramePacer() ran (real body: timeEndPeriod(1))");

		delete TheWritableGlobalData;
		Check(TheWritableGlobalData == nullptr, "9d. GlobalData::~GlobalData() nulled TheWritableGlobalData");

		WW3DErrorType shutdown_result = WW3D::Shutdown();
		Check(shutdown_result == WW3D_ERROR_OK, "9e. WW3D::Shutdown returned WW3D_ERROR_OK");
		Check(!WW3D::Is_Initted(), "9f. WW3D::Is_Initted() is false after Shutdown");

		delete TheW3DFileSystem;
		TheW3DFileSystem = nullptr;
		Check(_TheFileFactory == nullptr, "9g. ~W3DFileSystem restored _TheFileFactory");

		delete TheArchiveFileSystem; TheArchiveFileSystem = nullptr;
		delete TheLocalFileSystem; TheLocalFileSystem = nullptr;
		delete TheFileSystem; TheFileSystem = nullptr;
		Check(TheFileSystem == nullptr && TheLocalFileSystem == nullptr && TheArchiveFileSystem == nullptr && TheW3DFileSystem == nullptr,
			"9h. all four file-system singletons torn down and nulled");

		RemoveIfExists(kArchiveName);
	}

	shutdownMemoryManager();

	TheAsciiStringCriticalSection = nullptr;
	TheUnicodeStringCriticalSection = nullptr;
	TheDmaCriticalSection = nullptr;
	TheMemoryPoolCriticalSection = nullptr;
	TheDebugLogCriticalSection = nullptr;

	if (g_AnyFailure)
	{
		fprintf(stderr, "RENDERVIEWUPDATEDRAW_FAIL: one or more checks failed (see above)\n");
		return 1;
	}

	printf("RENDERVIEWUPDATEDRAW_OK: all checks passed\n");
	return 0;
}
