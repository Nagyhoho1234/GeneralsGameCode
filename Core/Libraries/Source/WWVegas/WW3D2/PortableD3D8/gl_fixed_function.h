/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Small external entry points into dx8wrapper_gl.cpp's fixed-function GLSL
// program (Milestone 2, Step 4/6), for Tests/RenderTexturedTriangle (Step 7)
// to drive directly. Deliberately separate from PortableD3D8/d3d8.h, which
// stays a pure D3D-vocabulary/vtable-shape stand-in with no GL-specific
// surface of its own - everything declared here is GL-side plumbing that
// exists only because this milestone does not wire a real
// DX8Wrapper::SetTransform(D3DTS_WORLD/VIEW/PROJECTION, ...) (see the
// non-goals in docs/native-port-plan.md's Draft 18).
#pragma once

#ifndef PORTABLE_GL_FIXED_FUNCTION_H
#define PORTABLE_GL_FIXED_FUNCTION_H

#ifndef _WIN32

// D3D-clip-space-to-GL-clip-space row remap (z in [0,w] -> z in [-w,w]).
// Both d3d/out_gl are column-major float[16] (GL uniform layout, not
// D3DMATRIX) - out_gl may alias d3d.
void Convert_D3D_Projection_To_GL(const float d3d[16], float out_gl[16]);

// Activates the one always-on fixed-function program and uploads its MVP
// uniform. mvp_gl must already be a complete, GL-column-major
// projection*view*model composition.
void Set_Fixed_Function_MVP(const float mvp_gl[16]);

#endif // !_WIN32

#endif // PORTABLE_GL_FIXED_FUNCTION_H
