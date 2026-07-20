// Phase 3 graphics-API spike (docs/native-port-plan.md).
//
// Validates, with running code rather than inspection alone, the semantics
// questions the plan's desk-check flagged as unresolved:
//   1. D3D8 (z in [0,w]) vs GL (z in [-w,w]) clip-space convention - proven
//      by constructing a D3D8-style projection matrix, applying the standard
//      row-remap, and checking it numerically agrees with a native GL
//      projection matrix at a set of sample points.
//   2. Alpha-test-reference behavior without glAlphaFunc (removed in GL 3.3
//      core) - emulated via fragment-shader `discard`, driven by the same
//      ShaderBits vocabulary shader.cpp uses (see shader_bits.h).
//   3. A render2d-equivalent textured quad plus one ShaderClass-driven
//      textured mesh, per the spike design called out in the plan doc.
//
// Renders offscreen to an FBO and dumps the result to spike_output.png so it
// can be inspected without a visible window - this also matters for later
// headless Mesa/llvmpipe runs on Linux CI, which have no display at all.
//
// NOT wired into the main game build. See CMakeLists.txt in this directory.

#define GLFW_INCLUDE_NONE // we manage our own GL 3.3 core loader; don't let GLFW pull in GL/gl.h too
#include <GLFW/glfw3.h>
#include "gl_core33.h"
#include "shader_bits.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <vector>

namespace {

struct Mat4 {
    // Column-major, matching GL's expected uniform layout.
    float m[16] = {0};
};

Mat4 mat4_identity()
{
    Mat4 r;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

Mat4 mat4_multiply(const Mat4& a, const Mat4& b)
{
    Mat4 r;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            r.m[col * 4 + row] = sum;
        }
    }
    return r;
}

Mat4 mat4_translate(float x, float y, float z)
{
    Mat4 r = mat4_identity();
    r.m[12] = x; r.m[13] = y; r.m[14] = z;
    return r;
}

Mat4 mat4_rotate_y(float radians)
{
    Mat4 r = mat4_identity();
    float c = cosf(radians), s = sinf(radians);
    r.m[0] = c;  r.m[8]  = s;
    r.m[2] = -s; r.m[10] = c;
    return r;
}

Mat4 mat4_ortho_gl(float left, float right, float bottom, float top, float n, float f)
{
    Mat4 r{};
    r.m[0] = 2.0f / (right - left);
    r.m[5] = 2.0f / (top - bottom);
    r.m[10] = -2.0f / (f - n);
    r.m[12] = -(right + left) / (right - left);
    r.m[13] = -(top + bottom) / (top - bottom);
    r.m[14] = -(f + n) / (f - n);
    r.m[15] = 1.0f;
    return r;
}

// Standard OpenGL perspective projection: right-handed, NDC z in [-1, 1].
Mat4 mat4_perspective_gl(float fovy_rad, float aspect, float n, float f)
{
    Mat4 r{};
    float t = 1.0f / tanf(fovy_rad * 0.5f);
    r.m[0] = t / aspect;
    r.m[5] = t;
    r.m[10] = (f + n) / (n - f);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * f * n) / (n - f);
    return r;
}

// D3D8-style NDC-z-range projection: RH (same eye-space convention as
// mat4_perspective_gl, w_clip = -z_eye) but z in [0, 1] instead of [-1, 1].
// Real D3D8 (D3DXMatrixPerspectiveFovLH) is additionally left-handed
// (w_clip = +z_eye) - that handedness swap is a separate, independently
// well-understood porting step (winding-order/cull-mode adjustment) and is
// deliberately NOT entangled here, so this function isolates exactly the
// z-range question the plan's desk-check flagged as unresolved. Mixing both
// changes in one matrix was an earlier bug in this file: it produced a
// large NDC delta that had nothing to do with the z-range remap and
// everything to do with silently comparing LH-convention output against
// RH-convention sample points.
Mat4 mat4_perspective_d3d(float fovy_rad, float aspect, float n, float f)
{
    Mat4 r{};
    float t = 1.0f / tanf(fovy_rad * 0.5f);
    r.m[0] = t / aspect;
    r.m[5] = t;
    r.m[10] = f / (n - f);           // RH, z in [0,1] at NDC (vs. GL's [-1,1])
    r.m[11] = -1.0f;                 // RH: w_clip = -z_eye, same as mat4_perspective_gl
    r.m[14] = (f * n) / (n - f);
    return r;
}

// Standard D3D-clip-space-to-GL-clip-space row remap: GL wants z' in
// [-w, w] where D3D produced z in [0, w]. z_gl = 2*z_d3d - w_d3d, which as a
// matrix operation is: row_z_gl = 2*row_z_d3d - row_w_d3d.
Mat4 convert_d3d_projection_to_gl(const Mat4& d3d)
{
    Mat4 r = d3d;
    // row 2 (z output) lives at m[2], m[6], m[10], m[14] in column-major layout.
    // row 3 (w output) lives at m[3], m[7], m[11], m[15].
    for (int col = 0; col < 4; ++col) {
        float z_row = d3d.m[col * 4 + 2];
        float w_row = d3d.m[col * 4 + 3];
        r.m[col * 4 + 2] = 2.0f * z_row - w_row;
    }
    return r;
}

void mat4_transform(const Mat4& m, float x, float y, float z, float out[4])
{
    for (int row = 0; row < 4; ++row) {
        out[row] = m.m[0 * 4 + row] * x + m.m[1 * 4 + row] * y + m.m[2 * 4 + row] * z + m.m[3 * 4 + row] * 1.0f;
    }
}

// Checks the D3D8-vs-GL clip-space row-remap formula numerically: projects a
// handful of view-space sample points through both the native-GL matrix and
// the D3D matrix-then-converted matrix, and checks the resulting NDC
// coordinates agree. Scope, precisely: this validates the remap formula
// against a hand-written D3D8-NDC-range *model* (mat4_perspective_d3d
// above), not against real D3D8 output or this engine's actual camera
// matrices (WW3D's camera.cpp builds its own frustum matrices, not
// exercised here) - both sides are built from the same fov/aspect code and
// differ only in the z-row, so the remap recovering GL's coefficients is an
// algebraic identity, not independent evidence D3D8 itself works this way.
// It would pass even if the hand-written D3D8 model were wrong. Useful as a
// check that this file's own row-remap code does what it claims to, not as
// proof the real conversion is correct end to end.
bool validate_clip_space_conversion(float fovy_rad, float aspect, float n, float f)
{
    Mat4 gl_proj = mat4_perspective_gl(fovy_rad, aspect, n, f);
    Mat4 d3d_proj = mat4_perspective_d3d(fovy_rad, aspect, n, f);
    Mat4 converted = convert_d3d_projection_to_gl(d3d_proj);

    const float sample_points[][3] = {
        {0.0f, 0.0f, -2.0f},
        {0.5f, -0.3f, -5.0f},
        {-1.2f, 0.8f, -1.5f},
        {0.0f, 0.0f, -9.99f},
        {2.0f, 2.0f, -50.0f},
    };

    float max_delta = 0.0f;
    for (auto& p : sample_points) {
        float clip_gl[4], clip_conv[4];
        mat4_transform(gl_proj, p[0], p[1], p[2], clip_gl);
        mat4_transform(converted, p[0], p[1], p[2], clip_conv);

        for (int i = 0; i < 4; ++i) {
            float ndc_gl = clip_gl[i] / clip_gl[3];
            float ndc_conv = clip_conv[i] / clip_conv[3];
            float delta = fabsf(ndc_gl - ndc_conv);
            if (delta > max_delta) max_delta = delta;
        }
    }

    printf("[clip-space check] max NDC delta between native-GL and D3D8-converted projection: %g\n", max_delta);
    return max_delta < 1e-5f;
}

GLuint compile_shader(GLenum type, const char* src)
{
    GLuint shader = gl_CreateShader(type);
    gl_ShaderSource(shader, 1, &src, nullptr);
    gl_CompileShader(shader);
    GLint ok = 0;
    gl_GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        gl_GetShaderInfoLog(shader, sizeof(log), nullptr, log);
        fprintf(stderr, "shader compile failed: %s\n", log);
        exit(1);
    }
    return shader;
}

GLuint build_program(const char* vs_src, const char* fs_src)
{
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    GLuint prog = gl_CreateProgram();
    gl_AttachShader(prog, vs);
    gl_AttachShader(prog, fs);
    gl_LinkProgram(prog);
    GLint ok = 0;
    gl_GetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        gl_GetProgramInfoLog(prog, sizeof(log), nullptr, log);
        fprintf(stderr, "program link failed: %s\n", log);
        exit(1);
    }
    gl_DeleteShader(vs);
    gl_DeleteShader(fs);
    return prog;
}

// GL blend-factor constants used below are GL 1.1 (already declared by
// <GL/gl.h>); GL_SRC_ALPHA/GL_ONE_MINUS_SRC_ALPHA/GL_SRC_COLOR/
// GL_ONE_MINUS_SRC_COLOR predate core-profile function loading entirely.
GLenum src_blend_to_gl(int v)
{
    using namespace ShaderBitsSpike;
    switch (v) {
        case SRCBLEND_ZERO: return GL_ZERO;
        case SRCBLEND_ONE: return GL_ONE;
        case SRCBLEND_SRC_ALPHA: return GL_SRC_ALPHA;
        case SRCBLEND_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
    }
    return GL_ONE;
}

GLenum dst_blend_to_gl(int v)
{
    using namespace ShaderBitsSpike;
    switch (v) {
        case DSTBLEND_ZERO: return GL_ZERO;
        case DSTBLEND_ONE: return GL_ONE;
        case DSTBLEND_SRC_COLOR: return GL_SRC_COLOR;
        case DSTBLEND_ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
        case DSTBLEND_SRC_ALPHA: return GL_SRC_ALPHA;
        case DSTBLEND_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
    }
    return GL_ZERO;
}

// Applies the blend/alpha-test-relevant subset of a ShaderBits word to
// current GL state + the ubershader's uniforms, the way a real DX8Wrapper
// backend swap would translate ShaderClass::Apply() call sites.
void apply_shader_bits(unsigned int bits, GLint loc_alpha_test, GLint loc_alpha_ref)
{
    using namespace ShaderBitsSpike;
    int src = get(bits, SHIFT_SRCBLEND, MASK_SRCBLEND);
    int dst = get(bits, SHIFT_DSTBLEND, MASK_DSTBLEND);
    int alpha_test = get(bits, SHIFT_ALPHATEST, MASK_ALPHATEST);

    if (src == SRCBLEND_ONE && dst == DSTBLEND_ZERO) {
        glDisable(GL_BLEND);
    } else {
        glEnable(GL_BLEND);
        glBlendFunc(src_blend_to_gl(src), dst_blend_to_gl(dst));
    }

    gl_Uniform1i(loc_alpha_test, alpha_test == ALPHATEST_ENABLE ? 1 : 0);
    gl_Uniform1f(loc_alpha_ref, 0.5f);
}

// Procedurally builds a checker pattern with a circular alpha cutout, so
// alpha-test/discard behavior is visually obvious in the output PNG: outside
// the circle, alpha is 0 and the fragment must be discarded, not blended.
std::vector<unsigned char> make_checker_circle_texture(int size)
{
    std::vector<unsigned char> pixels(size * size * 4);
    float radius = size * 0.45f;
    float cx = size * 0.5f, cy = size * 0.5f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            int idx = (y * size + x) * 4;
            bool checker = ((x / 8) % 2) ^ ((y / 8) % 2);
            unsigned char c = checker ? 230 : 40;
            float dx = x - cx, dy = y - cy;
            bool inside = (dx * dx + dy * dy) <= radius * radius;
            pixels[idx + 0] = c;
            pixels[idx + 1] = checker ? 180 : 40;
            pixels[idx + 2] = 60;
            pixels[idx + 3] = inside ? 255 : 0;
        }
    }
    return pixels;
}

} // namespace

int main()
{
    if (!validate_clip_space_conversion(1.0f, 1.0f, 1.0f, 100.0f)) {
        fprintf(stderr, "SPIKE_FAIL: D3D8-to-GL clip-space conversion did not match native GL projection\n");
        return 1;
    }

    glfwSetErrorCallback([](int error_code, const char* description) {
        fprintf(stderr, "GLFW error 0x%x: %s\n", error_code, description);
    });

    if (!glfwInit()) {
        fprintf(stderr, "SPIKE_FAIL: glfwInit failed\n");
        return 1;
    }

    // Forward-compatible core profile: required on macOS (whose GL is 4.1
    // core-only and strict), requested here too so this Windows run
    // surfaces any core-profile incompatibility early rather than only
    // failing later on the platform this whole plan targets.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // offscreen: FBO + glReadPixels only
    // The window's own default framebuffer is never rendered into or read
    // from - all real rendering happens in the separate FBO created below.
    // Requesting a minimal default framebuffer (no depth/stencil/samples)
    // reduces the pixel-format attribute combination NSGL/GLX/WGL has to
    // satisfy to the bare minimum, which matters in practice: a GitHub
    // Actions macOS runner failed with "NSGL: Failed to find a suitable
    // pixel format" (GLFW error 0x10009) when this wasn't set, even though
    // both the Windows/NVIDIA and Linux/Mesa-llvmpipe runs never hit this
    // (they were less strict about the default-framebuffer request GLFW's
    // built-in defaults ask for).
    glfwWindowHint(GLFW_DEPTH_BITS, 0);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);
    glfwWindowHint(GLFW_SAMPLES, 0);

    GLFWwindow* window = glfwCreateWindow(64, 64, "native-port-spike", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "SPIKE_FAIL: glfwCreateWindow failed (no GL 3.3 core context available)\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    // Function-pointer-to-function-pointer reinterpret_cast is technically
    // conditionally-supported rather than portable C++, but it's the
    // standard pattern every mainstream GL loader (glad, GLEW, GLFW's own
    // examples) relies on, and it's well-defined in practice on every ABI
    // this spike targets (Windows/Linux/macOS all represent function
    // pointers as plain addresses).
    if (!gl_core33_load(reinterpret_cast<void* (*)(const char*)>(&glfwGetProcAddress))) {
        fprintf(stderr, "SPIKE_FAIL: failed to resolve a required GL 3.3 core entry point\n");
        return 1;
    }

    printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));

    const int W = 512, H = 512;

    GLuint fbo, color_tex, depth_rb;
    gl_GenFramebuffers(1, &fbo);
    gl_BindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &color_tex);
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);

    gl_GenRenderbuffers(1, &depth_rb);
    gl_BindRenderbuffer(GL_RENDERBUFFER, depth_rb);
    gl_RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, W, H);
    gl_FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);

    if (gl_CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "SPIKE_FAIL: offscreen FBO incomplete\n");
        return 1;
    }

    glViewport(0, 0, W, H);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f); // mid-gray: makes blend/discard results visually unambiguous
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const char* vs_src =
        "#version 330 core\n"
        "layout(location=0) in vec3 aPos;\n"
        "layout(location=1) in vec2 aTexCoord;\n"
        "uniform mat4 uMVP;\n"
        "out vec2 vTexCoord;\n"
        "void main() { gl_Position = uMVP * vec4(aPos, 1.0); vTexCoord = aTexCoord; }\n";

    const char* fs_src =
        "#version 330 core\n"
        "in vec2 vTexCoord;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D uTex;\n"
        "uniform int uAlphaTest;\n"
        "uniform float uAlphaRef;\n"
        "void main() {\n"
        "    vec4 c = texture(uTex, vTexCoord);\n"
        "    if (uAlphaTest != 0 && c.a < uAlphaRef) discard;\n"
        "    FragColor = c;\n"
        "}\n";

    GLuint program = build_program(vs_src, fs_src);
    gl_UseProgram(program);
    GLint loc_mvp = gl_GetUniformLocation(program, "uMVP");
    GLint loc_tex = gl_GetUniformLocation(program, "uTex");
    GLint loc_alpha_test = gl_GetUniformLocation(program, "uAlphaTest");
    GLint loc_alpha_ref = gl_GetUniformLocation(program, "uAlphaRef");
    gl_Uniform1i(loc_tex, 0);

    // Shared quad geometry: unit quad in XY, [0,1] texcoords (GL bottom-left
    // origin). Real ported texture loads authored against D3D8's top-left
    // origin need a V-flip at upload or UV-generation time to match this -
    // noted here rather than faked, since validating it needs a real D3D8
    // reference render this spike does not have.
    float quad_verts[] = {
        //  x,     y,    z,   u,   v
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
         0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
    };

    GLuint vao, vbo;
    gl_GenVertexArrays(1, &vao);
    gl_BindVertexArray(vao);
    gl_GenBuffers(1, &vbo);
    gl_BindBuffer(GL_ARRAY_BUFFER, vbo);
    gl_BufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);
    gl_VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    gl_EnableVertexAttribArray(0);
    gl_VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    gl_EnableVertexAttribArray(1);

    auto tex_pixels = make_checker_circle_texture(128);
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl_ActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    // --- Quad A: render2d/UI-quad equivalent - orthographic, screen-space,
    // opaque textured modulate (ShaderBitsSpike::OPAQUE_MODULATE). ---
    {
        Mat4 proj = mat4_ortho_gl(0.0f, (float)W, 0.0f, (float)H, -1.0f, 1.0f);
        Mat4 model = mat4_multiply(mat4_translate(128.0f, H - 128.0f, 0.0f),
                                    [] { Mat4 s = mat4_identity(); s.m[0] = 200.0f; s.m[5] = 200.0f; return s; }());
        Mat4 mvp = mat4_multiply(proj, model);
        gl_UniformMatrix4fv(loc_mvp, 1, GL_FALSE, mvp.m);
        apply_shader_bits(ShaderBitsSpike::OPAQUE_MODULATE, loc_alpha_test, loc_alpha_ref);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // --- Quad B: ShaderClass-driven mesh - perspective-projected, tilted,
    // alpha-tested + additively blended (ShaderBitsSpike::ALPHATEST_ADDITIVE).
    // Uses the native-GL projection matrix, proven equivalent above to the
    // D3D8-matrix-then-converted one. ---
    {
        Mat4 proj = mat4_perspective_gl(1.0f, (float)W / H, 0.5f, 50.0f);
        Mat4 model = mat4_multiply(mat4_translate(0.0f, 0.0f, -2.5f), mat4_rotate_y(0.6f));
        Mat4 scale = mat4_identity();
        scale.m[0] = scale.m[5] = 1.8f;
        model = mat4_multiply(model, scale);
        Mat4 mvp = mat4_multiply(proj, model);
        gl_UniformMatrix4fv(loc_mvp, 1, GL_FALSE, mvp.m);
        apply_shader_bits(ShaderBitsSpike::ALPHATEST_ADDITIVE, loc_alpha_test, loc_alpha_ref);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // --- Small opaque quad closer to camera, overlapping quad B, to confirm
    // GL_DEPTH_TEST occlusion behaves correctly in this NDC z range. ---
    {
        Mat4 proj = mat4_perspective_gl(1.0f, (float)W / H, 0.5f, 50.0f);
        Mat4 model = mat4_translate(0.3f, 0.0f, -1.2f);
        Mat4 scale = mat4_identity();
        scale.m[0] = scale.m[5] = 0.5f;
        model = mat4_multiply(model, scale);
        Mat4 mvp = mat4_multiply(proj, model);
        gl_UniformMatrix4fv(loc_mvp, 1, GL_FALSE, mvp.m);
        apply_shader_bits(ShaderBitsSpike::OPAQUE_MODULATE, loc_alpha_test, loc_alpha_ref);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    std::vector<unsigned char> framebuffer(W * H * 4);
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer.data());

    // GL reads bottom-up; PNG (and D3D8's surface convention) is top-down -
    // another concrete origin-flip a real port must apply consistently.
    std::vector<unsigned char> flipped(W * H * 4);
    for (int y = 0; y < H; ++y) {
        memcpy(&flipped[y * W * 4], &framebuffer[(H - 1 - y) * W * 4], W * 4);
    }

    // Non-background pixel count is a cheap "something actually rendered"
    // sanity check independent of visually opening the PNG.
    int non_background = 0;
    for (int i = 0; i < W * H; ++i) {
        unsigned char r = flipped[i * 4 + 0], g = flipped[i * 4 + 1], b = flipped[i * 4 + 2];
        if (!(r == 127 || r == 128) || !(g == 127 || g == 128) || !(b == 127 || b == 128)) non_background++;
    }
    printf("non-background pixels: %d / %d\n", non_background, W * H);

    // Dump RGB only, not RGBA: the color buffer's alpha channel reflects
    // whatever the fragment shader last wrote (0 for non-blended fragments
    // whose source texture had alpha=0, e.g. quad A's un-blended checker
    // texture outside its own opaque-but-alpha-bearing source data). That's
    // correct GL/D3D framebuffer state, but PNG viewers composite alpha=0
    // as transparent/white, which reads as a rendering bug when it's really
    // just an artifact of round-tripping through a format that preserves
    // alpha. RGB-only avoids the false signal.
    std::vector<unsigned char> rgb_only(W * H * 3);
    for (int i = 0; i < W * H; ++i) {
        rgb_only[i * 3 + 0] = flipped[i * 4 + 0];
        rgb_only[i * 3 + 1] = flipped[i * 4 + 1];
        rgb_only[i * 3 + 2] = flipped[i * 4 + 2];
    }

    const char* out_path = "spike_output.png";
    if (!stbi_write_png(out_path, W, H, 3, rgb_only.data(), W * 3)) {
        fprintf(stderr, "SPIKE_FAIL: failed to write %s\n", out_path);
        return 1;
    }

    if (non_background < 1000) {
        fprintf(stderr, "SPIKE_FAIL: suspiciously few non-background pixels rendered\n");
        return 1;
    }

    printf("SPIKE_OK: wrote %s\n", out_path);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
