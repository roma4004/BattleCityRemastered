#pragma once

#include "enums/PlayerSlot.h"

// The zero-arg ones need a unique empty tag struct for the same reason as CoreLifecycleEvents.h.
// The bare-bool ones need a unique single-field struct so two unrelated bool events don't collide
// under type-based dispatch.

struct MenuReleasedEvent {};

struct PauseReleasedEvent {};

struct TabReleasedEvent {};

struct SetPauseEvent
{
	bool isPaused;
};

struct PauseStatusEvent
{
	bool isPaused;
};

struct PauseRequestedEvent
{
	bool isPaused;
};


struct MoveUpEvent
{
	bool isPressed;
};

struct MoveDownEvent
{
	bool isPressed;
};

struct MoveLeftEvent
{
	bool isPressed;
};

struct MoveRightEvent
{
	bool isPressed;
};

struct FireEvent
{
	bool isPressed;
};

//NOTE: maps to the literal event name "Reset_" (UserInput.cpp:208) - no matching listener found
//anywhere in the codebase; kept, not deleted, needs author follow-up.
struct ResetKeyEvent
{
	bool isPressed;
};

struct EnterEvent
{
	bool isPressed;
};

enum class GamepadButton : char8_t
{
	B,
	X,
	Y
};

struct GamepadButtonEvent
{
	PlayerSlot controllerSlot;
	GamepadButton button;
	bool isPressed;
};
