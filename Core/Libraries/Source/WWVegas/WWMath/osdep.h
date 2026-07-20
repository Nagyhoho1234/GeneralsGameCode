// This header has never existed in any public release of this codebase -
// `#ifdef _UNIX / #include "osdep.h"` in vector3.h and matrix3d.h dates back
// to Westwood's original 2003 EA source release (confirmed: present in the
// very first commit of this repo's history), and almost certainly referred
// to an internal Westwood build-system header shared across their W3D-engine
// titles (Renegade, Emperor: Battle for Dune, ...) that was never part of
// the Generals/Zero Hour public release.
//
// Nothing in vector3.h or matrix3d.h actually uses a symbol from this header
// after including it - confirmed empirically by building both files with an
// empty stub and getting no further errors related to missing declarations.
// Left empty rather than invented, pending any future need discovered by
// actually trying to build/run on the target platform (docs/native-port-plan.md
// Phase 1).
#pragma once
