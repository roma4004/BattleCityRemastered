#pragma once

#include "enums/PlayerSlot.h"

// Each event needs a type of its own - under type-based dispatch two bare bools would collide

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

//TODO: nothing in the codebase listens to this - wire it up or drop it
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
