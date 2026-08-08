#pragma once

#include <string>

// The zero-arg ones need a unique empty tag struct for the same reason as CoreLifecycleEvents.h.
// The bare-bool ones need a unique single-field struct so two unrelated bool events don't collide
// under type-based dispatch.

struct MenuReleasedEvent {};

struct PauseReleasedEvent {};

struct TabReleasedEvent {};

struct PauseStatusEvent
{
	bool isPaused;
};

struct ServerSendPauseStatusEvent
{
	bool isPaused;
};

struct ClientSendPauseStatusEvent
{
	bool isPaused;
};

struct ServerReceivePauseReleasedEvent
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


struct ServerReceiveMoveUpEvent
{
	bool isPressed;
};

struct ServerReceiveMoveDownEvent
{
	bool isPressed;
};

struct ServerReceiveMoveLeftEvent
{
	bool isPressed;
};

struct ServerReceiveMoveRightEvent
{
	bool isPressed;
};

struct ServerReceiveFireEvent
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
	std::string controllerTag;
	GamepadButton button;
	bool isPressed;
};
