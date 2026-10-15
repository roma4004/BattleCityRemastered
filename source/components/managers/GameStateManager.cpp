#include "components/managers/GameStateManager.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/MatchSettings.h"
#include "enums/Absence.h"
#include "enums/GameMode.h"
#include "enums/PlayerSlot.h"
#include "network/commands/WorldSnapshot.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <ranges>
#include <utility>

GameStateManager::GameStateManager(const std::shared_ptr<EventSystem>& events, const MatchSettings& match)
	: _events{events}
	, _seatCount{match.seats}
	, _bots{std::min(match.bots, MaxBots(match.seats))}
	, _fewestToStart{match.enemiesAtOnce > 0u ? 1u : 2u}
	, _isStartingAtOnce{match.isStartingAtOnce}
{
	Subscribe();
}

void GameStateManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnGameModeApplied));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnDemoStarted));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnPauseStatus));
	_subs.push_back(_events->AddListener(this, &GameStateManager::Reset));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnGameFinished));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnMapLoadFailed));

	_subs.push_back(_events->AddListener(this, &GameStateManager::OnClientReady));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnTankCommand));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnRestartRequested));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnMatchRestartRequested));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnHostPhase));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnClientLeft));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnClientLost));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnClientSeated));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnClientSynced));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnAbsenceChosen));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnAbsenceChanged));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnHostLeft));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnHostUnreachable));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnHostLost));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnWorldSnapshotRequested));
}

void GameStateManager::SetState(const GameState state)
{
	if (_state == state)
	{
		return;
	}

	_state = state;
	AnnouncePhase();
}

void GameStateManager::AnnouncePhase()
{
	//NOTE: settled before the match is - the reset it starts spawns by them
	if (_state == GameState::Playing && IsHost(_gameMode))
	{
		FillSeats();
	}

	_events->EmitEvent(GameStateChangedToEvent{.state = _state});

	if (_state == GameState::Playing)
	{
		_events->EmitEvent(MatchStartedEvent{});
	}
}

void GameStateManager::Resume()
{
	_state = GameState::Playing;
	_events->EmitEvent(GameStateChangedToEvent{.state = _state});
}

GameState GameStateManager::IdleStateForMode() const
{
	if (_isDemo)
	{
		return GameState::Demo;
	}

	if (!IsNetworkGame(_gameMode))
	{
		return GameState::Playing;
	}

	//NOTE: only the host counts seats. A client waits to be told, so its own idle phase is the lobby
	return IsHost(_gameMode) && IsReadyToStart() ? GameState::Playing : GameState::Lobby;
}

//NOTE: a client already in is waited for either way - starting without it would have it join a running match
bool GameStateManager::IsReadyToStart() const noexcept
{
	const std::size_t players{_readySeats.count()};
	const bool isEveryoneInReady{(_seatedSeats & ~_readySeats).none()};

	return players > 0u && isEveryoneInReady && players + _bots >= _fewestToStart
		   && (_isStartingAtOnce || players + _bots >= _seatCount);
}

//NOTE: bots fill empty seats from the top, so a later joiner sits in a free one first; the rest take players' seats
void GameStateManager::FillSeats()
{
	std::size_t botsLeft{_bots};
	for (const std::size_t seat: std::views::iota(std::size_t{}, kSeatCount) | std::views::reverse)
	{
		if (_readySeats.test(seat))
		{
			_holders[seat] = SeatHolder::Player;
		}
		else if (seat < _seatCount && botsLeft > 0u)
		{
			_holders[seat] = SeatHolder::Bot;
			--botsLeft;
		}
		else
		{
			_holders[seat] = SeatHolder::Empty;
		}
	}

	const auto isWatchable = [this](const std::size_t seat)
	{
		return _holders[seat] == SeatHolder::Player && !_drivenSeats.test(seat);
	};
	std::ranges::for_each(std::views::iota(std::size_t{}, _seatCount) | std::views::reverse
								  | std::views::filter(isWatchable) | std::views::take(botsLeft),
						  [this](const std::size_t seat) { _holders[seat] = SeatHolder::Bot; });
	std::ranges::for_each(std::views::iota(std::size_t{}, kSeatCount), [this](const std::size_t seat)
	{
		_drivenSeats.set(seat, _drivenSeats.test(seat) || _holders[seat] == SeatHolder::Player);
	});

	_events->EmitEvent(SeatsFilledEvent{.holders = _holders});
	AnnounceHolders();
}

void GameStateManager::AnnounceHolders() const { _events->EmitEvent(SeatHoldersAnnouncedEvent{.holders = _holders}); }

void GameStateManager::OnGameModeApplied(const GameModeAppliedEvent& event)
{
	_gameMode = event.mode;
	ForgetSeats();
	_seatedSeats.reset();
	_drivenSeats.reset();
	_holders.fill(SeatHolder::Empty);
	_isDemo = false;

	//NOTE: announced even when the phase keeps its name - spawners act on entering one, not on a diff
	_state = IdleStateForMode();
	AnnouncePhase();
}

void GameStateManager::OnDemoStarted(const DemoStartedEvent&)
{
	_isDemo = true;
	SetState(GameState::Demo);
}

void GameStateManager::OnPauseStatus(const PauseStatusEvent& event)
{
	_isPaused = event.isPaused;

	if (event.isPaused && _state == GameState::Playing)
	{
		SetState(GameState::Paused);
	}
	else if (!event.isPaused && _state == GameState::Paused)
	{
		Resume();
	}
}

void GameStateManager::OnGameFinished(const GameFinishedEvent& event) { SetState(event.state); }

//NOTE: not the mode's idle phase - a lobby would start the same match the moment the seats are ready,
//and it would fail on the same map
void GameStateManager::OnMapLoadFailed(const MapLoadFailedEvent&) { SetState(GameState::Menu); }

void GameStateManager::TakeSeat(const PlayerSlot slot)
{
	const std::size_t seat{SeatIndex(slot)};
	//NOTE: a second ready from a seat already in is no join
	const bool isJoining{IsInMatch(_state) && !_readySeats.test(seat)};
	_readySeats.set(seat);

	if (isJoining)
	{
		JoinRunningMatch(slot);

		return;
	}

	if (_state == GameState::Lobby && IsReadyToStart())
	{
		SetState(GameState::Playing);
	}
}

//NOTE: a seat nobody held is the newcomer's at once, a bot's stays the bot's until their first command
void GameStateManager::JoinRunningMatch(const PlayerSlot slot)
{
	const std::size_t seat{SeatIndex(slot)};
	if (_holders[seat] != SeatHolder::Bot)
	{
		const SeatHolder from{std::exchange(_holders[seat], SeatHolder::Player)};
		_events->EmitEvent(SeatHolderChangedEvent{.slot = slot, .from = from});
		_drivenSeats.set(seat);
	}

	AnnounceHolders();

	//NOTE: back while the others are still asked about it - the match goes on once one of them says so
	if (_absence[seat] != Absence::None)
	{
		_absence[seat] = Absence::Back;
		AnnounceAbsence();
	}

	_syncingSeats.set(seat);
	UpdateHold();
}

void GameStateManager::FinishSync(const PlayerSlot slot)
{
	_syncingSeats.reset(SeatIndex(slot));
	UpdateHold();
}

void GameStateManager::FreeSeat(const PlayerSlot slot)
{
	const std::size_t seat{SeatIndex(slot)};
	const bool wasPlaying{_readySeats.test(seat) && (_state == GameState::Playing || _state == GameState::Paused)};
	_readySeats.reset(seat);
	_seatedSeats.reset(seat);
	_syncingSeats.reset(seat);
	_drivenSeats.reset(seat);

	if (!IsNetworkGame(_gameMode))
	{
		return;
	}

	//NOTE: the match is held for whoever stayed to say whether it goes on without the one who left
	if (_readySeats.any() && IsInMatch(_state))
	{
		if (wasPlaying)
		{
			_absence[seat] = Absence::Left;
			AnnounceAbsence();
		}

		UpdateHold();

		return;
	}

	//NOTE: the one who left may have been all a match starting at once still waited for
	if (_state == GameState::Lobby && IsReadyToStart())
	{
		SetState(GameState::Playing);

		return;
	}

	//NOTE: the phase goes first, as on a restart
	SetState(GameState::Lobby);
	ForgetAbsence();
	UpdateHold();
}

//NOTE: the clients stay seated - they only have to be ready again
void GameStateManager::ForgetSeats()
{
	_readySeats.reset();
	_syncingSeats.reset();
	ForgetAbsence();
	UpdateHold();
}

//NOTE: on a change only - every announce reaches the pause as a fresh ask
void GameStateManager::UpdateHold()
{
	const bool isHeld{_syncingSeats.any() || HasAbsence()};
	if (isHeld == _isHeld)
	{
		return;
	}

	_isHeld = isHeld;
	_events->EmitEvent(MatchHoldChangedEvent{.isHeld = _isHeld});
}

bool GameStateManager::HasAbsence() const noexcept
{
	return std::ranges::any_of(_absence, [](const Absence absence) { return absence != Absence::None; });
}

void GameStateManager::AnnounceAbsence() const { _events->EmitEvent(AbsenceChangedEvent{.seats = _absence}); }

void GameStateManager::ForgetAbsence()
{
	if (!HasAbsence())
	{
		return;
	}

	_absence.fill(Absence::None);
	AnnounceAbsence();
}

//NOTE: the seats stay theirs either way - whoever of them dials back takes the seat over from nobody or from the bot
void GameStateManager::HandAbsentSeatsTo(const SeatHolder to)
{
	const auto isGone = [this](const PlayerSlot slot) { return _absence[SeatIndex(slot)] == Absence::Left; };

	for (const PlayerSlot slot: kSlots | std::views::filter(isGone))
	{
		const SeatHolder from{std::exchange(_holders[SeatIndex(slot)], to)};
		_events->EmitEvent(SeatHolderChangedEvent{.slot = slot, .from = from, .to = to});
	}

	AnnounceHolders();
}

//NOTE: waiting takes no answer - the panel standing is the wait. The first answer decides: a later one finds
//nobody left to ask about, so it changes nothing
void GameStateManager::OnAbsenceChosen(const AbsenceChosenEvent& event)
{
	if (!IsHost(_gameMode))
	{
		return;
	}

	HandAbsentSeatsTo(event.choice == AbsenceChoice::Bot ? SeatHolder::Bot : SeatHolder::Empty);
	_absence.fill(Absence::None);
	AnnounceAbsence();
	UpdateHold();
}

void GameStateManager::OnAbsenceChanged(const AbsenceChangedEvent& event)
{
	if (IsClient(_gameMode))
	{
		_absence = event.seats;
	}
}

void GameStateManager::LoseHost()
{
	if (IsNetworkGame(_gameMode))
	{
		SetState(GameState::Lobby);
		ForgetAbsence();
	}
}

void GameStateManager::OnClientReady(const ServerInClientReadyToStartGameEvent& event) { TakeSeat(event.slot); }

//NOTE: the tank changes hands where it stands, and the match waits for the field saying so to reach the one taking it
void GameStateManager::OnTankCommand(const ServerInTankCommandEvent& event)
{
	const std::size_t seat{SeatIndex(event.slot)};
	const bool isWatching{_readySeats.test(seat) && !_syncingSeats.test(seat) && _holders[seat] == SeatHolder::Bot};
	if (_state != GameState::Playing || !isWatching)
	{
		return;
	}

	_holders[seat] = SeatHolder::Player;
	_drivenSeats.set(seat);
	_events->EmitEvent(SeatHolderChangedEvent{.slot = event.slot, .from = SeatHolder::Bot});
	AnnounceHolders();

	_syncingSeats.set(seat);
	UpdateHold();
}

//NOTE: announced rather than set - a local match is already Playing, and SetState would see no change
//and leave the finished board on the screen
void GameStateManager::OnMatchRestartRequested(const MatchRestartRequestedEvent&)
{
	_state = IdleStateForMode();
	AnnouncePhase();
}

//NOTE: the phase goes first - a hold let go of here would otherwise resume the match being torn down
void GameStateManager::OnRestartRequested(const ServerInRestartRequestedEvent&)
{
	_state = GameState::Lobby;
	ForgetSeats();
	AnnouncePhase();
}

//NOTE: announced even when the phase keeps its name - SetState would swallow the repeat
void GameStateManager::OnHostPhase(const HostPhaseAnnouncedEvent& event)
{
	//NOTE: the field the players are looking at is not restarted under them - leaving a pause announces
	//Playing again, and a full announce would have it emptied and the map loaded over
	if (const bool isResuming{event.phase == GameState::Playing
							  && (_state == GameState::Paused || _state == GameState::Playing)};
		isResuming)
	{
		Resume();

		return;
	}

	_state = event.phase;
	AnnouncePhase();
}

void GameStateManager::OnClientLeft(const ServerInDisconnectEvent& event) { FreeSeat(event.slot); }
void GameStateManager::OnClientLost(const ServerClientLostEvent& event) { FreeSeat(event.slot); }
void GameStateManager::OnClientSeated(const ServerClientSeatedEvent& event) { _seatedSeats.set(SeatIndex(event.slot)); }
void GameStateManager::OnClientSynced(const ServerClientSyncedEvent& event) { FinishSync(event.slot); }
void GameStateManager::OnHostLeft(const ClientInDisconnectEvent&) { LoseHost(); }
void GameStateManager::OnHostUnreachable(const ClientReconnectAbandonedEvent&) { LoseHost(); }
void GameStateManager::OnHostLost(const ClientHostLostEvent&) { LoseHost(); }

void GameStateManager::OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const
{
	event.snapshot.phase = _state;
	event.snapshot.absence = _absence;
}

//NOTE: not under the panel asking about the one who left - the panel says why the match stands
bool GameStateManager::IsPauseShown() const
{
	const bool isPaused{_state == GameState::Paused || (_isPaused && (IsInMatch(_state) || _state == GameState::Demo))};

	return isPaused && !HasAbsence();
}

void GameStateManager::Reset(const GameResetEvent&)
{
	if (_state == GameState::Won || _state == GameState::Over)
	{
		SetState(IdleStateForMode());
	}
}
