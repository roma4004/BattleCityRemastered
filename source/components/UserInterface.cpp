#include "components/UserInterface.h"
#include "Options.h"
#include "components/EventSystem.h"
#include "components/LobbyScreen.h"
#include "components/Menu.h"
#include "components/RightSideBar.h"
#include "components/ScoreBoard.h"
#include "components/ServerScreen.h"
#include "components/events/RenderUIEvents.h"
#include "components/managers/FramePerSecondManager.h"
#include "components/managers/GameStateManager.h"
#include "enums/UiIcon.h"
#include <memory>

UserInterface::UserInterface(const std::shared_ptr<EventSystem>& events, const GameStateManager& gameState,
							 const ScoreBoard& scoreBoard, const RightSideBar& rightSideBar,
							 const Options& options, const LobbyScreen& lobbyScreen, const ServerScreen& serverScreen,
							 Menu& menu, const FramePerSecondManager& fps)
	: _events{events}
	, _gameState{gameState}
	, _scoreBoard{scoreBoard}
	, _rightSideBar{rightSideBar}
	, _lobbyScreen{lobbyScreen}
	, _serverScreen{serverScreen}
	, _menu{menu}
	, _fps{fps}
	, _options{options} {}

void UserInterface::Draw() const
{
	if (!_scoreBoard.IsShown())
	{
		DrawPause();
	}

	_scoreBoard.DrawPlate();
	_rightSideBar.Draw();
	_options.Draw();
	_lobbyScreen.Draw();
	_scoreBoard.Draw();
	_serverScreen.Draw();
	_menu.Draw();
	_fps.Draw();

	//NOTE: after the board, not before - its see-through panel would lay the statistics over the plate
	if (_scoreBoard.IsShown())
	{
		DrawPause();
	}
}

void UserInterface::DrawPause() const
{
	if (_gameState.IsPauseShown())
	{
		_events->EmitEvent(RenderPlateEvent{.plate = UiIcon::PlatePause});
	}
}
