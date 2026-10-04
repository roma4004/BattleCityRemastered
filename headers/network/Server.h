#pragma once

#include "DatagramLink.h"
#include "DiscoveryBeacon.h"
#include "Endpoints.h"
#include "ReplicationPublisher.h"
#include "SeatHistory.h"
#include "Session.h"
#include "WireFrame.h"
#include "commands/PortForwardingChange.h"
#include "components/EventSystem.h"
#include "components/MatchSettings.h"
#include "enums/DisconnectReason.h"
#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct MapLoadedEvent;
struct NetworkEndFrameEvent;
struct PortForwardingChangedEvent;
struct ServerClientSeatedEvent;
struct SeatHolderChangedEvent;
struct ServerStatusRequestedEvent;
struct ServerPlayersRequestedEvent;
struct ServerKickRequestedEvent;
struct ServerAcceptingChangedEvent;
class EventSystem;

namespace network::commands
{
using boost::asio::ip::udp;

class Server final
{
public:
	Server(boost::asio::io_context& ioContext, const ServerAddress& address,
		   const std::shared_ptr<EventSystem>& events, const MatchSettings& match);

	~Server();

	void Shutdown();

	void Shutdown(DisconnectReason reason, const std::function<void()>& onClosed);

	[[nodiscard]] uint16_t GetBoundPort() const noexcept { return _boundPort; }

	void ProcessNetworkCommands() const;

private:
	using Clock = DatagramLink::Clock;

	void Receive();
	void OnDatagram(std::string_view datagram, Clock::time_point now);

	//NOTE: the seat is the one FindFreeSlot picks - the order clients arrive in is the order they sit, unless
	//one sat here before
	void Seat(const udp::endpoint& endpoint, std::uint32_t connectionId, std::string_view hello,
			  Clock::time_point now);

	//NOTE: told why, the client waits for a free match instead of burning its retries
	void RefuseSeat(const udp::endpoint& endpoint, std::uint32_t connectionId, Clock::time_point now);

	void Tick();
	void ScheduleTick();
	void Transmit(Session& session, Clock::time_point now);
	void SendTo(const udp::endpoint& endpoint, const std::vector<std::string>& datagrams);
	void CloseSocket();

	[[nodiscard]] std::vector<std::shared_ptr<Session>> CopySessions() const;

	//NOTE: called with _sessionsMutex held - the search and the session that takes the seat have to
	//be one step, or two clients arriving back to back get the same one
	[[nodiscard]] std::optional<PlayerSlot> FindFreeSlot(const udp::endpoint& endpoint) const;

	//NOTE: what the beacon answers with, so it is taken under the mutex like any other read of the seats
	[[nodiscard]] std::uint8_t CountFreeSlots() const;

	void OnNetworkEndFrame(const NetworkEndFrameEvent&);
	void OnMapLoaded(const MapLoadedEvent& event);
	void OnSeatHolderChanged(const SeatHolderChangedEvent&) const;

	//NOTE: nullptr outside a match - a lobby has no field to catch up with
	[[nodiscard]] std::shared_ptr<const WireFrame> TakeWorldSnapshot() const;

	void CleanupDeadSessions();
	void OnStatusRequested(const ServerStatusRequestedEvent&) const;
	void OnPlayersRequested(const ServerPlayersRequestedEvent&) const;
	void OnKickRequested(const ServerKickRequestedEvent& event) const;
	void OnAcceptingChanged(const ServerAcceptingChangedEvent& event);
	void OnPortForwardingChanged(const PortForwardingChangedEvent& event);
	void OnClientSeated(const ServerClientSeatedEvent&);

	udp::socket _socket;
	//NOTE: written by the console on the game thread, read where a hello lands on the network one - UDP has
	//no acceptor to close, so the socket stays bound and the hello is turned away
	std::atomic_bool _isAccepting{true};
	const uint16_t _boundPort;
	//NOTE: its seats held to one to four - every client is told the match as it is played, the map it is on
	//now included, so it is written under _sessionsMutex
	MatchSettings _match;
	//NOTE: after _boundPort - it answers with that number, so it may not be built before there is one
	DiscoveryBeacon _beacon;
	boost::asio::steady_timer _tickTimer;
	udp::endpoint _sender{};
	std::array<char, DatagramLink::kMaxDatagramSize> _receiveBuffer{};
	std::shared_ptr<EventSystem> _events{nullptr};
	ReplicationPublisher _replicationOut;
	std::vector<EventSubscription> _subs{};

	std::vector<std::shared_ptr<Session>> _sessions;
	//NOTE: written where a seat is taken, under _sessionsMutex as well
	SeatHistory _seatHistory;
	mutable std::mutex _sessionsMutex;

	//NOTE: what the router said last - told again to whoever sits down after. Both sides of it are on the game thread
	std::optional<PortForwardingChange> _portForwarding{};

	//NOTE: set once the goodbyes are out - the socket closes when every one is acked or the linger runs out
	std::function<void()> _onClosed{};
	Clock::time_point _closeDeadline{};
};
}//namespace network::commands
