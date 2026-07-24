// Phase 4 rung 0 Milestone 16 (native port plan, Draft 41, "a real window
// with real input driving a real pick") - harness-local concrete
// Keyboard/Mouse subclasses driven by GLFW, per Draft 41's own real compile
// spike (a scratch GlfwKeyboard [2 overrides + ring buffer] and GlfwMouse [5
// overrides + MouseIO ring] compiled clean, -fsyntax-only, against the exact
// flags+PCH of this harness family). Mirrors the SAME device-buffer pattern
// Win32Mouse.cpp/Win32DIKeyboard.cpp already establish for the real engine
// (a ring buffer fed by real input delivery, drained one raw event per call
// by getMouseEvent()/getKey()) - these are the direct seeds of the eventual
// production input classes, not a throwaway (Draft 41's own framing).
//
// Two distinct feed paths write into the SAME ring buffers, matching Draft
// 41's own design:
//   1. Real GLFW callbacks (glfwSetKeyCallback/glfwSetMouseButtonCallback/
//      glfwSetCursorPosCallback), registered on glfwGetCurrentContext() -
//      the manual, human-visible payoff mode (real OS -> GLFW callback
//      delivery, the one segment the automated exit criteria can't cover).
//   2. Direct synthetic injection (InjectMouseMove/InjectMouseButton/
//      InjectKey) - bypasses ONLY GLFW's callback *delivery*, landing in the
//      exact same ring buffers real callbacks would fill, so
//      getMouseEvent()/getKey() (and everything downstream: createStreamMessages(),
//      the real translator chain, pickDrawable()) drains and processes them
//      identically either way. This is what the automated, CI-safe exit
//      criteria (Milestone 16's own must-pass bar) inject through.
//
// TheSuperHackers @port Milestone 16 (native port plan, Draft 41): real,
// minimal concrete Keyboard/Mouse subclasses replacing TheKeyboard's
// previously-absent construction (Gotcha 2: a real Mouse unconditionally
// dereferences TheKeyboard->getModifierFlags()) and MouseDummy.

#pragma once

#include "GameClient/Keyboard.h"
#include "GameClient/Mouse.h"

struct GLFWwindow;

// ---- MapGlfwKeyToDik: a ~60-entry GLFW-keycode -> DIK-based KeyDefType
// mapping table (KeyDefs.h self-defines the DIK_* constants portably, no
// DirectInput header needed, per Draft 41's own finding). Covers ESC,
// modifiers, arrows, function keys, letters, digits, and common punctuation
// - enough for this milestone's own explicit scope (not exhaustive; e.g. no
// numpad, no non-US layout keys, matching Draft 41's own "don't try to be
// exhaustive" guidance). Returns KEY_NONE for anything unmapped. ----
KeyDefType MapGlfwKeyToDik(int glfwKey);

// ---- GlfwKeyboard: Keyboard's 2 pure virtuals (getCapsState(), getKey())
// plus a real ring buffer fed by either real GLFW key callbacks or direct
// synthetic injection. ----
class GlfwKeyboard : public Keyboard
{
public:
	GlfwKeyboard();
	virtual ~GlfwKeyboard() override {}

	virtual Bool getCapsState() override;

	// Real GLFW callback entry point (called from the free-function GLFW
	// callback registered in RegisterGlfwCallbacks() below).
	void OnGlfwKey(int glfwKey, int action);

	// Synthetic injection entry point - CI-safe exit-criteria path, bypasses
	// GLFW callback delivery only, lands in the SAME ring buffer.
	void InjectKey(KeyDefType dik, Bool down);

protected:
	virtual void getKey(KeyboardIO *key) override;

private:
	enum { RING_SIZE = 64 };
	KeyboardIO m_ring[RING_SIZE];
	Int m_head;
	Int m_count;

	void pushEvent(KeyDefType dik, Bool down);
};

// ---- GlfwMouse: Mouse's 5 pure virtuals (initCursorResources(), setCursor(),
// capture(), releaseCapture(), getMouseEvent()) plus a real MouseIO ring
// buffer fed by either real GLFW callbacks or direct synthetic injection. ----
class GlfwMouse : public Mouse
{
public:
	enum { LEFT_BUTTON = 0, RIGHT_BUTTON = 1, MIDDLE_BUTTON = 2 };

	GlfwMouse();
	virtual ~GlfwMouse() override {}

	virtual void init() override;
	virtual void initCursorResources() override {}
	virtual void setCursor(MouseCursor cursor) override;

	// Real GLFW callback entry points.
	void OnGlfwCursorPos(double x, double y);
	void OnGlfwMouseButton(int glfwButton, int action);

	// Synthetic injection entry points - CI-safe exit-criteria path.
	void InjectMouseMove(Int x, Int y);
	void InjectMouseButton(Int whichButton /* 0=left,1=right,2=middle */, Bool down);

protected:
	virtual void capture() override {}
	virtual void releaseCapture() override {}
	virtual UnsignedByte getMouseEvent(MouseIO *result, Bool flush) override;

private:
	enum { RING_SIZE = 64 };
	MouseIO m_ring[RING_SIZE];
	Int m_head;
	Int m_count;
	ICoord2D m_lastPos;

	void pushMoveEvent(Int x, Int y);
	void pushButtonEvent(Int whichButton, Bool down);
};

// ---- RegisterGlfwCallbacks: wires glfwSetKeyCallback/
// glfwSetMouseButtonCallback/glfwSetCursorPosCallback onto the given window
// (the harness passes glfwGetCurrentContext(), per Draft 41's own finding
// that this gives the live window handle with zero wrapper modification),
// routing to the given GlfwKeyboard/GlfwMouse instances via static
// (harness-lifetime, single-instance) pointers. Manual-mode-only wiring -
// the automated exit criteria never call this, injecting directly instead.
void RegisterGlfwCallbacks(GLFWwindow *window, GlfwKeyboard *keyboard, GlfwMouse *mouse);
