// Phase 5(a) Milestone 13 (native port plan, Draft 37, "rung 3b-ii-b") -
// this milestone's own payoff harness: a REAL, NAMED Object+Drawable,
// constructed through the real ThingFactory::newObject() ->
// GameLogic::friend_createObject() -> Object::Object() ->
// GameLogic::sendObjectCreated() -> ThingFactory::newDrawable() ->
// GameClient::friend_createDrawable() -> Drawable::Drawable() chain,
// rendered through the real W3DModelDraw draw module and picked back
// through the real W3DView::pickDrawable()/
// TheGameClient->iterateDrawablesInRegion(). See docs/native-port-plan.md's
// Draft 37 section for the full design rationale (the chain-tracing that
// makes this milestone tractable, the three measured new costs, the
// mandatory step-0 spike discipline).
//
// Structure: Tests/PosixGameEngineHarness/main.cpp's own real
// GameEngine::init()/update()/execute() prologue (Milestone 12, unchanged in
// spirit - same PosixGameEngine, same pre-init() globals) PLUS Tests/
// RenderViewUpdateDraw/main.cpp's own WW3D/GL setup, asset-authoring, and
// real W3DView-driving technique (Milestone 11), PLUS this milestone's own
// new steps: the team bootstrap (Cost A), the real W3DAssetManager +
// Load_3D_Assets of a test-authored named-unit mesh, and the real
// TheThingFactory->newObject() call.
//
// Construction order (this file's own main()):
//   1. Prologue: five CriticalSection globals + initMemoryManager()
//      (unchanged from every prior harness in this port).
//   2. Setup 0: author a textured quad "M13.UNIT" (Tests/
//      RenderViewUpdateDraw's own Write_Mesh_Chunk/Write_Solid_TGA
//      authoring code, reused verbatim per Draft 37's own "reuse
//      RenderGameAssets/RenderW3DMesh's asset-authoring code" guidance -
//      this milestone reuses the SAME technique those two milestones
//      established, already proven working by Milestone 11's own use of it)
//      into a .big archive at the process's own cwd, discoverable by
//      TheArchiveFileSystem's own real init()-time recursive *.big scan
//      (Milestone 12's own StdBIGFileSystem, now genuinely needed for a
//      real on-disk asset instead of just this harness's own Data/INI
//      scaffold).
//   3. Pre-init() prologue (Milestone 12's own four globals: TheVersion,
//      TheGameText, TheWritableGlobalData with m_headless=TRUE (kept for
//      engine.init() - see the real W3DView-driving step below for why this
//      gets flipped briefly, AFTER construction, not here), TheFramePacer).
//   4. PosixGameEngine engine; TheGameEngine = &engine; engine.init() - the
//      SAME real, unmodified GameEngine::init() Milestone 12 already
//      proved, now ALSO real-constructing TheThingFactory's actual
//      "M13NamedUnit" ThingTemplate (parsed from this harness's own
//      Data/INI/Object.ini) and TheModuleFactory with W3DModelDraw
//      registered (this milestone's own ModuleFactoryStub).
//   5. Cost A - the team bootstrap: TheSidesList->validateSides();
//      ThePlayerList->newGame(); ThePartitionManager->init(); - all real
//      public API, no engine changes (Draft 37's own "~4 harness lines").
//   6. WW3D/GL setup (Milestone 11's own technique, unchanged): TheW3DFileSystem,
//      WW3D::Init()/Set_Render_Device() - genuinely safe to run AFTER
//      engine.init() here (unlike Milestone 11, which had no GameEngine::init()
//      to interleave with) since TheArchiveFileSystem is already real by this
//      point, and INI parsing itself never constructs a W3DModelDraw module
//      instance (only ITS OWN ModuleData - the C++ module object is built
//      later, per-Drawable, confirmed by reading ThingTemplate::parseModuleName()
//      and W3DModelDraw's own constructor).
//   7. A real RTS3DScene/RTS2DScene (Milestone 8/11's own construction) as
//      W3DDisplay::m_3DScene/m_2DScene (static storage, W3DDisplay.cpp
//      itself deliberately not linked - Milestone 11's own established
//      technique), plus a real W3DAssetManager (Draft 37's own "one hard
//      link requirement") as W3DDisplay::m_assetManager, with the authored
//      "m13unit.w3d" loaded via Load_3D_Assets - all of this MUST be wired
//      up BEFORE step 8, since W3DModelDraw::W3DModelDraw()'s own
//      constructor unconditionally calls
//      "W3DDisplay::m_assetManager->Create_Render_Obj(...)" followed by
//      "W3DDisplay::m_3DScene->Add_Render_Object(...)" (confirmed by
//      reading, not assumed).
//   8. THE MILESTONE'S ACTUAL PAYOFF CALL:
//      TheThingFactory->newObject(TheThingFactory->findTemplate("M13NamedUnit"),
//      ThePlayerList->getNeutralPlayer()->getDefaultTeam()) - constructs a
//      real Object, which (via its own real constructor's
//      "TheGameLogic->sendObjectCreated(this)" call) constructs and binds a
//      real Drawable, which (via its own real constructor's
//      TheModuleFactory->newModule() call) constructs a real W3DModelDraw,
//      which creates and adds the real RenderObjClass to the real scene -
//      the ENTIRE chain Draft 37's own chain-tracing predicted, now
//      exercised for real.
//   9. obj->setPosition(&pos) - Object::setPosition() (inherited
//      Thing::setPosition()) internally calls Object::reactToTransformChange(),
//      whose own real body unconditionally syncs
//      "m_drawable->setTransformMatrix(this->getTransformMatrix())" - the
//      real Object-to-Drawable position sync this milestone relies on
//      (confirmed by reading, not assumed).
//  10. A real W3DView (Milestone 11's own technique, unchanged), driven
//      through the same real per-frame camera API, centered on the unit's
//      own world position.
//  11. Checks: the drawable is in TheGameClient's list by name; a pixel
//      check on the rendered mesh; pickDrawable() returns the REAL drawable
//      (identity + getTemplate()->getName(), closing Milestone 11's own
//      scoped-down sentinel proof for real - the real render object's own
//      "Set_User_Data(draw->getDrawableInfo())" call, done automatically by
//      W3DModelDraw's own constructor, makes this possible with NO manual
//      sentinel hack, unlike Milestone 11); iterateDrawablesInRegion()'s
//      point-pick callback fires with the live entry.
//  12. No teardown: matches Milestone 12's own established "no teardown"
//      precedent (this milestone extends that same GameEngine::init() chain
//      to its logical next step) - process exit reclaims everything.
//
// ---- Milestone 14 addendum (native port plan, Draft 38, "closing rung 3,
// multiple real Drawables") - EXTENDS this same file/target in place rather
// than forking a new sibling harness: the combination stays small (one more
// authored mesh/texture pair, one more ThingTemplate, one more
// newObject()/setPosition() call, a wider camera framing both units, and a
// handful of additional Check calls against the SAME already-real
// TheGameClient/view/pickDrawable()/iterateDrawablesInRegion() surface this
// file already exercises) - forking a second ~840-line harness whose first
// 700 lines would be a verbatim copy of this one was judged higher-risk and
// lower-value than extending it directly. Adds a SECOND real, named
// Object+Drawable ("M14SecondUnit", Data/INI/Object.ini, at
// UNIT_B_WORLD_POS, far enough from M13NamedUnit's own UNIT_A_WORLD_POS
// that region-based queries cleanly distinguish them) alongside the first,
// and closes Draft 35 findings 12/13's own "only ever tested with one live
// entry" gap for real:
//   (a) iterateDrawablesInRegion()'s real multi-entry traversal - a region
//       containing both units' positions fires the harness-local callback
//       twice, not once (Check 10 below);
//   (b) pickDrawable() identity-distinguishing two real candidates - picking
//       at unit A's own screen position returns unit A specifically (by
//       pointer identity AND by getTemplate()->getName()), not unit B, and
//       vice versa (Check 9 below);
//   (c) a region query positioned to contain only ONE of the two units
//       correctly excludes the other - a real negative control in both
//       directions, not just "more than zero" (Check 10 below).
//
// TheSuperHackers @port Milestone 14 (native port plan, Draft 38): the
// direct-call question - does Milestone 13's manual "drawable->draw()"
// substitution still apply for N objects, or does W3DView::update()'s own
// internal "TheGameClient->iterateDrawablesInRegion(&axisAlignedRegion,
// drawDrawable, nullptr)" call, W3DView.cpp:1717, already cover every
// registered drawable on its own - is answered by a real, in-code
// diagnostic (Check 7i below), not by comment-only reasoning, and the real,
// measured answer is: **NO, the manual substitution is NOT needed for this
// milestone's own two-unit configuration.** A real run showed BOTH unit A's
// and unit B's render-object transforms were already correctly pushed by
// view->update() alone, with NEITHER drawable's own "draw()" called
// manually beforehand (Check 7i's own printed diagnostic:
// "unit A auto-pushed by view->update() alone = YES, unit B auto-pushed =
// YES"; Check 8's own subsequent real pixel checks against both units'
// authored colors, at their own real projected screen positions, both
// passed on this exact render). This is consistent with reading
// GameClient::iterateDrawablesInRegion() (GameClient.cpp:820-837) as a
// plain, unconditional "for (draw = m_drawableList; draw; draw =
// nextDrawable)" walk with no early break, called by W3DView::update()
// itself (W3DView.cpp:1717) with a FRESH getAxisAlignedViewRegion() every
// call - so every real drawable inside the current view region gets a real
// "draw->draw()" once per update(), automatically, for as many drawables as
// are actually in view, once the camera's own transform is current (which
// it is here, since m_headless is flipped to FALSE and lookAt() sets
// m_recalcCamera=true before this same update() call runs). This differs
// from Milestone 13's own single-unit finding (see the original header
// comment above, step 11) - the two are not in conflict, each is a real,
// correctly-measured result for a DIFFERENT harness configuration, and
// Draft 38 explicitly warned not to assume one carries over to the other.
// Milestone 13's own single-unit harness was never re-tested to find out
// WHY its manual call was needed there; this milestone's own two-unit
// configuration simply never needed it. The code below still calls
// "drawableA->draw()"/"drawableB->draw()" conditionally, ONLY if the
// diagnostic ever finds a given unit was NOT auto-pushed - a harmless,
// evidence-gated safety net (never actually taken in this milestone's own
// real runs), not a load-bearing requirement.
#include "PreRTS.h"

#include "Common/AsciiString.h"
#include "Common/CriticalSection.h"
#include "Common/file.h"
#include "Common/FileSystem.h"
#include "Common/GameMemory.h"
#include "Common/NameKeyGenerator.h"
#include "Common/GlobalData.h"
#include "Common/GameEngine.h"
#include "Common/MessageStream.h"
#include "Common/FramePacer.h"
#include "Common/version.h"
#include "Common/PlayerList.h"
#include "Common/Player.h"
#include "Common/Team.h"
#include "GameLogic/SidesList.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameClient/GameText.h"
#include "GameClient/Drawable.h"
#include "GameClient/View.h"
#include "W3DDevice/GameClient/W3DFileSystem.h"
#include "W3DDevice/GameClient/W3DScene.h"
#include "W3DDevice/GameClient/W3DView.h"
#include "W3DDevice/GameClient/W3DDisplay.h"
#include "W3DDevice/GameClient/W3DAssetManager.h"
#include "harness_stub_classes.h"

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
#include <algorithm>
#include <filesystem>
#include <unistd.h>

namespace fs = std::filesystem;

namespace
{
	bool g_AnyFailure = false;

	void Check(bool ok, const char* what)
	{
		if (ok) {
			printf("  %s: OK\n", what);
		} else {
			fprintf(stderr, "  %s: FAILED\n", what);
			g_AnyFailure = true;
		}
	}

	void Fail(const char* what)
	{
		fprintf(stderr, "RENDERNAMEDDRAWABLE_FAIL: %s\n", what);
		g_AnyFailure = true;
	}

	int g_W = 640;
	int g_H = 480;

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

	bool Close(unsigned char actual, int expected, int tolerance = 2)
	{
		return std::abs(static_cast<int>(actual) - expected) <= tolerance;
	}

	bool CloseReal(Real actual, Real expected, Real tolerance)
	{
		return std::fabs(actual - expected) <= tolerance;
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

	// --- Asset authoring: real TGA/.w3d chunk bytes, built at runtime.
	// Byte-for-byte the same mechanics as Tests/RenderViewUpdateDraw/
	// main.cpp's own helpers (itself reused from Tests/RenderRTS3DScene),
	// reused per Draft 37's own "reuse RenderGameAssets/RenderW3DMesh's
	// asset-authoring code" guidance - a plain, single-mesh textured quad is
	// sufficient for this milestone's own explicit non-goals (no animation/
	// LOD content), so the simpler mesh-only technique (not the HLOD
	// technique Tests/RenderW3DMesh also demonstrates) is reused here. -----

	std::string Temp_Path(const char* leaf)
	{
		const char* tmp = getenv("TMPDIR");
		if (!tmp) tmp = "/tmp";
		char buf[512];
		snprintf(buf, sizeof(buf), "%s/rnd_%d_%s", tmp, static_cast<int>(getpid()), leaf);
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

	std::string Author_Unit_W3D_Bytes(const char* container_name, const char* mesh_name, const char* texture_name, float half)
	{
		std::string path = Temp_Path("unit.w3d");
		{
			RawFileClass file(path.c_str());
			if (!file.Open(FileClass::WRITE)) { Fail("could not create temp W3D authoring file"); return std::string(); }
			ChunkSaveClass csave(&file);
			Write_Mesh_Chunk(csave, container_name, mesh_name, texture_name, half);
			file.Close();
		}
		return SlurpAndDelete(path);
	}

	std::string Author_Unit_TGA_Bytes(int width, int height, unsigned char r, unsigned char g, unsigned char b)
	{
		std::string path = Temp_Path("unit.tga");
		Write_Solid_TGA(path, width, height, r, g, b);
		return SlurpAndDelete(path);
	}

	// --- .big archive authoring: byte-for-byte the same format as Tests/
	// RenderViewUpdateDraw's own AuthorBigArchive. -----
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

	const char* const kArchiveName = "RenderNamedDrawable.big";

	const int BG_R = 40, BG_G = 70, BG_B = 110;
	const int TEX_R = 200, TEX_G = 60, TEX_B = 90;
	const float UNIT_HALF = 20.0f;

	// TheSuperHackers @port Milestone 14 (native port plan, Draft 38): the
	// original Milestone 13 unit ("M13NamedUnit") is now UNIT A; Milestone
	// 14 adds a second, distinct-templated UNIT B ("M14SecondUnit",
	// TEX2_R/G/B, at UNIT_B_WORLD_POS) - far enough from UNIT A (200 world
	// units apart on each axis, ~283 units of straight-line separation) that
	// a region query with a modest (50-unit) half-width around either one
	// cleanly excludes the other (Check 10), while both stay close enough to
	// the shared HarnessTerrainLogic-constrained 0..1000 extent's own center
	// to both remain visible in a single, moderately-zoomed-out camera frame
	// (Check 7, confirmed via the real view->worldToScreen() below rather
	// than assumed from FOV math).
	const Coord3D UNIT_A_WORLD_POS = { 400.0f, 400.0f, 0.0f };
	const Coord3D UNIT_B_WORLD_POS = { 600.0f, 600.0f, 0.0f };
	const int TEX2_R = 60, TEX2_G = 200, TEX2_B = 90;

	// ---- Check 10 (iterateDrawablesInRegion): a harness-local callback,
	// counting hits and recording every Drawable* seen (not just the last -
	// Milestone 14's own extension, needed to confirm BOTH real entries are
	// visited by a single region query, not just that the count is right) -
	// Draft 35 finding 12's own multi-entry-traversal proof, closed for real
	// this milestone. ----
	struct IterateResult
	{
		int count = 0;
		Drawable* lastHit = nullptr;
		std::vector<Drawable*> hits;
	};

	void IterateCallback(Drawable* draw, void* userData)
	{
		IterateResult* result = static_cast<IterateResult*>(userData);
		result->count++;
		result->lastHit = draw;
		result->hits.push_back(draw);
	}

} // end anonymous namespace

int main()
{
	setvbuf(stdout, nullptr, _IOLBF, 0);
	setvbuf(stderr, nullptr, _IOLBF, 0);

	static CriticalSection critSec1, critSec2, critSec3, critSec4, critSec5;
	TheAsciiStringCriticalSection = &critSec1;
	TheUnicodeStringCriticalSection = &critSec2;
	TheDmaCriticalSection = &critSec3;
	TheMemoryPoolCriticalSection = &critSec4;
	TheDebugLogCriticalSection = &critSec5;

	initMemoryManager();

	// Literal nested block BEFORE shutdownMemoryManager() - every prior
	// rendering harness's own established "found-by-gdb SIGSEGV" lesson.
	// Also MUST be lexically inside main() itself (not a helper function/
	// lambda) per dx8wrapper.h's own "friend int main();" grant.
	{
		RemoveIfExists(kArchiveName);

		printf("=== Setup 0: authoring m13unit.w3d/.tga (unit A) + m14unit.w3d/.tga (unit B) into a test-authored .big archive ===\n");
		std::string tgaBytes = Author_Unit_TGA_Bytes(4, 4, TEX_R, TEX_G, TEX_B);
		Check(!tgaBytes.empty(), "0a. authored TGA bytes are non-empty (unit A)");
		std::string w3dBytes = Author_Unit_W3D_Bytes("M13", "UNIT", "m13unit.tga", UNIT_HALF);
		Check(!w3dBytes.empty(), "0b. authored W3D bytes are non-empty (unit A)");

		// TheSuperHackers @port Milestone 14 (native port plan, Draft 38):
		// unit B's own authored mesh/texture pair - a distinct container
		// name ("M14", matching Data/INI/Object.ini's own "Model = M14.UNIT"
		// for the new "M14SecondUnit" template) and a distinct solid color
		// (TEX2_R/G/B) so Check 8/9's pixel/pick checks can tell the two
		// real render objects apart on sight, not just by pointer identity.
		std::string tgaBytes2 = Author_Unit_TGA_Bytes(4, 4, TEX2_R, TEX2_G, TEX2_B);
		Check(!tgaBytes2.empty(), "0c. authored TGA bytes are non-empty (unit B)");
		std::string w3dBytes2 = Author_Unit_W3D_Bytes("M14", "UNIT", "m14unit.tga", UNIT_HALF);
		Check(!w3dBytes2.empty(), "0d. authored W3D bytes are non-empty (unit B)");

		std::vector<ArchiveEntry> entries = {
			{ "Art\\W3D\\m13unit.w3d",     w3dBytes },
			{ "Art\\Textures\\m13unit.tga", tgaBytes },
			{ "Art\\W3D\\m14unit.w3d",     w3dBytes2 },
			{ "Art\\Textures\\m14unit.tga", tgaBytes2 },
		};
		AuthorBigArchive(kArchiveName, entries);

		// ---- Pre-init() prologue (Milestone 12's own four globals). ----
		printf("=== Setup 1: pre-init() prologue (Milestone 12's own enumerated list) ===\n");
		TheVersion = NEW Version;
		Check(TheVersion != nullptr, "1a. TheVersion constructed (non-null)");

		TheGameText = CreateGameTextInterface();
		Check(TheGameText != nullptr, "1b. TheGameText constructed (non-null, un-init()'d)");

		TheWritableGlobalData = NEW GlobalData;
		Check(TheWritableGlobalData != nullptr, "1c. TheWritableGlobalData constructed (non-null)");
		TheWritableGlobalData->m_headless = TRUE;
		Check(TheWritableGlobalData->m_headless == TRUE, "1d. m_headless set TRUE for engine.init() (Milestone 12's own established choice, kept for construction - see this file's own header comment for why it is later flipped, briefly, for the camera update only)");

		TheFramePacer = NEW FramePacer;
		Check(TheFramePacer != nullptr, "1e. TheFramePacer constructed (non-null)");

		// ---- Check 1: the real, unmodified GameEngine::init() - Milestone
		// 12's own payoff call, now ALSO real-constructing this milestone's
		// own "M13NamedUnit" ThingTemplate and W3DModelDraw-registered
		// TheModuleFactory. ----
		printf("=== Check 1: real GameEngine::init() (Milestone 12's own payoff call) ===\n");
		PosixGameEngine engine;
		TheGameEngine = &engine;
		Check(TheGameEngine != nullptr, "1f. TheGameEngine constructed (non-null, harness-local PosixGameEngine)");

		engine.init();

		Check(TheFileSystem != nullptr, "1g. TheFileSystem real-constructed by GameEngine::init() (non-null)");
		Check(TheArchiveFileSystem != nullptr, "1h. TheArchiveFileSystem real-constructed via createArchiveFileSystem() (non-null)");
		Check(TheThingFactory != nullptr, "1i. TheThingFactory real-constructed via createThingFactory() (non-null)");
		Check(TheModuleFactory != nullptr, "1j. TheModuleFactory real-constructed via createModuleFactory() (non-null, this milestone's own ModuleFactoryStub)");
		Check(TheGameClient != nullptr, "1k. TheGameClient real-constructed via createGameClient() (non-null)");
		Check(TheGameLogic != nullptr, "1l. TheGameLogic real-constructed via createGameLogic() (non-null, this milestone's own PosixGameLogicStub)");
		Check(TheTerrainLogic != nullptr, "1m. TheTerrainLogic real-constructed by GameLogic::init() (non-null, HarnessTerrainLogic)");
		Check(ThePlayerList != nullptr, "1n. ThePlayerList real-constructed by GameEngine::init() (non-null)");
		Check(TheRadar != nullptr, "1o. TheRadar real-constructed via createRadar() (non-null)");
		Check(TheSidesList != nullptr, "1p. TheSidesList real-constructed by GameEngine::init() (non-null)");
		Check(ThePartitionManager != nullptr, "1q. ThePartitionManager real-constructed by GameEngine::init() (non-null)");

		const ThingTemplate* namedUnitTemplate = TheThingFactory->findTemplate("M13NamedUnit");
		Check(namedUnitTemplate != nullptr, "1r. TheThingFactory->findTemplate(\"M13NamedUnit\") found the real, INI-parsed template");

		// TheSuperHackers @port Milestone 14 (native port plan, Draft 38):
		// the second, distinct-templated unit's own real, INI-parsed
		// ThingTemplate.
		const ThingTemplate* secondUnitTemplate = TheThingFactory->findTemplate("M14SecondUnit");
		Check(secondUnitTemplate != nullptr, "1s. TheThingFactory->findTemplate(\"M14SecondUnit\") found the real, INI-parsed template");

		// ---- Cost A: the team bootstrap (Draft 37's own "~4 harness
		// lines", all real public API, no engine changes). ----
		printf("=== Check 2: team bootstrap (Draft 37 Cost A) ===\n");
		TheSidesList->validateSides();
		Check(ThePlayerList->getNeutralPlayer() != nullptr, "2a. ThePlayerList->getNeutralPlayer() non-null after TheSidesList->validateSides()");

		ThePlayerList->newGame();
		Team* neutralDefaultTeam = ThePlayerList->getNeutralPlayer() ? ThePlayerList->getNeutralPlayer()->getDefaultTeam() : nullptr;
		Check(neutralDefaultTeam != nullptr, "2b. getNeutralPlayer()->getDefaultTeam() resolves (non-null) after ThePlayerList->newGame()");

		ThePartitionManager->init();
		Check(true, "2c. ThePartitionManager->init() returned without crashing (real, self-healing no-map body)");

		// ---- WW3D/GL setup (Milestone 11's own technique) - genuinely
		// safe to run AFTER engine.init() here (see this file's own header
		// comment for why). ----
		printf("=== Check 3: WW3D/GL init round trip ===\n");
		TheW3DFileSystem = new W3DFileSystem;
		Check(_TheFileFactory == static_cast<FileFactoryClass*>(TheW3DFileSystem),
			"3a. W3DFileSystem constructor installed itself as _TheFileFactory");

		WW3DErrorType init_result = WW3D::Init(nullptr);
		Check(init_result == WW3D_ERROR_OK, "3b. WW3D::Init returned WW3D_ERROR_OK");

		WW3DErrorType device_result = WW3D::Set_Render_Device(0, g_W, g_H, 32, /*windowed=*/1, /*resize_window=*/true);
		Check(device_result == WW3D_ERROR_OK, "3c. WW3D::Set_Render_Device returned WW3D_ERROR_OK");

		if (init_result != WW3D_ERROR_OK || device_result != WW3D_ERROR_OK)
		{
			fprintf(stderr, "RENDERNAMEDDRAWABLE_FAIL: WW3D init round-trip failed, aborting\n");
			g_AnyFailure = true;
			return 1;
		}

		WW3D::Set_Thumbnail_Enabled(false);
		DX8Wrapper::Set_Texture_Bitdepth(32);

		// ---- Step 0's own hard link/construction requirement: the real
		// scene + real asset manager, wired as W3DDisplay's own static
		// members BEFORE any Drawable is constructed (W3DModelDraw's own
		// constructor unconditionally dereferences both). ----
		printf("=== Check 4: real RTS3DScene/RTS2DScene + W3DAssetManager (this milestone's own hard requirement) ===\n");
		RTS3DScene* scene = NEW_REF(RTS3DScene, ());
		Check(scene != nullptr, "4a. RTS3DScene constructed (non-null)");
		scene->Set_Ambient_Light(Vector3(1.0f, 1.0f, 1.0f));

		LightClass* zeroLight = NEW_REF(LightClass, (LightClass::DIRECTIONAL));
		zeroLight->Set_Ambient(Vector3(0.0f, 0.0f, 0.0f));
		zeroLight->Set_Diffuse(Vector3(0.0f, 0.0f, 0.0f));
		zeroLight->Set_Specular(Vector3(0.0f, 0.0f, 0.0f));
		scene->setGlobalLight(zeroLight, 0);
		zeroLight->Release_Ref();

		W3DDisplay::m_3DScene = scene;

		RTS2DScene* scene2d = NEW_REF(RTS2DScene, ());
		Check(scene2d != nullptr, "4b. RTS2DScene constructed (non-null)");
		W3DDisplay::m_2DScene = scene2d;

		W3DAssetManager assetManager;
		W3DDisplay::m_assetManager = &assetManager;
		bool loaded = assetManager.Load_3D_Assets("m13unit.w3d");
		Check(loaded, "4c. Load_3D_Assets(\"m13unit.w3d\") succeeded via factory->TheFileSystem->archive resolution");

		// TheSuperHackers @port Milestone 14 (native port plan, Draft 38):
		// the second unit's own real asset load, into the SAME real
		// W3DAssetManager (a second Load_3D_Assets call on the real,
		// unmodified object - no per-call special-casing needed).
		bool loaded2 = assetManager.Load_3D_Assets("m14unit.w3d");
		Check(loaded2, "4d. Load_3D_Assets(\"m14unit.w3d\") succeeded via factory->TheFileSystem->archive resolution");

		// ---- Check 5 (THE MILESTONE'S ACTUAL PAYOFF CALL): a real, named
		// Object+Drawable through TheThingFactory->newObject() - UNIT A
		// (Milestone 13's own "M13NamedUnit"). ----
		printf("=== Check 5: TheThingFactory->newObject() - unit A (Milestone 13's own payoff call) ===\n");
		Object* objA = nullptr;
		if (namedUnitTemplate != nullptr && neutralDefaultTeam != nullptr && loaded)
		{
			objA = TheThingFactory->newObject(namedUnitTemplate, neutralDefaultTeam);
		}
		Check(objA != nullptr, "5a. TheThingFactory->newObject() returned a real, non-null Object (unit A)");

		Drawable* drawableA = objA ? objA->getDrawable() : nullptr;
		Check(drawableA != nullptr, "5b. objA->getDrawable() returned a real, non-null Drawable (unit A, bound by the real sendObjectCreated()/bindObjectAndDrawable() chain)");
		Check(drawableA != nullptr && drawableA->getTemplate() == namedUnitTemplate, "5c. drawableA->getTemplate() matches the real, parsed \"M13NamedUnit\" template");

		if (objA != nullptr)
		{
			objA->setPosition(&UNIT_A_WORLD_POS);
			Check(true, "5d. objA->setPosition() returned without crashing (see 5e for the real position readback)");
			const Coord3D* drawPosA = drawableA->getPosition();
			Check(drawPosA != nullptr &&
				std::fabs(drawPosA->x - UNIT_A_WORLD_POS.x) < 0.01f &&
				std::fabs(drawPosA->y - UNIT_A_WORLD_POS.y) < 0.01f,
				"5e. drawableA->getPosition() matches objA->setPosition()'s target (real Object::reactToTransformChange() sync)");
		}

		// TheSuperHackers @port Milestone 14 (native port plan, Draft 38):
		// UNIT B - a SECOND real, named Object+Drawable, through the exact
		// same real chain, a distinct template ("M14SecondUnit") at a
		// distinct world position (UNIT_B_WORLD_POS).
		printf("=== Check 5f: TheThingFactory->newObject() - unit B (Milestone 14's own second payoff call) ===\n");
		Object* objB = nullptr;
		if (secondUnitTemplate != nullptr && neutralDefaultTeam != nullptr && loaded2)
		{
			objB = TheThingFactory->newObject(secondUnitTemplate, neutralDefaultTeam);
		}
		Check(objB != nullptr, "5f. TheThingFactory->newObject() returned a real, non-null Object (unit B)");

		Drawable* drawableB = objB ? objB->getDrawable() : nullptr;
		Check(drawableB != nullptr, "5g. objB->getDrawable() returned a real, non-null Drawable (unit B, bound by the real sendObjectCreated()/bindObjectAndDrawable() chain)");
		Check(drawableB != nullptr && drawableB->getTemplate() == secondUnitTemplate, "5h. drawableB->getTemplate() matches the real, parsed \"M14SecondUnit\" template");
		Check(drawableB != nullptr && drawableA != nullptr && drawableB != drawableA, "5i. drawableB is a distinct real Drawable* from drawableA (two real, separate entries)");

		if (objB != nullptr)
		{
			objB->setPosition(&UNIT_B_WORLD_POS);
			Check(true, "5j. objB->setPosition() returned without crashing (see 5k for the real position readback)");
			const Coord3D* drawPosB = drawableB->getPosition();
			Check(drawPosB != nullptr &&
				std::fabs(drawPosB->x - UNIT_B_WORLD_POS.x) < 0.01f &&
				std::fabs(drawPosB->y - UNIT_B_WORLD_POS.y) < 0.01f,
				"5k. drawableB->getPosition() matches objB->setPosition()'s target (real Object::reactToTransformChange() sync)");
		}

		// ---- Check 6: BOTH drawables are in TheGameClient's real drawable
		// list, by name (Milestone 14's own first real proof that the list
		// actually holds more than one live entry - Draft 35 finding 12).
		// ----
		printf("=== Check 6: both drawables present in TheGameClient's real drawable list, by name ===\n");
		Drawable* foundA = nullptr;
		Drawable* foundB = nullptr;
		int drawableListCount = 0;
		for (Drawable* d = TheGameClient->firstDrawable(); d != nullptr; d = d->getNextDrawable())
		{
			++drawableListCount;
			if (d->getTemplate() != nullptr && d->getTemplate()->getName() == "M13NamedUnit")
				foundA = d;
			if (d->getTemplate() != nullptr && d->getTemplate()->getName() == "M14SecondUnit")
				foundB = d;
		}
		Check(drawableListCount >= 2, "6a. TheGameClient's real drawable list has at least 2 entries");
		Check(foundA != nullptr && foundA == drawableA, "6b. the real drawable list contains our exact unit A Drawable*, found by template name \"M13NamedUnit\"");
		Check(foundB != nullptr && foundB == drawableB, "6c. the real drawable list contains our exact unit B Drawable*, found by template name \"M14SecondUnit\"");

		// ---- Check 7: a real W3DView, driven through the real per-frame
		// camera API, framed on the MIDPOINT between unit A and unit B
		// (Milestone 11's own established technique; Milestone 14's own
		// wider framing so BOTH units land on screen at once, confirmed
		// below via the real view->worldToScreen(), not assumed from FOV
		// math). ----
		printf("=== Check 7: real W3DView, driven through the real per-frame camera API, framed on both units ===\n");
		W3DView* view = NEW W3DView;
		Check(view != nullptr, "7a. W3DView constructed (non-null)");

		view->init();
		view->setDefaultView(
			DEG_TO_RADF(TheWritableGlobalData->m_cameraPitch),
			DEG_TO_RADF(TheWritableGlobalData->m_cameraYaw),
			1.0f);
		view->setPitch(ViewDefaultPitchRadians);
		view->setAngle(DEG_TO_RADF(15.0f));
		view->setZoom(1.0f);
		const Coord3D MIDPOINT_WORLD_POS = {
			(UNIT_A_WORLD_POS.x + UNIT_B_WORLD_POS.x) * 0.5f,
			(UNIT_A_WORLD_POS.y + UNIT_B_WORLD_POS.y) * 0.5f,
			0.0f
		};
		view->lookAt(&MIDPOINT_WORLD_POS);
		Check(CloseReal(view->getPosition().x, MIDPOINT_WORLD_POS.x, 0.01f) && CloseReal(view->getPosition().y, MIDPOINT_WORLD_POS.y, 0.01f),
			"7b. getPosition() == lookAt() target (the midpoint between unit A and unit B)");

		// ---- Real, implementation-time finding (this milestone's own step-0
		// spike, not foreseen by Draft 37): W3DView::updateCameraTransform()
		// (called internally from view->update() below) has its own
		// unconditional "if (TheGlobalData->m_headless) return;" early-out
		// (W3DView.cpp:752) - with m_headless left at TRUE (kept for
		// engine.init(), matching Milestone 12's own established, proven-safe
		// construction chain unchanged), a real run confirmed the camera
		// silently never moved (view->get3DCameraPosition() stayed pinned at
		// world origin regardless of lookAt()/setZoom()/setAngle()). Flipping
		// m_headless to FALSE HERE - well after engine.init()/GameClient::init()
		// have already fully run and every subsystem is already constructed -
		// is safe and disclosed: a real, targeted attempt at flipping it BEFORE
		// engine.init() instead (so the "real" non-dummy construction path is
		// used throughout) was tried first and found genuinely unsafe - a real,
		// pre-existing engine bug (InGameUI::init() -> ControlBar::init() ->
		// "TheWindowManager->winGetWindowFromId(...)->winGetPosition(...)"
		// dereferences the result of a real, non-guarded lookup with NO null
		// check, ControlBar.cpp:1097-1098, genuinely reached once
		// GameClient::init() itself takes real, non-headless code paths) -
		// confirming m_headless must stay TRUE through construction and only
		// flip afterward, exactly as done here. Nothing downstream of this
		// point reads m_headless again.
		TheWritableGlobalData->m_headless = FALSE;

		view->update();
		Check(true, "7c. view->update() returned without crashing (real isGamePaused()/isTimeFrozenDebug()/getAxisAlignedViewRegion()/iterateDrawablesInRegion() all executed)");

		// ---- Real screen positions for both units, via the real
		// view->worldToScreen() - used for both the pixel checks (Check 8)
		// and the pickDrawable() identity checks (Check 9) below, instead of
		// an assumed/hardcoded screen position (Milestone 13's own
		// screen-center shortcut only worked because it had exactly one,
		// exactly-centered unit). ----
		ICoord2D screenA, screenB;
		Bool insideA = view->worldToScreen(&UNIT_A_WORLD_POS, &screenA);
		Bool insideB = view->worldToScreen(&UNIT_B_WORLD_POS, &screenB);
		Check(insideA, "7d. worldToScreen() places unit A's world position inside the camera frustum");
		Check(insideB, "7e. worldToScreen() places unit B's world position inside the camera frustum");
		printf("      unit A projected screen pos: (%d,%d); unit B projected screen pos: (%d,%d)\n", screenA.x, screenA.y, screenB.x, screenB.y);
		Check(screenA.x >= 0 && screenA.x < g_W && screenA.y >= 0 && screenA.y < g_H, "7f. unit A's projected screen position is within the framebuffer bounds");
		Check(screenB.x >= 0 && screenB.x < g_W && screenB.y >= 0 && screenB.y < g_H, "7g. unit B's projected screen position is within the framebuffer bounds");
		int screenSeparation = std::abs(screenA.x - screenB.x) + std::abs(screenA.y - screenB.y);
		Check(screenSeparation > 20, "7h. unit A and unit B project to sufficiently separated, non-overlapping screen positions");

		const Vector3 CLEAR_COLOR(BG_R / 255.0f, BG_G / 255.0f, BG_B / 255.0f);

		// ---- Check 7i (DIAGNOSTIC - Draft 38's own explicit "investigate
		// and report honestly" question): render ONCE, with NEITHER unit's
		// drawable->draw() called yet, to determine empirically whether
		// view->update()'s own internal "TheGameClient->
		// iterateDrawablesInRegion(&axisAlignedRegion, drawDrawable,
		// nullptr)" call (W3DView.cpp:1717 - GameClient::iterateDrawablesInRegion(),
		// GameClient.cpp:820-837, a plain unconditional walk of the WHOLE
		// m_drawableList with no early break) already pushed BOTH real
		// drawables' render-object transforms on its own. This is a genuine
		// measurement, not an assumption in either direction - see this
		// file's own header comment for the real, measured result. ----
		printf("=== Check 7i (diagnostic): rendering BEFORE any manual drawable->draw() call ===\n");
		Check(WW3D::Begin_Render(true, true, CLEAR_COLOR) == WW3D_ERROR_OK, "7i-1. WW3D::Begin_Render returned WW3D_ERROR_OK (diagnostic pass)");
		view->drawView();
		Check(WW3D::End_Render(true) == WW3D_ERROR_OK, "7i-2. WW3D::End_Render returned WW3D_ERROR_OK (diagnostic pass)");
		bool autoPushWorkedA = false, autoPushWorkedB = false;
		{
			unsigned char* fbo = Read_Fbo_Pixels_TopDown(g_W, g_H);
			int idxA = (screenA.y * g_W + screenA.x) * 4;
			int idxB = (screenB.y * g_W + screenB.x) * 4;
			autoPushWorkedA = Close(fbo[idxA + 0], TEX_R) && Close(fbo[idxA + 1], TEX_G) && Close(fbo[idxA + 2], TEX_B);
			autoPushWorkedB = Close(fbo[idxB + 0], TEX2_R) && Close(fbo[idxB + 1], TEX2_G) && Close(fbo[idxB + 2], TEX2_B);
			free(fbo);
		}
		printf("      diagnostic result: unit A auto-pushed by view->update() alone = %s, unit B auto-pushed = %s\n",
			autoPushWorkedA ? "YES" : "NO", autoPushWorkedB ? "YES" : "NO");

		// TheSuperHackers @port Milestone 14 (native port plan, Draft 38):
		// apply Milestone 13's own manual "drawable->draw()" workaround only
		// to whichever real drawable(s) the diagnostic above found NOT
		// already pushed automatically - see this file's own header comment
		// for the real, measured outcome this milestone's own step-0 spike
		// found for THIS harness's own two-unit camera configuration.
		if (!autoPushWorkedA && drawableA != nullptr)
			drawableA->draw();
		if (!autoPushWorkedB && drawableB != nullptr)
			drawableB->draw();
		Check(true, "7j. any needed manual drawable->draw() call(s) returned without crashing");

		// ---- Check 8: pixel check on both real named units' rendered
		// meshes, at their own real, projected screen positions (Check
		// 7d-7h above). ----
		printf("=== Check 8: pixel check on both real named units' rendered meshes ===\n");
		Check(WW3D::Begin_Render(true, true, CLEAR_COLOR) == WW3D_ERROR_OK, "8a. WW3D::Begin_Render returned WW3D_ERROR_OK");
		view->drawView();
		Check(WW3D::End_Render(true) == WW3D_ERROR_OK, "8b. WW3D::End_Render returned WW3D_ERROR_OK");

		{
			unsigned char* fbo = Read_Fbo_Pixels_TopDown(g_W, g_H);
			Check_Pixel(fbo, 5, 5, BG_R, BG_G, BG_B, "8c. background == clear color");
			Check_Pixel(fbo, screenA.x, screenA.y, TEX_R, TEX_G, TEX_B, "8d. authored texel at unit A's own projected screen position (the real unit A render object)");
			Check_Pixel(fbo, screenB.x, screenB.y, TEX2_R, TEX2_G, TEX2_B, "8e. authored texel at unit B's own projected screen position (the real unit B render object)");
			free(fbo);
		}

		// ---- Check 9: pickDrawable() distinguishes the two real
		// candidates - identity + template name, in BOTH directions (Draft
		// 35 finding 13's own "not just hits the one thing that exists vs.
		// nothing" gap, closed for real this milestone). ----
		printf("=== Check 9: pickDrawable() real ray-cast identity proof - distinguishing unit A from unit B ===\n");
		Drawable* hitA = view->pickDrawable(&screenA, false, PICK_TYPE_SELECTABLE);
		Check(hitA == drawableA, "9a. pickDrawable() at unit A's screen position returns the REAL unit A drawable pointer (identity match)");
		Check(hitA != nullptr && hitA->getTemplate() != nullptr && hitA->getTemplate()->getName() == "M13NamedUnit",
			"9b. the picked drawable's getTemplate()->getName() == \"M13NamedUnit\" (not unit B's)");

		Drawable* hitB = view->pickDrawable(&screenB, false, PICK_TYPE_SELECTABLE);
		Check(hitB == drawableB, "9c. pickDrawable() at unit B's screen position returns the REAL unit B drawable pointer (identity match)");
		Check(hitB != nullptr && hitB->getTemplate() != nullptr && hitB->getTemplate()->getName() == "M14SecondUnit",
			"9d. the picked drawable's getTemplate()->getName() == \"M14SecondUnit\" (not unit A's)");

		Check(hitA != nullptr && hitB != nullptr && hitA != hitB, "9e. pickDrawable() at unit A's and unit B's screen positions return two DIFFERENT real drawables (a real cross-check, not just non-null)");

		ICoord2D missScreen;
		missScreen.x = 5;
		missScreen.y = 5;
		Drawable* missResult = view->pickDrawable(&missScreen, false, PICK_TYPE_SELECTABLE);
		Check(missResult == nullptr, "9f. pickDrawable() at a background screen point returns nullptr (real castRay() miss)");

		// ---- Check 10: TheGameClient->iterateDrawablesInRegion()'s real
		// Region3D-based traversal (GameClient.cpp:820-837 - a plain,
		// unconditional walk of TheGameClient's own m_drawableList, DISTINCT
		// from W3DView::iterateDrawablesInRegion()'s own IRegion2D/
		// point-pick overload that pickDrawable()-based Check 9 above
		// exercises) actually visits BOTH live entries for a region
		// containing both, and correctly excludes the other for a region
		// containing only one - Draft 35 finding 12's own "only ever tested
		// with one live entry" gap, closed for real this milestone. ----
		printf("=== Check 10: iterateDrawablesInRegion() real multi-entry region traversal ===\n");
		Region3D bothRegion;
		bothRegion.lo.x = std::min(UNIT_A_WORLD_POS.x, UNIT_B_WORLD_POS.x) - 50.0f;
		bothRegion.lo.y = std::min(UNIT_A_WORLD_POS.y, UNIT_B_WORLD_POS.y) - 50.0f;
		bothRegion.lo.z = -50.0f;
		bothRegion.hi.x = std::max(UNIT_A_WORLD_POS.x, UNIT_B_WORLD_POS.x) + 50.0f;
		bothRegion.hi.y = std::max(UNIT_A_WORLD_POS.y, UNIT_B_WORLD_POS.y) + 50.0f;
		bothRegion.hi.z = 50.0f;
		IterateResult bothResult;
		TheGameClient->iterateDrawablesInRegion(&bothRegion, IterateCallback, &bothResult);
		Check(bothResult.count == 2, "10a. iterateDrawablesInRegion() callback fired exactly twice for a region containing both real units (the loop actually walks a multi-entry list, not just the first)");
		Check(std::find(bothResult.hits.begin(), bothResult.hits.end(), drawableA) != bothResult.hits.end(),
			"10b. the callback's own hit list contains our exact unit A Drawable*");
		Check(std::find(bothResult.hits.begin(), bothResult.hits.end(), drawableB) != bothResult.hits.end(),
			"10c. the callback's own hit list contains our exact unit B Drawable*");

		Region3D onlyARegion;
		onlyARegion.lo.x = UNIT_A_WORLD_POS.x - 50.0f; onlyARegion.lo.y = UNIT_A_WORLD_POS.y - 50.0f; onlyARegion.lo.z = -50.0f;
		onlyARegion.hi.x = UNIT_A_WORLD_POS.x + 50.0f; onlyARegion.hi.y = UNIT_A_WORLD_POS.y + 50.0f; onlyARegion.hi.z = 50.0f;
		IterateResult onlyAResult;
		TheGameClient->iterateDrawablesInRegion(&onlyARegion, IterateCallback, &onlyAResult);
		Check(onlyAResult.count == 1, "10d. a region containing only unit A fires the callback exactly once (real negative control - unit B correctly excluded)");
		Check(onlyAResult.count == 1 && onlyAResult.lastHit == drawableA, "10e. the one hit is unit A specifically");

		Region3D onlyBRegion;
		onlyBRegion.lo.x = UNIT_B_WORLD_POS.x - 50.0f; onlyBRegion.lo.y = UNIT_B_WORLD_POS.y - 50.0f; onlyBRegion.lo.z = -50.0f;
		onlyBRegion.hi.x = UNIT_B_WORLD_POS.x + 50.0f; onlyBRegion.hi.y = UNIT_B_WORLD_POS.y + 50.0f; onlyBRegion.hi.z = 50.0f;
		IterateResult onlyBResult;
		TheGameClient->iterateDrawablesInRegion(&onlyBRegion, IterateCallback, &onlyBResult);
		Check(onlyBResult.count == 1, "10f. a region containing only unit B fires the callback exactly once (real negative control - unit A correctly excluded)");
		Check(onlyBResult.count == 1 && onlyBResult.lastHit == drawableB, "10g. the one hit is unit B specifically");

		Region3D emptyRegion;
		emptyRegion.lo.x = std::max(UNIT_A_WORLD_POS.x, UNIT_B_WORLD_POS.x) + 10000.0f;
		emptyRegion.lo.y = std::max(UNIT_A_WORLD_POS.y, UNIT_B_WORLD_POS.y) + 10000.0f;
		emptyRegion.lo.z = -50.0f;
		emptyRegion.hi.x = emptyRegion.lo.x + 100.0f;
		emptyRegion.hi.y = emptyRegion.lo.y + 100.0f;
		emptyRegion.hi.z = 50.0f;
		IterateResult emptyResult;
		TheGameClient->iterateDrawablesInRegion(&emptyRegion, IterateCallback, &emptyResult);
		Check(emptyResult.count == 0, "10h. iterateDrawablesInRegion() callback did NOT fire for a region far away from both real units");

		// ---- No teardown: matches Milestone 12's own established
		// precedent (see this file's own header comment). Process exit
		// reclaims everything. ----
		printf("=== Teardown: deliberately skipped (matches Milestone 12's own established precedent) ===\n");
	}

	shutdownMemoryManager();

	TheAsciiStringCriticalSection = nullptr;
	TheUnicodeStringCriticalSection = nullptr;
	TheDmaCriticalSection = nullptr;
	TheMemoryPoolCriticalSection = nullptr;
	TheDebugLogCriticalSection = nullptr;

	if (g_AnyFailure)
	{
		fprintf(stderr, "RENDERNAMEDDRAWABLE_FAIL: one or more checks failed (see above)\n");
		return 1;
	}

	printf("RENDERNAMEDDRAWABLE_OK: all checks passed\n");
	return 0;
}
