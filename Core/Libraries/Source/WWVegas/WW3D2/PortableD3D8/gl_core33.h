// Minimal hand-rolled GL 3.3 core-profile function loader.
//
// Windows only exports GL 1.1 from opengl32.dll; everything from 1.2 onward
// (including all of shader/VAO/VBO/FBO) must be resolved at runtime via
// wglGetProcAddress/glfwGetProcAddress. Written by hand instead of pulling in
// glad/GLEW so this spike stays a single self-contained, auditable unit with
// no generated-code dependency - the whole point of the spike is inspecting
// exact GL semantics, not abstracting them away.
//
// Constant values and typedefs below are taken from the public Khronos
// OpenGL registry (gl.xml / glcorearb.h) - stable ABI numbers, not vendor
// code.
#pragma once

#if defined(_WIN32)
// GL/gl.h's WINGDIAPI/APIENTRY macros are only self-consistent when
// windows.h has already been included - without it, its own fallback
// definitions conflict with themselves in the same header.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
// Linux/macOS: APIENTRY is a Windows calling-convention artifact with no
// equivalent need elsewhere - defined empty here so the same typedefs below
// compile unchanged on every platform this spike targets, matching how the
// Khronos headers themselves handle non-Windows platforms.
#define APIENTRY
#endif

#if defined(__APPLE__)
// macOS has no <GL/gl.h> at all - GL 3.2+ core-profile entry points live in
// <OpenGL/gl3.h> as part of the OpenGL framework, and (unlike Windows/Linux)
// are real linked symbols, not just runtime-resolved ones. This header still
// defines the same GL_* constants/typedefs (GLenum, GLuint, ...) the rest of
// this file relies on, so no other code in this loader needs to change -
// gl_core33_load() below still resolves gl_GenVertexArrays etc. via
// glfwGetProcAddress uniformly across platforms; GLFW documents that this
// works correctly on its Cocoa/NSGL backend too, it's just not the only
// valid way to get these symbols on macOS the way it is elsewhere.
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

// --- constants (Khronos registry values) ---
// Guarded with #ifndef throughout: Apple's <OpenGL/gl3.h> is itself a GL 3.2
// core header and almost certainly already defines most of these (unlike
// Windows' GL/gl.h, which is GL 1.1-only) - guarding avoids a macro
// redefinition error if the values ever differ, and is a no-op otherwise
// since these are all stable Khronos registry constants.
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER                  0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER          0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW                   0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW                  0x88E8
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER               0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER                 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS                0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS                   0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH               0x8B84
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER                   0x8D40
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER                  0x8D41
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0             0x8CE0
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT              0x8D00
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24             0x81A6
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE          0x8CD5
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0                      0x84C0
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE                 0x812F
#endif

#include <cstddef> // ptrdiff_t - MSVC pulls this in transitively via windows.h, GCC/Clang do not

typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;

// --- function pointer typedefs (subset actually used by this spike) ---
typedef void      (APIENTRY* PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void      (APIENTRY* PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void      (APIENTRY* PFNGLDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint*);
typedef void      (APIENTRY* PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void      (APIENTRY* PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void      (APIENTRY* PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void      (APIENTRY* PFNGLBUFFERSUBDATAPROC)(GLenum, GLintptr, GLsizeiptr, const void*);
typedef void      (APIENTRY* PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void      (APIENTRY* PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void      (APIENTRY* PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef GLuint    (APIENTRY* PFNGLCREATESHADERPROC)(GLenum);
typedef void      (APIENTRY* PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void      (APIENTRY* PFNGLCOMPILESHADERPROC)(GLuint);
typedef void      (APIENTRY* PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void      (APIENTRY* PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void      (APIENTRY* PFNGLDELETESHADERPROC)(GLuint);
typedef GLuint    (APIENTRY* PFNGLCREATEPROGRAMPROC)(void);
typedef void      (APIENTRY* PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void      (APIENTRY* PFNGLLINKPROGRAMPROC)(GLuint);
typedef void      (APIENTRY* PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void      (APIENTRY* PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void      (APIENTRY* PFNGLUSEPROGRAMPROC)(GLuint);
typedef void      (APIENTRY* PFNGLDELETEPROGRAMPROC)(GLuint);
typedef GLint     (APIENTRY* PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void      (APIENTRY* PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void      (APIENTRY* PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void      (APIENTRY* PFNGLUNIFORM4FPROC)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void      (APIENTRY* PFNGLUNIFORMMATRIX4FVPROC)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void      (APIENTRY* PFNGLGENFRAMEBUFFERSPROC)(GLsizei, GLuint*);
typedef void      (APIENTRY* PFNGLBINDFRAMEBUFFERPROC)(GLenum, GLuint);
typedef void      (APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef GLenum    (APIENTRY* PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum);
typedef void      (APIENTRY* PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void      (APIENTRY* PFNGLGENRENDERBUFFERSPROC)(GLsizei, GLuint*);
typedef void      (APIENTRY* PFNGLBINDRENDERBUFFERPROC)(GLenum, GLuint);
typedef void      (APIENTRY* PFNGLRENDERBUFFERSTORAGEPROC)(GLenum, GLenum, GLsizei, GLsizei);
typedef void      (APIENTRY* PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum, GLenum, GLenum, GLuint);
typedef void      (APIENTRY* PFNGLDELETERENDERBUFFERSPROC)(GLsizei, const GLuint*);
typedef void      (APIENTRY* PFNGLACTIVETEXTUREPROC)(GLenum);

extern PFNGLGENVERTEXARRAYSPROC gl_GenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC gl_BindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC gl_DeleteVertexArrays;
extern PFNGLGENBUFFERSPROC gl_GenBuffers;
extern PFNGLBINDBUFFERPROC gl_BindBuffer;
extern PFNGLBUFFERDATAPROC gl_BufferData;
extern PFNGLBUFFERSUBDATAPROC gl_BufferSubData;
extern PFNGLDELETEBUFFERSPROC gl_DeleteBuffers;
extern PFNGLVERTEXATTRIBPOINTERPROC gl_VertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC gl_EnableVertexAttribArray;
extern PFNGLCREATESHADERPROC gl_CreateShader;
extern PFNGLSHADERSOURCEPROC gl_ShaderSource;
extern PFNGLCOMPILESHADERPROC gl_CompileShader;
extern PFNGLGETSHADERIVPROC gl_GetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC gl_GetShaderInfoLog;
extern PFNGLDELETESHADERPROC gl_DeleteShader;
extern PFNGLCREATEPROGRAMPROC gl_CreateProgram;
extern PFNGLATTACHSHADERPROC gl_AttachShader;
extern PFNGLLINKPROGRAMPROC gl_LinkProgram;
extern PFNGLGETPROGRAMIVPROC gl_GetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC gl_GetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC gl_UseProgram;
extern PFNGLDELETEPROGRAMPROC gl_DeleteProgram;
extern PFNGLGETUNIFORMLOCATIONPROC gl_GetUniformLocation;
extern PFNGLUNIFORM1IPROC gl_Uniform1i;
extern PFNGLUNIFORM1FPROC gl_Uniform1f;
extern PFNGLUNIFORM4FPROC gl_Uniform4f;
extern PFNGLUNIFORMMATRIX4FVPROC gl_UniformMatrix4fv;
extern PFNGLGENFRAMEBUFFERSPROC gl_GenFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC gl_BindFramebuffer;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC gl_FramebufferTexture2D;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC gl_CheckFramebufferStatus;
extern PFNGLDELETEFRAMEBUFFERSPROC gl_DeleteFramebuffers;
extern PFNGLGENRENDERBUFFERSPROC gl_GenRenderbuffers;
extern PFNGLBINDRENDERBUFFERPROC gl_BindRenderbuffer;
extern PFNGLRENDERBUFFERSTORAGEPROC gl_RenderbufferStorage;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC gl_FramebufferRenderbuffer;
extern PFNGLDELETERENDERBUFFERSPROC gl_DeleteRenderbuffers;
extern PFNGLACTIVETEXTUREPROC gl_ActiveTexture;

// Resolves all of the above via the given loader (glfwGetProcAddress).
// Returns false (and leaves an unresolved pointer null) on first failure.
bool gl_core33_load(void* (*get_proc_address)(const char*));
