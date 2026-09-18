#pragma once

#include "Endpoints.h"
#include "enums/DisconnectReason.h"
#include "ReplicationPublisher.h"
#include "Session.h"
#include "components/EventSystem.h"
#include "enums/PlayerSlot.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct NetworkEndFrameEvent;
struct ServerStatusRequestedEvent;
struct ServerPlayersRequestedEvent;
struct ServerKickRequestedEvent;
struct ServerAcceptingChangedEvent;
class EventSystem;

namespace network::commands
{
using boost::asio::ip::tcp;

class Server final
{
public:
	Server(boost::asio::io_context& ioContext, const ServerAddress& address,
		   const std::shared_ptr<EventSystem>& events);

	~Server();

	void Shutdown();

	void Shutdown(DisconnectReason reason, const std::function<void()>& onClosed);

	//NOTE: remembered, not asked - a closed acceptor has no local endpoint
	[[nodiscard]] uint16_t GetBoundPort() const noexcept { return _endpoint.port(); }

	void ProcessNetworkCommands() const;

private:
	void DoAccept();

	//NOTE: the seat is whatever FindFreeSlot has left - the order clients arrive in is the order they sit
	void Seat(tcp::socket socket);

	void RefuseSeat(tcp::socket socket) const;

	[[nodiscard]] std::vector<std::shared_ptr<Session>> SnapshotSessions() const;

	//NOTE: called with _sessionsMutex held - the search and the session that takes the seat have to
	//be one step, or two clients accepted back to back get the same one
	[[nodiscard]] std::optional<PlayerSlot> FindFreeSlot() const;

	void OnNetworkEndFrame(const NetworkEndFrameEvent&);

	void SendToAll(const std::shared_ptr<const std::string>& message);
	void CleanupDeadSessions();
	void CloseAcceptor();
	void OpenAcceptor();
	void OnStatusRequested(const ServerStatusRequestedEvent&) const;
	void OnPlayersRequested(const ServerPlayersRequestedEvent&) const;
	void OnKickRequested(const ServerKickRequestedEvent& event) const;
	void OnAcceptingChanged(const ServerAcceptingChangedEvent& event);

	tcp::acceptor _acceptor;
	tcp::endpoint _endpoint;
	std::shared_ptr<EventSystem> _events{nullptr};
	ReplicationPublisher _replicationOut;
	std::vector<EventSubscription> _subs{};

	std::vector<std::shared_ptr<Session>> _sessions;
	mutable std::mutex _sessionsMutex;
};
}//namespace network::commands
