#include "components/LobbyScreen.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/UiTable.h"
#include "enums/GameMode.h"
#include "enums/DisconnectReason.h"
#include "enums/GameState.h"
#include "enums/MatchRules.h"
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
	_subs.push_back(_events->AddListener(this, &LobbyScreen::OnMenuShown));
	_subs.push_back(_events->AddListener(this, &LobbyScreen::OnServerScreenShown));
	_subs.push_back(_events->AddListener(this, &LobbyScreen::OnRefusedOrLost));
	_subs.push_back(_events->AddListener(this, &LobbyScreen::OnConnectedToHost));
	_subs.push_back(_events->AddListener(this, &LobbyScreen::OnSlotAssigned));
}

void LobbyScreen::OnGameStateChangedTo(const GameStateChangedToEvent& event)
{
	_isLobby = event.state == GameState::Lobby;
	Display();
}

//NOTE: the menu opens on top - two panels stacked would darken each other
void LobbyScreen::OnMenuShown(const MenuShownEvent& event)
{
	_isMenuShown = event.isShown;
	Display();
}

//NOTE: so does the server screen
void LobbyScreen::OnServerScreenShown(const ServerScreenShownEvent& event)
{
	_isServerScreenShown = event.isShown;
	Display();
}

void LobbyScreen::OnRefusedOrLost(const ClientInDisconnectEvent& event)
{
	_isServerFull = event.reason == DisconnectReason::ServerFull;
	_match.reset();
}

void LobbyScreen::OnConnectedToHost(const ClientConnectedToHostEvent&) { _isServerFull = false; }

void LobbyScreen::OnSlotAssigned(const PlayerSlotAssignedEvent& event) { _match = event.match; }

void LobbyScreen::OnDrawUserInterface(const DrawUserInterfaceEvent&) const { Draw(); }

void LobbyScreen::Display()
{
	const bool isShown{_isLobby && !_isMenuShown && !_isServerScreenShown};
	_drawSub = isShown ? _events->AddListener(this, &LobbyScreen::OnDrawUserInterface) : EventSubscription{};
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

	if (_match)
	{
		const bool isClassic{_match->rules == MatchRules::Classic};
		lines.rows.push_back(line(std::string{isClassic ? "CLASSIC" : "FREE FOR ALL"} + ", "
								  + std::to_string(_match->seats) + " SEATS, MAP " + _match->map));
		if (isClassic)
		{
			lines.rows.push_back(line(std::to_string(_match->enemiesAtOnce) + " ENEMIES AT ONCE"));
		}
	}

	lines.rows.push_back(line("PRESS M FOR MENU"));

	std::vector<UiTable> tables{};
	tables.push_back(std::move(lines));
	_events->EmitEvent(RenderPanelTablesEvent{.tables = std::move(tables)});
}
