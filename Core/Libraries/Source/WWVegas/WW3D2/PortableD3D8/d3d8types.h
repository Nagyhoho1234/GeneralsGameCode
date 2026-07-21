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

// Stand-in for Microsoft's d3d8types.h (min-dx8-sdk, see cmake/dx8.cmake) on
// non-Windows. Enum values are copied verbatim from the real SDK header
// (build/win32/_deps/dx8-src/d3d8types.h) so state-cache DWORDs written by
// shared code (dx8wrapper.h's RenderStates[]/TextureStageStates[] tables)
// carry the same numeric meaning on every platform. Types referenced only
// as never-dereferenced pointer parameters on not-yet-implemented methods
// are forward-declared rather than fully defined - see PortableD3D8/d3d8.h.
#pragma once

#ifndef PORTABLE_D3D8TYPES_H
#define PORTABLE_D3D8TYPES_H

#ifndef _WIN32

#include <Utility/win32_compat.h>

#define D3DCOLOR_ARGB(a,r,g,b) \
	((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a,r,g,b)
#define D3DCOLOR_XRGB(r,g,b)   D3DCOLOR_ARGB(0xff,r,g,b)
#define D3DCOLOR_COLORVALUE(r,g,b,a) \
	D3DCOLOR_RGBA((DWORD)((r)*255.f),(DWORD)((g)*255.f),(DWORD)((b)*255.f),(DWORD)((a)*255.f))

#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
	((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
	((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))

typedef DWORD D3DCOLOR;

typedef struct _D3DVECTOR
{
	float x, y, z;
} D3DVECTOR;

typedef struct _D3DCOLORVALUE
{
	float r, g, b, a;
} D3DCOLORVALUE;

typedef struct _D3DMATRIX
{
	union
	{
		struct
		{
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		float m[4][4];
	};
} D3DMATRIX;

typedef struct _D3DRECT
{
	LONG x1, y1, x2, y2;
} D3DRECT;

typedef enum _D3DFORMAT
{
	D3DFMT_UNKNOWN              = 0,

	D3DFMT_R8G8B8               = 20,
	D3DFMT_A8R8G8B8             = 21,
	D3DFMT_X8R8G8B8             = 22,
	D3DFMT_R5G6B5               = 23,
	D3DFMT_X1R5G5B5             = 24,
	D3DFMT_A1R5G5B5             = 25,
	D3DFMT_A4R4G4B4             = 26,
	D3DFMT_R3G3B2               = 27,
	D3DFMT_A8                   = 28,
	D3DFMT_A8R3G3B2             = 29,
	D3DFMT_X4R4G4B4             = 30,
	D3DFMT_A2B10G10R10          = 31,
	D3DFMT_G16R16               = 34,

	D3DFMT_A8P8                 = 40,
	D3DFMT_P8                   = 41,

	D3DFMT_L8                   = 50,
	D3DFMT_A8L8                 = 51,
	D3DFMT_A4L4                 = 52,

	D3DFMT_V8U8                 = 60,
	D3DFMT_L6V5U5               = 61,
	D3DFMT_X8L8V8U8             = 62,
	D3DFMT_Q8W8V8U8             = 63,
	D3DFMT_V16U16               = 64,
	D3DFMT_W11V11U10            = 65,
	D3DFMT_A2W10V10U10          = 67,

	D3DFMT_UYVY                 = MAKEFOURCC('U', 'Y', 'V', 'Y'),
	D3DFMT_YUY2                 = MAKEFOURCC('Y', 'U', 'Y', '2'),
	D3DFMT_DXT1                 = MAKEFOURCC('D', 'X', 'T', '1'),
	D3DFMT_DXT2                 = MAKEFOURCC('D', 'X', 'T', '2'),
	D3DFMT_DXT3                 = MAKEFOURCC('D', 'X', 'T', '3'),
	D3DFMT_DXT4                 = MAKEFOURCC('D', 'X', 'T', '4'),
	D3DFMT_DXT5                 = MAKEFOURCC('D', 'X', 'T', '5'),

	D3DFMT_D16_LOCKABLE         = 70,
	D3DFMT_D32                  = 71,
	D3DFMT_D15S1                = 73,
	D3DFMT_D24S8                = 75,
	D3DFMT_D16                  = 80,
	D3DFMT_D24X8                = 77,
	D3DFMT_D24X4S4              = 79,

	D3DFMT_VERTEXDATA           = 100,
	D3DFMT_INDEX16              = 101,
	D3DFMT_INDEX32              = 102,

	D3DFMT_FORCE_DWORD          = 0x7fffffff
} D3DFORMAT;

typedef enum _D3DPOOL
{
	D3DPOOL_DEFAULT                 = 0,
	D3DPOOL_MANAGED                 = 1,
	D3DPOOL_SYSTEMMEM               = 2,
	D3DPOOL_SCRATCH                 = 3,

	D3DPOOL_FORCE_DWORD             = 0x7fffffff
} D3DPOOL;

typedef enum _D3DMULTISAMPLE_TYPE
{
	D3DMULTISAMPLE_NONE            = 0,
	D3DMULTISAMPLE_2_SAMPLES       = 2,
	D3DMULTISAMPLE_3_SAMPLES       = 3,
	D3DMULTISAMPLE_4_SAMPLES       = 4,
	D3DMULTISAMPLE_5_SAMPLES       = 5,
	D3DMULTISAMPLE_6_SAMPLES       = 6,
	D3DMULTISAMPLE_7_SAMPLES       = 7,
	D3DMULTISAMPLE_8_SAMPLES       = 8,
	D3DMULTISAMPLE_9_SAMPLES       = 9,
	D3DMULTISAMPLE_10_SAMPLES      = 10,
	D3DMULTISAMPLE_11_SAMPLES      = 11,
	D3DMULTISAMPLE_12_SAMPLES      = 12,
	D3DMULTISAMPLE_13_SAMPLES      = 13,
	D3DMULTISAMPLE_14_SAMPLES      = 14,
	D3DMULTISAMPLE_15_SAMPLES      = 15,
	D3DMULTISAMPLE_16_SAMPLES      = 16,

	D3DMULTISAMPLE_FORCE_DWORD     = 0x7fffffff
} D3DMULTISAMPLE_TYPE;

typedef enum _D3DSWAPEFFECT
{
	D3DSWAPEFFECT_DISCARD           = 1,
	D3DSWAPEFFECT_FLIP              = 2,
	D3DSWAPEFFECT_COPY              = 3,
	D3DSWAPEFFECT_COPY_VSYNC        = 4,

	D3DSWAPEFFECT_FORCE_DWORD       = 0x7fffffff
} D3DSWAPEFFECT;

typedef enum _D3DDEVTYPE
{
	D3DDEVTYPE_HAL         = 1,
	D3DDEVTYPE_REF         = 2,
	D3DDEVTYPE_SW          = 3,

	D3DDEVTYPE_FORCE_DWORD = 0x7fffffff
} D3DDEVTYPE;

typedef enum _D3DRESOURCETYPE
{
	D3DRTYPE_SURFACE                =  1,
	D3DRTYPE_VOLUME                 =  2,
	D3DRTYPE_TEXTURE                =  3,
	D3DRTYPE_VOLUMETEXTURE          =  4,
	D3DRTYPE_CUBETEXTURE            =  5,
	D3DRTYPE_VERTEXBUFFER           =  6,
	D3DRTYPE_INDEXBUFFER            =  7,

	D3DRTYPE_FORCE_DWORD            = 0x7fffffff
} D3DRESOURCETYPE;

#define D3DUSAGE_RENDERTARGET       0x00000001L
#define D3DUSAGE_DEPTHSTENCIL       0x00000002L
#define D3DUSAGE_WRITEONLY          0x00000008L
#define D3DUSAGE_SOFTWAREPROCESSING 0x00000010L
#define D3DUSAGE_DONOTCLIP          0x00000020L
#define D3DUSAGE_POINTS             0x00000040L
#define D3DUSAGE_RTPATCHES          0x00000080L
#define D3DUSAGE_NPATCHES           0x00000100L
#define D3DUSAGE_DYNAMIC            0x00000200L

typedef enum _D3DBACKBUFFER_TYPE
{
	D3DBACKBUFFER_TYPE_MONO         = 0,
	D3DBACKBUFFER_TYPE_LEFT         = 1,
	D3DBACKBUFFER_TYPE_RIGHT        = 2,

	D3DBACKBUFFER_TYPE_FORCE_DWORD  = 0x7fffffff
} D3DBACKBUFFER_TYPE;

typedef enum _D3DPRIMITIVETYPE
{
	D3DPT_POINTLIST             = 1,
	D3DPT_LINELIST              = 2,
	D3DPT_LINESTRIP             = 3,
	D3DPT_TRIANGLELIST          = 4,
	D3DPT_TRIANGLESTRIP         = 5,
	D3DPT_TRIANGLEFAN           = 6,

	D3DPT_FORCE_DWORD           = 0x7fffffff
} D3DPRIMITIVETYPE;

typedef enum _D3DSTATEBLOCKTYPE
{
	D3DSBT_ALL           = 1,
	D3DSBT_PIXELSTATE    = 2,
	D3DSBT_VERTEXSTATE   = 3,

	D3DSBT_FORCE_DWORD   = 0x7fffffff
} D3DSTATEBLOCKTYPE;

typedef enum _D3DLIGHTTYPE
{
	D3DLIGHT_POINT          = 1,
	D3DLIGHT_SPOT           = 2,
	D3DLIGHT_DIRECTIONAL    = 3,

	D3DLIGHT_FORCE_DWORD    = 0x7fffffff
} D3DLIGHTTYPE;

typedef enum _D3DRENDERSTATETYPE
{
	D3DRS_ZENABLE                   = 7,
	D3DRS_FILLMODE                  = 8,
	D3DRS_SHADEMODE                 = 9,
	D3DRS_LINEPATTERN               = 10,
	D3DRS_ZWRITEENABLE              = 14,
	D3DRS_ALPHATESTENABLE           = 15,
	D3DRS_LASTPIXEL                 = 16,
	D3DRS_SRCBLEND                  = 19,
	D3DRS_DESTBLEND                 = 20,
	D3DRS_CULLMODE                  = 22,
	D3DRS_ZFUNC                     = 23,
	D3DRS_ALPHAREF                  = 24,
	D3DRS_ALPHAFUNC                 = 25,
	D3DRS_DITHERENABLE              = 26,
	D3DRS_ALPHABLENDENABLE          = 27,
	D3DRS_FOGENABLE                 = 28,
	D3DRS_SPECULARENABLE            = 29,
	D3DRS_ZVISIBLE                  = 30,
	D3DRS_FOGCOLOR                  = 34,
	D3DRS_FOGTABLEMODE              = 35,
	D3DRS_FOGSTART                  = 36,
	D3DRS_FOGEND                    = 37,
	D3DRS_FOGDENSITY                = 38,
	D3DRS_EDGEANTIALIAS             = 40,
	D3DRS_ZBIAS                     = 47,
	D3DRS_RANGEFOGENABLE            = 48,
	D3DRS_STENCILENABLE             = 52,
	D3DRS_STENCILFAIL               = 53,
	D3DRS_STENCILZFAIL              = 54,
	D3DRS_STENCILPASS               = 55,
	D3DRS_STENCILFUNC               = 56,
	D3DRS_STENCILREF                = 57,
	D3DRS_STENCILMASK               = 58,
	D3DRS_STENCILWRITEMASK          = 59,
	D3DRS_TEXTUREFACTOR             = 60,
	D3DRS_WRAP0                     = 128,
	D3DRS_WRAP1                     = 129,
	D3DRS_WRAP2                     = 130,
	D3DRS_WRAP3                     = 131,
	D3DRS_WRAP4                     = 132,
	D3DRS_WRAP5                     = 133,
	D3DRS_WRAP6                     = 134,
	D3DRS_WRAP7                     = 135,
	D3DRS_CLIPPING                  = 136,
	D3DRS_LIGHTING                  = 137,
	D3DRS_AMBIENT                   = 139,
	D3DRS_FOGVERTEXMODE             = 140,
	D3DRS_COLORVERTEX               = 141,
	D3DRS_LOCALVIEWER               = 142,
	D3DRS_NORMALIZENORMALS          = 143,
	D3DRS_DIFFUSEMATERIALSOURCE     = 145,
	D3DRS_SPECULARMATERIALSOURCE    = 146,
	D3DRS_AMBIENTMATERIALSOURCE     = 147,
	D3DRS_EMISSIVEMATERIALSOURCE    = 148,
	D3DRS_VERTEXBLEND               = 151,
	D3DRS_CLIPPLANEENABLE           = 152,
	D3DRS_SOFTWAREVERTEXPROCESSING  = 153,
	D3DRS_POINTSIZE                 = 154,
	D3DRS_POINTSIZE_MIN             = 155,
	D3DRS_POINTSPRITEENABLE         = 156,
	D3DRS_POINTSCALEENABLE          = 157,
	D3DRS_POINTSCALE_A              = 158,
	D3DRS_POINTSCALE_B              = 159,
	D3DRS_POINTSCALE_C              = 160,
	D3DRS_MULTISAMPLEANTIALIAS      = 161,
	D3DRS_MULTISAMPLEMASK           = 162,
	D3DRS_PATCHEDGESTYLE            = 163,
	D3DRS_PATCHSEGMENTS             = 164,
	D3DRS_DEBUGMONITORTOKEN         = 165,
	D3DRS_POINTSIZE_MAX             = 166,
	D3DRS_INDEXEDVERTEXBLENDENABLE  = 167,
	D3DRS_COLORWRITEENABLE          = 168,
	D3DRS_TWEENFACTOR               = 170,
	D3DRS_BLENDOP                   = 171,
	D3DRS_POSITIONORDER             = 172,
	D3DRS_NORMALORDER               = 173,

	D3DRS_FORCE_DWORD               = 0x7fffffff
} D3DRENDERSTATETYPE;

typedef enum _D3DTEXTURESTAGESTATETYPE
{
	D3DTSS_COLOROP        =  1,
	D3DTSS_COLORARG1      =  2,
	D3DTSS_COLORARG2      =  3,
	D3DTSS_ALPHAOP        =  4,
	D3DTSS_ALPHAARG1      =  5,
	D3DTSS_ALPHAARG2      =  6,
	D3DTSS_BUMPENVMAT00   =  7,
	D3DTSS_BUMPENVMAT01   =  8,
	D3DTSS_BUMPENVMAT10   =  9,
	D3DTSS_BUMPENVMAT11   = 10,
	D3DTSS_TEXCOORDINDEX  = 11,
	D3DTSS_ADDRESSU       = 13,
	D3DTSS_ADDRESSV       = 14,
	D3DTSS_BORDERCOLOR    = 15,
	D3DTSS_MAGFILTER      = 16,
	D3DTSS_MINFILTER      = 17,
	D3DTSS_MIPFILTER      = 18,
	D3DTSS_MIPMAPLODBIAS  = 19,
	D3DTSS_MAXMIPLEVEL    = 20,
	D3DTSS_MAXANISOTROPY  = 21,
	D3DTSS_BUMPENVLSCALE  = 22,
	D3DTSS_BUMPENVLOFFSET = 23,
	D3DTSS_TEXTURETRANSFORMFLAGS = 24,
	D3DTSS_ADDRESSW       = 25,
	D3DTSS_COLORARG0      = 26,
	D3DTSS_ALPHAARG0      = 27,
	D3DTSS_RESULTARG      = 28,

	D3DTSS_FORCE_DWORD    = 0x7fffffff
} D3DTEXTURESTAGESTATETYPE;

typedef enum _D3DFILLMODE
{
	D3DFILL_POINT       = 1,
	D3DFILL_WIREFRAME   = 2,
	D3DFILL_SOLID       = 3,

	D3DFILL_FORCE_DWORD = 0x7fffffff
} D3DFILLMODE;

typedef enum _D3DCULL
{
	D3DCULL_NONE        = 1,
	D3DCULL_CW          = 2,
	D3DCULL_CCW         = 3,

	D3DCULL_FORCE_DWORD = 0x7fffffff
} D3DCULL;

typedef enum _D3DTRANSFORMSTATETYPE
{
	D3DTS_VIEW          = 2,
	D3DTS_PROJECTION    = 3,
	D3DTS_TEXTURE0      = 16,
	D3DTS_TEXTURE1      = 17,
	D3DTS_TEXTURE2      = 18,
	D3DTS_TEXTURE3      = 19,
	D3DTS_TEXTURE4      = 20,
	D3DTS_TEXTURE5      = 21,
	D3DTS_TEXTURE6      = 22,
	D3DTS_TEXTURE7      = 23,

	D3DTS_FORCE_DWORD   = 0x7fffffff
} D3DTRANSFORMSTATETYPE;

#define D3DTS_WORLDMATRIX(index) (D3DTRANSFORMSTATETYPE)(index + 256)
#define D3DTS_WORLD  D3DTS_WORLDMATRIX(0)
#define D3DTS_WORLD1 D3DTS_WORLDMATRIX(1)
#define D3DTS_WORLD2 D3DTS_WORLDMATRIX(2)
#define D3DTS_WORLD3 D3DTS_WORLDMATRIX(3)

#define D3DFVF_RESERVED0        0x001
#define D3DFVF_POSITION_MASK    0x00E
#define D3DFVF_XYZ              0x002
#define D3DFVF_XYZRHW           0x004
#define D3DFVF_XYZB1            0x006
#define D3DFVF_XYZB2            0x008
#define D3DFVF_XYZB3            0x00a
#define D3DFVF_XYZB4            0x00c
#define D3DFVF_XYZB5            0x00e

#define D3DFVF_NORMAL           0x010
#define D3DFVF_PSIZE            0x020
#define D3DFVF_DIFFUSE          0x040
#define D3DFVF_SPECULAR         0x080

#define D3DFVF_TEXCOUNT_MASK    0xf00
#define D3DFVF_TEXCOUNT_SHIFT   8
#define D3DFVF_TEX0             0x000
#define D3DFVF_TEX1             0x100
#define D3DFVF_TEX2             0x200
#define D3DFVF_TEX3             0x300
#define D3DFVF_TEX4             0x400
#define D3DFVF_TEX5             0x500
#define D3DFVF_TEX6             0x600
#define D3DFVF_TEX7             0x700
#define D3DFVF_TEX8             0x800

#define D3DFVF_LASTBETA_UBYTE4  0x1000
#define D3DFVF_RESERVED2        0xE000

#define D3DFVF_TEXTUREFORMAT2 0
#define D3DFVF_TEXTUREFORMAT1 3
#define D3DFVF_TEXTUREFORMAT3 1
#define D3DFVF_TEXTUREFORMAT4 2

#define D3DFVF_TEXCOORDSIZE3(CoordIndex) (D3DFVF_TEXTUREFORMAT3 << (CoordIndex*2 + 16))
#define D3DFVF_TEXCOORDSIZE2(CoordIndex) (D3DFVF_TEXTUREFORMAT2)
#define D3DFVF_TEXCOORDSIZE4(CoordIndex) (D3DFVF_TEXTUREFORMAT4 << (CoordIndex*2 + 16))
#define D3DFVF_TEXCOORDSIZE1(CoordIndex) (D3DFVF_TEXTUREFORMAT1 << (CoordIndex*2 + 16))

#define D3DDP_MAXTEXCOORD 8

typedef struct _D3DVIEWPORT8
{
	DWORD X, Y;
	DWORD Width, Height;
	float MinZ, MaxZ;
} D3DVIEWPORT8;

typedef struct _D3DLOCKED_RECT
{
	INT   Pitch;
	void* pBits;
} D3DLOCKED_RECT;

typedef struct _D3DDISPLAYMODE
{
	UINT      Width;
	UINT      Height;
	UINT      RefreshRate;
	D3DFORMAT Format;
} D3DDISPLAYMODE;

#define MAX_DEVICE_IDENTIFIER_STRING 512

typedef struct _D3DADAPTER_IDENTIFIER8
{
	char  Driver[MAX_DEVICE_IDENTIFIER_STRING];
	char  Description[MAX_DEVICE_IDENTIFIER_STRING];
	DWORD DriverVersionLowPart;
	DWORD DriverVersionHighPart;
	DWORD VendorId;
	DWORD DeviceId;
	DWORD SubSysId;
	DWORD Revision;
	GUID  DeviceIdentifier;
	DWORD WHQLLevel;
} D3DADAPTER_IDENTIFIER8;

typedef struct _D3DMATERIAL8
{
	D3DCOLORVALUE Diffuse;
	D3DCOLORVALUE Ambient;
	D3DCOLORVALUE Specular;
	D3DCOLORVALUE Emissive;
	float         Power;
} D3DMATERIAL8;

typedef struct _D3DLIGHT8
{
	D3DLIGHTTYPE  Type;
	D3DCOLORVALUE Diffuse;
	D3DCOLORVALUE Specular;
	D3DCOLORVALUE Ambient;
	D3DVECTOR     Position;
	D3DVECTOR     Direction;
	float         Range;
	float         Falloff;
	float         Attenuation0;
	float         Attenuation1;
	float         Attenuation2;
	float         Theta;
	float         Phi;
} D3DLIGHT8;

typedef struct _D3DPRESENT_PARAMETERS_
{
	UINT                BackBufferWidth;
	UINT                BackBufferHeight;
	D3DFORMAT           BackBufferFormat;
	UINT                BackBufferCount;
	D3DMULTISAMPLE_TYPE MultiSampleType;
	D3DSWAPEFFECT       SwapEffect;
	HWND                hDeviceWindow;
	BOOL                Windowed;
	BOOL                EnableAutoDepthStencil;
	D3DFORMAT           AutoDepthStencilFormat;
	DWORD               Flags;
	UINT                FullScreen_RefreshRateInHz;
	UINT                FullScreen_PresentationInterval;
} D3DPRESENT_PARAMETERS;

// Only ever used as never-dereferenced pointer parameters on stub methods
// (SetCursorProperties/Present/SetPaletteEntries/SetClipStatus/GetCreationParameters/
// DrawRectPatch/DrawTriPatch/SetGammaRamp) - forward declared, not defined.
struct RGNDATA;
struct PALETTEENTRY;
struct D3DGAMMARAMP;
struct D3DCLIPSTATUS8;
struct D3DDEVICE_CREATION_PARAMETERS;
struct D3DRECTPATCH_INFO;
struct D3DTRIPATCH_INFO;
struct D3DRASTER_STATUS;

#define D3DLOCK_READONLY        0x00000010L
#define D3DLOCK_NOSYSLOCK       0x00000800L
#define D3DLOCK_NOOVERWRITE     0x00001000L
#define D3DLOCK_DISCARD         0x00002000L
#define D3DLOCK_NO_DIRTY_UPDATE 0x00008000L

#define D3DCLEAR_TARGET  0x00000001L
#define D3DCLEAR_ZBUFFER 0x00000002L
#define D3DCLEAR_STENCIL 0x00000004L

#define D3DCREATE_FPU_PRESERVE               0x00000002L
#define D3DCREATE_MULTITHREADED              0x00000004L
#define D3DCREATE_PUREDEVICE                 0x00000010L
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING  0x00000020L
#define D3DCREATE_HARDWARE_VERTEXPROCESSING  0x00000040L
#define D3DCREATE_MIXED_VERTEXPROCESSING     0x00000080L
#define D3DCREATE_DISABLE_DRIVER_MANAGEMENT   0x00000100L

#define D3DPRESENTFLAG_LOCKABLE_BACKBUFFER 0x00000001L

#define D3DPRESENT_RATE_DEFAULT   0x00000000
#define D3DPRESENT_RATE_UNLIMITED 0x7fffffff

#define D3DPRESENT_INTERVAL_DEFAULT   0x00000000L
#define D3DPRESENT_INTERVAL_ONE       0x00000001L
#define D3DPRESENT_INTERVAL_TWO       0x00000002L
#define D3DPRESENT_INTERVAL_THREE     0x00000004L
#define D3DPRESENT_INTERVAL_FOUR      0x00000008L
#define D3DPRESENT_INTERVAL_IMMEDIATE 0x80000000L

#define D3D_OK 0L
typedef HRESULT D3DERR;
#define D3DERR_INVALIDCALL       ((HRESULT)-2005530516L)
#define D3DERR_NOTAVAILABLE      ((HRESULT)-2005530518L)
#define D3DERR_DEVICELOST        ((HRESULT)-2005530520L)
#define D3DERR_DEVICENOTRESET    ((HRESULT)-2005530519L)
#define D3DERR_DRIVERINTERNALERROR ((HRESULT)-2005530585L)
#define D3DERR_OUTOFVIDEOMEMORY  ((HRESULT)-2005530508L)

#endif // !_WIN32

#endif // PORTABLE_D3D8TYPES_H
