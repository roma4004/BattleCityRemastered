#pragma once

#include "components/EventSystem.h"
#include "components/MatchSettings.h"
#include "enums/Absence.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/PlayerSlot.h"
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

struct PauseStatusEvent;
struct GameResetEvent;
struct GameModeAppliedEvent;
struct DemoStartedEvent;
struct GameFinishedEvent;
struct MapLoadFailedEvent;
struct ServerInClientReadyToStartGameEvent;
struct ServerInTankCommandEvent;
struct ServerInRestartRequestedEvent;
struct MatchRestartRequestedEvent;
struct HostPhaseAnnouncedEvent;
struct ServerInDisconnectEvent;
struct ServerClientLostEvent;
struct ServerClientSeatedEvent;
struct ServerClientSyncedEvent;
struct AbsenceChangedEvent;
struct AbsenceChosenEvent;
struct ClientInDisconnectEvent;
struct ClientReconnectAbandonedEvent;
struct ClientHostLostEvent;
struct WorldSnapshotRequestedEvent;
class EventSystem;

class GameStateManager final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	GameState _state{GameState::Menu};
	GameMode _gameMode{};
	//NOTE: seats, not a count - a seat whose client drops before its ready was never taken
	std::bitset<kSeatCount> _readySeats{};
	//NOTE: a client sits down before it is ready - a match starting at once still waits for the ones already in
	std::bitset<kSeatCount> _seatedSeats{};
	//NOTE: joined a running match and not yet holding the field it was sent
	std::bitset<kSeatCount> _syncingSeats{};
	//NOTE: seats whose player has driven the tank; a bot never takes those back at the next level
	std::bitset<kSeatCount> _drivenSeats{};
	std::array<SeatHolder, kSeatCount> _holders{};
	//NOTE: written by the host, mirrored by a client; the absence panel replaces the pause plate
	std::array<Absence, kSeatCount> _absence{};
	bool _isHeld{};
	//NOTE: the match starts once this many are ready, the bots counted in
	std::size_t _seatCount{};
	std::size_t _bots{};
	//NOTE: two with no enemies - one alone would have nobody to fight
	std::size_t _fewestToStart{};
	bool _isStartingAtOnce{};
	bool _isDemo{};
	bool _isPaused{};

	void Subscribe();
	void SetState(GameState state);
	void AnnouncePhase();
	void Resume();
	[[nodiscard]] GameState IdleStateForMode() const;
	[[nodiscard]] bool IsReadyToStart() const noexcept;
	[[nodiscard]] std::array<SeatHolder, kSeatCount> HoldersFor(const std::bitset<kSeatCount>& players) const;
	void FillSeats();
	void AnnounceLobbySeats();
	void AnnounceHolders() const;

	void OnGameModeApplied(const GameModeAppliedEvent& event);
	void OnDemoStarted(const DemoStartedEvent&);
	void OnPauseStatus(const PauseStatusEvent& event);
	void OnGameFinished(const GameFinishedEvent& event);
	void OnMapLoadFailed(const MapLoadFailedEvent&);
	void OnClientReady(const ServerInClientReadyToStartGameEvent& event);
	void OnTankCommand(const ServerInTankCommandEvent& event);
	void OnRestartRequested(const ServerInRestartRequestedEvent&);
	void OnMatchRestartRequested(const MatchRestartRequestedEvent&);
	void OnHostPhase(const HostPhaseAnnouncedEvent& event);
	void OnClientLeft(const ServerInDisconnectEvent& event);
	void OnClientLost(const ServerClientLostEvent& event);
	void OnClientSeated(const ServerClientSeatedEvent& event);
	void OnClientSynced(const ServerClientSyncedEvent& event);
	void OnHostLeft(const ClientInDisconnectEvent&);
	void OnHostUnreachable(const ClientReconnectAbandonedEvent&);
	void OnHostLost(const ClientHostLostEvent&);
	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;

	void TakeSeat(PlayerSlot slot);
	void JoinRunningMatch(PlayerSlot slot);
	void FinishSync(PlayerSlot slot);
	void FreeSeat(PlayerSlot slot);
	void ForgetSeats();
	void UpdateHold();
	[[nodiscard]] bool HasAbsence() const noexcept;
	void AnnounceAbsence() const;
	void ForgetAbsence();
	void HandAbsentSeatsTo(SeatHolder to);
	void OnAbsenceChosen(const AbsenceChosenEvent& event);
	void OnAbsenceChanged(const AbsenceChangedEvent& event);
	void LoseHost();

	void Reset(const GameResetEvent&);

public:
	explicit GameStateManager(const std::shared_ptr<EventSystem>& events, const MatchSettings& match = {});

	[[nodiscard]] bool IsPauseShown() const;
};
