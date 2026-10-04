#include "components/LobbyScreen.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/UiTable.h"
#include "enums/Absence.h"
#include "enums/GameMode.h"
#include "enums/DisconnectReason.h"
#include "enums/GameState.h"
#include "enums/InputChannel.h"
#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
constexpr unsigned int kTextColor{0xffffffffu};

UiRow Line(std::string text) { return UiRow{.cells = {TextCell(std::move(text), kTextColor, UiAlign::Centered)}}; }

[[nodiscard]] std::string_view Label(const AbsenceChoice choice, const bool isAnyoneOut)
{
	switch (choice)
	{
		case AbsenceChoice::Continue:
			return isAnyoneOut ? "PLAY ON WITHOUT" : "CONTINUE";
		case AbsenceChoice::Bot:
			return "HAND OVER TO BOT";
	}

	return "";
}
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
	_subs.push_back(_events->AddListener(this, &LobbyScreen::OnAbsenceChanged));
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

//NOTE: the pick goes back to the top - the choices left may be fewer
void LobbyScreen::OnAbsenceChanged(const AbsenceChangedEvent& event)
{
	_absence = event.seats;
	_pick = 0u;
	Display();
}

void LobbyScreen::OnDrawUserInterface(const DrawUserInterfaceEvent&) const { Draw(); }

//NOTE: a click answers at once, as in the server screen
void LobbyScreen::OnRowClicked(const PanelRowClickedEvent& event)
{
	if (event.row < Choices().size())
	{
		Choose(event.row);
	}
}

void LobbyScreen::OnRowHovered(const PanelRowHoveredEvent& event)
{
	if (event.row < Choices().size())
	{
		_pick = event.row;
	}
}

void LobbyScreen::OnEnter(const EnterEvent& event) { Confirm(event.isPressed); }

void LobbyScreen::OnFire(const FireEvent& event) { Confirm(event.isPressed); }

void LobbyScreen::OnPadUp(const MoveUpEvent& event)
{
	if (event.isPressed && _pick > 0u)
	{
		--_pick;
	}
}

void LobbyScreen::OnPadDown(const MoveDownEvent& event)
{
	if (event.isPressed && _pick + 1u < Choices().size())
	{
		++_pick;
	}
}

//NOTE: the lobby goes first - a match torn down may tell the phase before it clears who left
bool LobbyScreen::IsAsking() const
{
	return !_isLobby && std::ranges::any_of(_absence, [](const Absence absence) { return absence != Absence::None; });
}

//NOTE: no wait among them - the panel standing is the wait
std::vector<AbsenceChoice> LobbyScreen::Choices() const
{
	if (std::ranges::contains(_absence, Absence::Left))
	{
		return {AbsenceChoice::Continue, AbsenceChoice::Bot};
	}

	return {AbsenceChoice::Continue};
}

void LobbyScreen::Confirm(const bool isPressed)
{
	if (isPressed)
	{
		_isConfirmHeld = true;

		return;
	}

	if (_isConfirmHeld)
	{
		_isConfirmHeld = false;
		Choose(_pick);
	}
}

//NOTE: the panel stays as it is until the server answers - somebody else may have answered first
void LobbyScreen::Choose(const std::size_t pick) const
{
	_events->EmitEvent(AbsenceChosenEvent{.choice = Choices()[pick]});
}

void LobbyScreen::Display()
{
	const bool isCovered{_isMenuShown || _isServerScreenShown};
	const bool isShown{(_isLobby || IsAsking()) && !isCovered};
	_drawSub = isShown ? _events->AddListener(this, &LobbyScreen::OnDrawUserInterface) : EventSubscription{};

	const bool isTakingAnswers{IsAsking() && !isCovered};
	if (isTakingAnswers == !_choiceSubs.empty())
	{
		return;
	}

	_choiceSubs.clear();
	_isConfirmHeld = false;
	if (!isTakingAnswers)
	{
		return;
	}

	_choiceSubs.push_back(_events->AddListener(this, &LobbyScreen::OnRowClicked));
	_choiceSubs.push_back(_events->AddListener(this, &LobbyScreen::OnRowHovered));
	_choiceSubs.push_back(_events->AddListener(this, &LobbyScreen::OnEnter));
	for (const InputChannel channel: kSlots | std::views::transform(LocalInput))
	{
		_choiceSubs.push_back(_events->AddListener(Key(channel), this, &LobbyScreen::OnPadUp));
		_choiceSubs.push_back(_events->AddListener(Key(channel), this, &LobbyScreen::OnPadDown));
		_choiceSubs.push_back(_events->AddListener(Key(channel), this, &LobbyScreen::OnFire));
	}
}

void LobbyScreen::Draw() const
{
	_events->EmitEvent(RenderMenuBackgroundEvent{});
	if (!_isLobby)
	{
		DrawAbsence();

		return;
	}

	UiTable lines{};
	if (_isServerFull)
	{
		lines.rows = {Line("MATCH IN PROGRESS"), Line("WAITING FOR A FREE SEAT")};
	}
	else
	{
		lines.rows = {Line(IsHost(_gameConfig.gameMode) ? "WAITING FOR PLAYER" : "CONNECTING TO HOST")};
	}

	if (_match)
	{
		const bool isClassic{_match->rules == MatchRules::Classic};
		lines.rows.push_back(Line(std::string{isClassic ? "CLASSIC" : "FREE FOR ALL"} + ", "
								  + std::to_string(_match->seats) + " SEATS, MAP " + _match->map));
		if (isClassic)
		{
			lines.rows.push_back(Line(std::to_string(_match->enemiesAtOnce) + " ENEMIES AT ONCE"));
		}

		lines.rows.push_back(Line(std::to_string(_match->bots) + " BOTS, "
								  + (_match->isStartingAtOnce ? "STARTS AT ONCE" : "STARTS WHEN FULL")));
	}

	lines.rows.push_back(Line("PRESS M FOR MENU"));

	std::vector<UiTable> tables{};
	tables.push_back(std::move(lines));
	_events->EmitEvent(RenderPanelTablesEvent{.tables = std::move(tables)});
}

void LobbyScreen::DrawAbsence() const
{
	UiTable seats{};
	for (const PlayerSlot slot: kSlots)
	{
		const std::string player{"PLAYER " + std::to_string(SeatIndex(slot) + 1u)};
		switch (_absence[SeatIndex(slot)])
		{
			case Absence::None:
				break;
			case Absence::Left:
				seats.rows.push_back(Line(player + " LEFT, WAITING"));
				break;
			case Absence::Back:
				seats.rows.push_back(Line(player + " IS BACK"));
				break;
		}
	}

	const bool isAnyoneOut{std::ranges::contains(_absence, Absence::Left)};
	UiTable choices{};
	std::ranges::transform(Choices(), std::back_inserter(choices.rows), [isAnyoneOut](const AbsenceChoice choice)
	{
		return Line(std::string{Label(choice, isAnyoneOut)});
	});

	std::vector<UiTable> tables{};
	tables.push_back(std::move(seats));
	tables.push_back(std::move(choices));
	_events->EmitEvent(RenderPanelTablesEvent{.tables = std::move(tables),
											  .pick = PanelPick{.table = 1u, .selectedRow = _pick}});
}
