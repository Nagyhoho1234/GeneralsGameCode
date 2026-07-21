// Phase 5(a) Milestone 1 verification harness (native port plan, see
// docs/native-port-plan.md): drives the real device-init call chain
// (DX8Wrapper::Init -> Set_Render_Device -> Create_Device -> Begin_Scene/
// Clear/End_Scene) on the PortableD3D8 GL backend, then reads back the
// offscreen FBO and asserts every pixel matches the requested clear color -
// not just "did it crash", matching native-port-spike's own verification
// bar. Gated RTS_BUILD_TESTS AND NOT WIN32 (not run on Windows - that path
// already has the real D3D8 backend and its own CI).
//
// Calls DX8Wrapper:: directly rather than through the WW3D:: wrapper class.
// WW3D::Init()/Set_Render_Device() live in ww3d.cpp, a single monolithic
// translation unit whose *other* functions (DX8MeshRendererClass,
// BoxRenderObjClass, TextureFilterClass, W3D memory pools, ...) reference
// dozens of WW3D2 subsystems that are still per-tree/Windows-only and out
// of scope for this milestone - every symbol referenced anywhere in that
// object file must resolve at link time even though this harness only ever
// calls a couple of its functions. DX8Wrapper is the actual unit Milestone 1
// ports; ww3d.cpp itself is exercised for real once Phase 4 windowing and
// the rest of WW3D2 are unified.
//
// Explicit non-goals (see the Milestone 1 plan): no mesh rendering, no
// texture loading, no window visible on screen - this only proves device
// init + a GL-backed clear reaches the framebuffer through the same
// DX8Wrapper entry points the real game uses.
#include <cstdio>
#include <cstdlib>

#include "dx8wrapper.h"
#include "PortableD3D8/gl_core33.h"

#include <GLFW/glfw3.h>

int main()
{
	if (!DX8Wrapper::Init(nullptr))
	{
		fprintf(stderr, "RENDERDEVICEINIT_FAIL: DX8Wrapper::Init failed\n");
		return 1;
	}

	const int W = 256, H = 256;
	if (!DX8Wrapper::Set_Render_Device(0, W, H, 32, 0))
	{
		fprintf(stderr, "RENDERDEVICEINIT_FAIL: DX8Wrapper::Set_Render_Device failed\n");
		DX8Wrapper::Shutdown();
		return 1;
	}

	const Vector3 clear_color(0.2f, 0.4f, 0.8f);

	DX8Wrapper::Begin_Scene();
	DX8Wrapper::Clear(true, true, clear_color);
	DX8Wrapper::End_Scene(false);

	unsigned char* pixels = (unsigned char*)malloc(4 * W * H);
	glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	const unsigned char expected_r = (unsigned char)(clear_color.X * 255.0f + 0.5f);
	const unsigned char expected_g = (unsigned char)(clear_color.Y * 255.0f + 0.5f);
	const unsigned char expected_b = (unsigned char)(clear_color.Z * 255.0f + 0.5f);

	bool all_match = true;
	for (int i = 0; i < W * H && all_match; ++i)
	{
		unsigned char r = pixels[i * 4 + 0];
		unsigned char g = pixels[i * 4 + 1];
		unsigned char b = pixels[i * 4 + 2];
		// Allow +/-1 for float->8bit rounding, matching native-port-spike's tolerance.
		if (abs(r - expected_r) > 1 || abs(g - expected_g) > 1 || abs(b - expected_b) > 1)
		{
			all_match = false;
		}
	}

	free(pixels);

	if (!all_match)
	{
		fprintf(stderr, "RENDERDEVICEINIT_FAIL: cleared framebuffer did not match the requested color\n");
		DX8Wrapper::Shutdown();
		return 1;
	}

	DX8Wrapper::Shutdown();
	printf("RENDERDEVICEINIT_OK: device init + GL clear-to-color verified (%dx%d)\n", W, H);
	return 0;
}
