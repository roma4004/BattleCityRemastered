#pragma once

#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/MatchSettings.h"
#include "enums/Absence.h"
#include "enums/PlayerSlot.h"
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

struct DrawUserInterfaceEvent;
struct GameStateChangedToEvent;
struct ClientInDisconnectEvent;
struct ClientConnectedToHostEvent;
struct MenuShownEvent;
struct PlayerSlotAssignedEvent;
struct ServerScreenShownEvent;
struct AbsenceChangedEvent;
struct EnterEvent;
struct FireEvent;
struct MoveUpEvent;
struct MoveDownEvent;
struct PanelRowClickedEvent;
struct PanelRowHoveredEvent;
class GameConfig;
class EventSystem;

//NOTE: the wait before a match, and the one inside it when a player leaves - then it asks the rest what to do
class LobbyScreen final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	EventSubscription _drawSub{};
	//NOTE: held only while the panel asks - clearing them stops the answers
	std::vector<EventSubscription> _choiceSubs{};

	const GameConfig& _gameConfig;

	bool _isLobby{};
	bool _isMenuShown{};
	bool _isServerScreenShown{};
	bool _isServerFull{};
	//NOTE: what the host picked - every seat is shown it, only the host could change it
	std::optional<MatchSettings> _match{};
	//NOTE: what a friend from the internet dials, once the server asked the router - or why nobody gets in
	std::optional<PortForwardingChangedEvent> _portForwarding{};
	std::array<Absence, kSeatCount> _absence{};
	//NOTE: an index into Choices()
	std::size_t _pick{};
	//NOTE: a release answers only a press made while the panel asked, not the shot that was being fired
	bool _isConfirmHeld{};

	void Subscribe();
	void OnGameStateChangedTo(const GameStateChangedToEvent& event);
	void OnMenuShown(const MenuShownEvent& event);
	void OnServerScreenShown(const ServerScreenShownEvent& event);
	void OnRefusedOrLost(const ClientInDisconnectEvent& event);
	void OnConnectedToHost(const ClientConnectedToHostEvent&);
	void OnSlotAssigned(const PlayerSlotAssignedEvent& event);
	void OnPortForwardingChanged(const PortForwardingChangedEvent& event);
	void OnAbsenceChanged(const AbsenceChangedEvent& event);
	void OnDrawUserInterface(const DrawUserInterfaceEvent&) const;

	void OnRowClicked(const PanelRowClickedEvent& event);
	void OnRowHovered(const PanelRowHoveredEvent& event);
	void OnEnter(const EnterEvent& event);
	void OnFire(const FireEvent& event);
	void OnPadUp(const MoveUpEvent& event);
	void OnPadDown(const MoveDownEvent& event);

	[[nodiscard]] bool IsAsking() const;
	[[nodiscard]] std::vector<AbsenceChoice> Choices() const;
	void Confirm(bool isPressed);
	void Choose(std::size_t pick) const;

	void Display();
	void Draw() const;
	void DrawAbsence() const;

public:
	LobbyScreen(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);
};
