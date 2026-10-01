#include "components/LobbyScreen.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/UiTable.h"
#include "enums/GameMode.h"
#include "enums/DisconnectReason.h"
#include "enums/GameState.h"
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr unsigned int kTextColor{0xffffffffu};
}//namespace

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

void LobbyScreen::OnDrawUserInterface(const DrawUserInterfaceEvent&) const { Draw(); }

void LobbyScreen::Display(const bool isDisplayed)
{
	_drawSub = isDisplayed ? _events->AddListener(this, &LobbyScreen::OnDrawUserInterface) : EventSubscription{};
}

void LobbyScreen::Draw() const
{
	_events->EmitEvent(RenderMenuBackgroundEvent{});

	const auto line = [](std::string text)
	{
		return UiRow{.cells = {TextCell(std::move(text), kTextColor, UiAlign::Centered)}};
	};

	UiTable lines{};
	if (_isServerFull)
	{
		lines.rows = {line("MATCH IN PROGRESS"), line("WAITING FOR A FREE SEAT")};
	}
	else
	{
		lines.rows = {line(IsHost(_gameConfig.gameMode) ? "WAITING FOR PLAYER" : "CONNECTING TO HOST")};
	}

	lines.rows.push_back(line("PRESS M FOR MENU"));

	std::vector<UiTable> tables{};
	tables.push_back(std::move(lines));
	_events->EmitEvent(RenderPanelTablesEvent{.tables = std::move(tables)});
}
