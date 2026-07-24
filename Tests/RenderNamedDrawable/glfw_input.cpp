// Phase 4 rung 0 Milestone 16 (native port plan, Draft 41) - see
// glfw_input.h's own header comment for the full design rationale.
#include "PreRTS.h"

#include "glfw_input.h"

#include <GLFW/glfw3.h>

#include <cstring>

// ---- MapGlfwKeyToDik ----------------------------------------------------
KeyDefType MapGlfwKeyToDik(int glfwKey)
{
	switch (glfwKey)
	{
		case GLFW_KEY_ESCAPE:        return KEY_ESC;
		case GLFW_KEY_ENTER:         return KEY_ENTER;
		case GLFW_KEY_TAB:           return KEY_TAB;
		case GLFW_KEY_BACKSPACE:     return KEY_BACKSPACE;
		case GLFW_KEY_SPACE:         return KEY_SPACE;
		case GLFW_KEY_CAPS_LOCK:     return KEY_CAPS;

		case GLFW_KEY_UP:            return KEY_UP;
		case GLFW_KEY_DOWN:          return KEY_DOWN;
		case GLFW_KEY_LEFT:          return KEY_LEFT;
		case GLFW_KEY_RIGHT:         return KEY_RIGHT;

		case GLFW_KEY_HOME:          return KEY_HOME;
		case GLFW_KEY_END:           return KEY_END;
		case GLFW_KEY_PAGE_UP:       return KEY_PGUP;
		case GLFW_KEY_PAGE_DOWN:     return KEY_PGDN;
		case GLFW_KEY_INSERT:        return KEY_INS;
		case GLFW_KEY_DELETE:        return KEY_DEL;

		case GLFW_KEY_LEFT_SHIFT:    return KEY_LSHIFT;
		case GLFW_KEY_RIGHT_SHIFT:   return KEY_RSHIFT;
		case GLFW_KEY_LEFT_CONTROL:  return KEY_LCTRL;
		case GLFW_KEY_RIGHT_CONTROL: return KEY_RCTRL;
		case GLFW_KEY_LEFT_ALT:      return KEY_LALT;
		case GLFW_KEY_RIGHT_ALT:     return KEY_RALT;

		case GLFW_KEY_F1:            return KEY_F1;
		case GLFW_KEY_F2:            return KEY_F2;
		case GLFW_KEY_F3:            return KEY_F3;
		case GLFW_KEY_F4:            return KEY_F4;
		case GLFW_KEY_F5:            return KEY_F5;
		case GLFW_KEY_F6:            return KEY_F6;
		case GLFW_KEY_F7:            return KEY_F7;
		case GLFW_KEY_F8:            return KEY_F8;
		case GLFW_KEY_F9:            return KEY_F9;
		case GLFW_KEY_F10:           return KEY_F10;
		case GLFW_KEY_F11:           return KEY_F11;
		case GLFW_KEY_F12:           return KEY_F12;

		case GLFW_KEY_A: return KEY_A; case GLFW_KEY_B: return KEY_B;
		case GLFW_KEY_C: return KEY_C; case GLFW_KEY_D: return KEY_D;
		case GLFW_KEY_E: return KEY_E; case GLFW_KEY_F: return KEY_F;
		case GLFW_KEY_G: return KEY_G; case GLFW_KEY_H: return KEY_H;
		case GLFW_KEY_I: return KEY_I; case GLFW_KEY_J: return KEY_J;
		case GLFW_KEY_K: return KEY_K; case GLFW_KEY_L: return KEY_L;
		case GLFW_KEY_M: return KEY_M; case GLFW_KEY_N: return KEY_N;
		case GLFW_KEY_O: return KEY_O; case GLFW_KEY_P: return KEY_P;
		case GLFW_KEY_Q: return KEY_Q; case GLFW_KEY_R: return KEY_R;
		case GLFW_KEY_S: return KEY_S; case GLFW_KEY_T: return KEY_T;
		case GLFW_KEY_U: return KEY_U; case GLFW_KEY_V: return KEY_V;
		case GLFW_KEY_W: return KEY_W; case GLFW_KEY_X: return KEY_X;
		case GLFW_KEY_Y: return KEY_Y; case GLFW_KEY_Z: return KEY_Z;

		case GLFW_KEY_0: return KEY_0; case GLFW_KEY_1: return KEY_1;
		case GLFW_KEY_2: return KEY_2; case GLFW_KEY_3: return KEY_3;
		case GLFW_KEY_4: return KEY_4; case GLFW_KEY_5: return KEY_5;
		case GLFW_KEY_6: return KEY_6; case GLFW_KEY_7: return KEY_7;
		case GLFW_KEY_8: return KEY_8; case GLFW_KEY_9: return KEY_9;

		case GLFW_KEY_MINUS:         return KEY_MINUS;
		case GLFW_KEY_EQUAL:         return KEY_EQUAL;
		case GLFW_KEY_LEFT_BRACKET:  return KEY_LBRACKET;
		case GLFW_KEY_RIGHT_BRACKET: return KEY_RBRACKET;
		case GLFW_KEY_SEMICOLON:     return KEY_SEMICOLON;
		case GLFW_KEY_APOSTROPHE:    return KEY_APOSTROPHE;
		case GLFW_KEY_GRAVE_ACCENT:  return KEY_TICK;
		case GLFW_KEY_BACKSLASH:     return KEY_BACKSLASH;
		case GLFW_KEY_COMMA:         return KEY_COMMA;
		case GLFW_KEY_PERIOD:        return KEY_PERIOD;
		case GLFW_KEY_SLASH:         return KEY_SLASH;

		default: return KEY_NONE;
	}
}

// ---- GlfwKeyboard ---------------------------------------------------------
GlfwKeyboard::GlfwKeyboard() : m_head(0), m_count(0)
{
	memset(m_ring, 0, sizeof(m_ring));
}

Bool GlfwKeyboard::getCapsState()
{
	// Real caps-lock LOCK state (not just "is the key currently held") needs
	// GLFW_LOCK_KEY_MODS input mode support, which the current window setup
	// does not enable - out of this milestone's own explicit scope (ESC/
	// modifiers/arrows are enough, per Draft 41). A fixed FALSE is a safe,
	// real default: Keyboard::resetKeys() is the only base-class caller, and
	// it only affects the KEY_STATE_CAPSLOCK modifier bit, not any of this
	// milestone's own exit criteria.
	return FALSE;
}

void GlfwKeyboard::pushEvent(KeyDefType dik, Bool down)
{
	if (m_count >= RING_SIZE)
		return; // buffer full, drop (matches Win32Mouse's own "event will be lost" precedent)

	Int index = (m_head + m_count) % RING_SIZE;
	KeyboardIO &io = m_ring[index];
	io.key = static_cast<UnsignedByte>(dik);
	io.status = KeyboardIO::STATUS_UNUSED;
	io.state = down ? KEY_STATE_DOWN : KEY_STATE_UP;
	io.keyDownTimeMsec = timeGetTime();

	++m_count;
}

void GlfwKeyboard::OnGlfwKey(int glfwKey, int action)
{
	if (action != GLFW_PRESS && action != GLFW_RELEASE)
		return; // ignore GLFW_REPEAT - Keyboard::checkKeyRepeat() already synthesizes repeats internally

	KeyDefType dik = MapGlfwKeyToDik(glfwKey);
	if (dik == KEY_NONE)
		return;

	pushEvent(dik, action == GLFW_PRESS);
}

void GlfwKeyboard::InjectKey(KeyDefType dik, Bool down)
{
	pushEvent(dik, down);
}

void GlfwKeyboard::getKey(KeyboardIO *key)
{
	if (m_count == 0)
	{
		key->key = KEY_NONE;
		return;
	}

	*key = m_ring[m_head];
	m_head = (m_head + 1) % RING_SIZE;
	--m_count;
}

// ---- GlfwMouse -------------------------------------------------------------
GlfwMouse::GlfwMouse() : m_head(0), m_count(0)
{
	memset(m_ring, 0, sizeof(m_ring));
	m_lastPos.x = 0;
	m_lastPos.y = 0;
}

void GlfwMouse::init()
{
	Mouse::init();

	// Real GLFW cursor-position callbacks (and this milestone's own
	// synthetic injection) always report absolute window-local coordinates,
	// never relative deltas - matches Win32Mouse::init()'s own established
	// rationale for the identical m_inputMovesAbsolute flip (Win32Mouse.cpp).
	m_inputMovesAbsolute = TRUE;
}

void GlfwMouse::setCursor(MouseCursor cursor)
{
	// Real base body (updates cursor text state) plus the real cursor-image
	// bookkeeping every concrete Mouse subclass is required to do itself
	// (Win32Mouse::setCursor's own established pattern) - this milestone's
	// own exit criteria read getMouseCursor() back as a real, observable
	// side effect of the real SelectionTranslator::onMouseoverDrawableHint()
	// call chain (TheMouse->setCursor(...)), so m_currentCursor must
	// actually update here, not just in the (do-nothing) base.
	Mouse::setCursor(cursor);
	m_currentCursor = cursor;
}

void GlfwMouse::pushMoveEvent(Int x, Int y)
{
	m_lastPos.x = x;
	m_lastPos.y = y;

	if (m_count >= RING_SIZE)
		return;

	Int index = (m_head + m_count) % RING_SIZE;
	MouseIO &io = m_ring[index];
	memset(&io, 0, sizeof(io));
	io.pos.x = x;
	io.pos.y = y;
	io.time = timeGetTime();
	io.leftState = io.rightState = io.middleState = MBS_None;

	++m_count;
}

void GlfwMouse::pushButtonEvent(Int whichButton, Bool down)
{
	if (m_count >= RING_SIZE)
		return;

	Int index = (m_head + m_count) % RING_SIZE;
	MouseIO &io = m_ring[index];
	memset(&io, 0, sizeof(io));
	io.pos = m_lastPos;
	io.time = timeGetTime();
	io.leftState = io.rightState = io.middleState = MBS_None;

	MouseButtonState state = down ? MBS_Down : MBS_Up;
	switch (whichButton)
	{
		case LEFT_BUTTON:   io.leftState = state; break;
		case RIGHT_BUTTON:  io.rightState = state; break;
		case MIDDLE_BUTTON: io.middleState = state; break;
		default: return;
	}

	++m_count;
}

void GlfwMouse::OnGlfwCursorPos(double x, double y)
{
	pushMoveEvent(static_cast<Int>(x), static_cast<Int>(y));
}

void GlfwMouse::OnGlfwMouseButton(int glfwButton, int action)
{
	if (action != GLFW_PRESS && action != GLFW_RELEASE)
		return;

	Int whichButton;
	switch (glfwButton)
	{
		case GLFW_MOUSE_BUTTON_LEFT:   whichButton = LEFT_BUTTON; break;
		case GLFW_MOUSE_BUTTON_RIGHT:  whichButton = RIGHT_BUTTON; break;
		case GLFW_MOUSE_BUTTON_MIDDLE: whichButton = MIDDLE_BUTTON; break;
		default: return;
	}

	pushButtonEvent(whichButton, action == GLFW_PRESS);
}

void GlfwMouse::InjectMouseMove(Int x, Int y)
{
	pushMoveEvent(x, y);
}

void GlfwMouse::InjectMouseButton(Int whichButton, Bool down)
{
	pushButtonEvent(whichButton, down);
}

UnsignedByte GlfwMouse::getMouseEvent(MouseIO *result, Bool /*flush*/)
{
	if (m_count == 0)
		return MOUSE_NONE;

	*result = m_ring[m_head];
	m_head = (m_head + 1) % RING_SIZE;
	--m_count;

	return MOUSE_OK;
}

// ---- RegisterGlfwCallbacks --------------------------------------------------
namespace
{
	GlfwKeyboard *g_HarnessKeyboard = nullptr;
	GlfwMouse *g_HarnessMouse = nullptr;

	void GlfwKeyCallback(GLFWwindow * /*window*/, int key, int /*scancode*/, int action, int /*mods*/)
	{
		if (g_HarnessKeyboard)
			g_HarnessKeyboard->OnGlfwKey(key, action);
	}

	void GlfwMouseButtonCallback(GLFWwindow * /*window*/, int button, int action, int /*mods*/)
	{
		if (g_HarnessMouse)
			g_HarnessMouse->OnGlfwMouseButton(button, action);
	}

	void GlfwCursorPosCallback(GLFWwindow * /*window*/, double x, double y)
	{
		if (g_HarnessMouse)
			g_HarnessMouse->OnGlfwCursorPos(x, y);
	}
}

void RegisterGlfwCallbacks(GLFWwindow *window, GlfwKeyboard *keyboard, GlfwMouse *mouse)
{
	if (!window)
		return;

	g_HarnessKeyboard = keyboard;
	g_HarnessMouse = mouse;

	glfwSetKeyCallback(window, GlfwKeyCallback);
	glfwSetMouseButtonCallback(window, GlfwMouseButtonCallback);
	glfwSetCursorPosCallback(window, GlfwCursorPosCallback);
}
