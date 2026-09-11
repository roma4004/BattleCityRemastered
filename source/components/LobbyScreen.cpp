#include "components/LobbyScreen.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/RenderUIEvents.h"
#include "enums/GameMode.h"
#include "enums/DisconnectReason.h"
#include "enums/GameState.h"
#include <string>
#include <vector>

LobbyScreen::LobbyScreen(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _events{events}
	, _gameConfig{gameConfig}
{
	Subscribe();
}

void LobbyScreen::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &LobbyScreen::OnGameStateChangedTo));
	_subs.push_back(_events->AddListener(this, &LobbyScreen::OnMenuShowed));
	_subs.push_back(_events->AddListener(this, &LobbyScreen::OnRefusedOrLost));
	_subs.push_back(_events->AddListener(this, &LobbyScreen::OnConnectedToHost));
}

void LobbyScreen::OnGameStateChangedTo(const GameStateChangedToEvent& event)
{
	_isLobby = event.state == GameState::Lobby;
	Display(_isLobby && !_isMenuShown);
}

//NOTE: the menu opens on top - two panels stacked would darken each other
void LobbyScreen::OnMenuShowed(const MenuShowedEvent& event)
{
	_isMenuShown = event.isShown;
	Display(_isLobby && !_isMenuShown);
}

void LobbyScreen::OnRefusedOrLost(const ClientInDisconnectEvent& event)
{
	_isServerFull = event.reason == DisconnectReason::ServerFull;
}

void LobbyScreen::OnConnectedToHost(const ClientConnectedToHostEvent&) { _isServerFull = false; }

void LobbyScreen::OnDrawUserInterface(const DrawUserInterfaceEvent&) { Draw(); }

void LobbyScreen::Display(const bool isDisplayed)
{
	_drawSub = isDisplayed ? _events->AddListener(this, &LobbyScreen::OnDrawUserInterface) : EventSubscription{};
}

void LobbyScreen::Draw() const
{
	_events->EmitEvent(RenderMenuBackgroundEvent{.pos = _pos});

	constexpr unsigned int color{0xffffffffu};
	constexpr int lineStep{30};
	std::vector<TextBlockLine> lines;
	if (_isServerFull)
	{
		lines.push_back(TextBlockLine{.color = color, .text = "MATCH IN PROGRESS"});
		lines.push_back(TextBlockLine{.color = color, .text = "WAITING FOR A FREE SEAT"});
	}
	else
	{
		const std::string waitingFor =
				IsHost(_gameConfig.gameMode) ? "WAITING FOR PLAYER" : "CONNECTING TO HOST";
		lines.push_back(TextBlockLine{.color = color, .text = waitingFor});
	}

	lines.push_back(TextBlockLine{.color = color, .text = "PRESS M FOR MENU"});

	_events->EmitEvent(RenderMenuTextBlockEvent{.menuPos = _pos,
												.lineHeight = lineStep,
												.align = TextBlockAlign::CenteredInPanel,
												.lines = std::move(lines)});
}
