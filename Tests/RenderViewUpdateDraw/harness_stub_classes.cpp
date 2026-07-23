// Phase 5(a) Milestone 11 RETRY (native port plan, rung 3b-ii-a, Draft 35) -
// a standalone translation unit for harness_stub_classes.h, whose five stub
// subclasses are otherwise fully inline (header-only). This TU exists only
// so the header is compiled and vtable-checked on its own, independent of
// main.cpp's own PreRTS.h-driven include order - a cheap sanity net, not a
// source of any new symbols (nothing here is referenced from elsewhere).
#include "PreRTS.h"
#include "harness_stub_classes.h"
