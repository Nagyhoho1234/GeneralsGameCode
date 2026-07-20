// Mirrors the subset of WW3D2/shader.h's ShaderClass ShaderBits vocabulary
// this spike exercises. Confirmed byte-identical between the Generals and
// GeneralsMD trees (both define MASK_POSTDETAILCOLORFUNC = 15<<20 etc. with
// the same shifts) - GeneralsMD only adds enum values inside existing bit
// fields, so this subset is safe to hand-copy for a standalone spike rather
// than pulling in the real header's full dependency chain (AsciiString,
// W3D types, ...).
#pragma once

namespace ShaderBitsSpike {

enum Shift {
    SHIFT_DEPTHCOMPARE = 0,
    SHIFT_DEPTHMASK = 3,
    SHIFT_COLORMASK = 4,
    SHIFT_DSTBLEND = 5,
    SHIFT_FOG = 8,
    SHIFT_PRIGRADIENT = 10,
    SHIFT_SECGRADIENT = 13,
    SHIFT_SRCBLEND = 14,
    SHIFT_TEXTURING = 16,
    SHIFT_NPATCHENABLE = 17,
    SHIFT_ALPHATEST = 18,
    SHIFT_CULLMODE = 19,
};

enum Mask {
    MASK_DSTBLEND = (7 << SHIFT_DSTBLEND),
    MASK_PRIGRADIENT = (7 << SHIFT_PRIGRADIENT),
    MASK_SRCBLEND = (3 << SHIFT_SRCBLEND),
    MASK_TEXTURING = (1 << SHIFT_TEXTURING),
    MASK_ALPHATEST = (1 << SHIFT_ALPHATEST),
};

// DstBlendFuncType
enum { DSTBLEND_ZERO = 0, DSTBLEND_ONE, DSTBLEND_SRC_COLOR, DSTBLEND_ONE_MINUS_SRC_COLOR, DSTBLEND_SRC_ALPHA, DSTBLEND_ONE_MINUS_SRC_ALPHA };
// SrcBlendFuncType
enum { SRCBLEND_ZERO = 0, SRCBLEND_ONE, SRCBLEND_SRC_ALPHA, SRCBLEND_ONE_MINUS_SRC_ALPHA };
// PriGradientType
enum { GRADIENT_DISABLE = 0, GRADIENT_MODULATE, GRADIENT_ADD, GRADIENT_BUMPENVMAP, GRADIENT_BUMPENVMAPLUMINANCE, GRADIENT_MODULATE2X };
// AlphaTestType
enum { ALPHATEST_DISABLE = 0, ALPHATEST_ENABLE };
// TexturingType
enum { TEXTURING_DISABLE = 0, TEXTURING_ENABLE };

// Two representative SHADE_CNST-style presets, chosen to exercise opposite
// ends of the vocabulary the desk-check flagged as the real remaining risk:
// resource/semantics matching, not combiner emulation.

// Opaque textured modulate - the common terrain/object case.
constexpr unsigned int OPAQUE_MODULATE =
    (TEXTURING_ENABLE << SHIFT_TEXTURING) |
    (GRADIENT_MODULATE << SHIFT_PRIGRADIENT) |
    (ALPHATEST_DISABLE << SHIFT_ALPHATEST) |
    (SRCBLEND_ONE << SHIFT_SRCBLEND) |
    (DSTBLEND_ZERO << SHIFT_DSTBLEND);

// Alpha-blended additive with alpha-test - the common particle/effect case.
// No glAlphaFunc exists in GL 3.3 core; ALPHATEST_ENABLE must be emulated via
// fragment-shader `discard`, which is exactly the semantics worth
// prototyping (fable review flagged this as spike design, not a gap).
constexpr unsigned int ALPHATEST_ADDITIVE =
    (TEXTURING_ENABLE << SHIFT_TEXTURING) |
    (GRADIENT_MODULATE << SHIFT_PRIGRADIENT) |
    (ALPHATEST_ENABLE << SHIFT_ALPHATEST) |
    (SRCBLEND_SRC_ALPHA << SHIFT_SRCBLEND) |
    (DSTBLEND_ONE << SHIFT_DSTBLEND);

inline int get(unsigned int bits, Shift shift, unsigned int mask) { return (bits & mask) >> shift; }

} // namespace ShaderBitsSpike
