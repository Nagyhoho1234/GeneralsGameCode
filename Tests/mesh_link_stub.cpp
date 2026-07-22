// Link-only stub for pre-Milestone-5 harnesses (RenderEngineDrawPath,
// RenderTexturePipeline) that link mapper.cpp (for VertexMaterialClass's
// TextureMapperClass* Mapper[] members) but not the real mesh.cpp - out of
// scope for what those milestones test, and pulling it in would drag
// dx8renderer.cpp/assetmgr.cpp's whole closure into harnesses deliberately
// scoped narrower than that (native port plan Phase 5(a) Milestone 5,
// Draft 24 Step 5).
//
// mapper.cpp's Reset_All_Texture_Mappers references MeshClass::Make_Unique
// even though neither harness ever calls it (nothing in their draw paths
// sets a real texture mapper) - the linker still has to resolve every
// symbol a linked TU references. This is the same class of stub Milestone
// 3 originally placed inside mapper.cpp itself (finding 6); Milestone 5
// Step 5 deleted that one because mesh.cpp is now portable and the real
// implementation would collide with it wherever both link (corei_ww3d2,
// and Step 7's new RenderW3DMesh harness) - this narrower copy exists
// only for the two harnesses that still don't link mesh.cpp at all.
#include "mesh.h"

void MeshClass::Make_Unique(bool force_meshmdl_clone)
{
	WWASSERT_PRINT(false, "MeshClass::Make_Unique: this harness does not link the real mesh.cpp");
}
