#pragma once

#include "enums/PlayerSlot.h"
#include <cstddef>
#include <string>

// Each event needs a type of its own - under type-based dispatch two bare bools would collide

struct MenuReleasedEvent {};

struct PauseReleasedEvent {};

//NOTE: Shift+Tab swaps the second pair of seats
struct TabReleasedEvent
{
	bool isSecondPair{};
};

struct SetPauseEvent
{
	bool isPaused;
};

struct PauseStatusEvent
{
	bool isPaused;
};

//NOTE: a client joining a running match is catching up, or a player left it and the rest have not said what
//to do - the match stands still whatever the players ask for
struct MatchHoldChangedEvent
{
	bool isHeld;
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

struct EnterEvent
{
	bool isPressed;
};

struct TextTypedEvent
{
	std::string text;
};

//NOTE: apart from typing - a whole address goes to the row it belongs to
struct TextPastedEvent
{
	std::string text;
};

//NOTE: a word lies between separators - an octet of an address
enum class TextKey : char8_t
{
	Erase,
	EraseRight,
	CaretLeft,
	CaretRight,
	WordLeft,
	WordRight,
	NextField,
	PreviousField
};

struct TextKeyEvent
{
	TextKey key;
};

//NOTE: the row of PanelRowsPlacedEvent a click landed on
struct PanelRowClickedEvent
{
	std::size_t row;
	//NOTE: the symbol of the row's text under it, counted at the panel's own size
	std::size_t symbol;
};

//NOTE: and the one the pointer moved onto
struct PanelRowHoveredEvent
{
	std::size_t row;
};

//NOTE: Esc while typing leaves the field, not the game
struct TextInputCancelledEvent {};

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
