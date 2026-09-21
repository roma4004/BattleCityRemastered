#pragma once

#include "DatagramLink.h"
#include "Endpoints.h"
#include "PeerLink.h"
#include "enums/DisconnectReason.h"
#include "ReplicationApplier.h"
#include "ReplicationPublisher.h"
#include "commands/AnyCommand.h"
#include "components/EventSystem.h"
#include <array>
#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct NetworkEndFrameEvent;
struct PlayerSlotAssignedEvent;
class EventSystem;

namespace network::commands
{
using boost::asio::ip::udp;

class Client final : public PeerLink, public std::enable_shared_from_this<Client>
{
public:
	Client(boost::asio::io_context& ioContext, const ServerAddress& address,
		   const std::shared_ptr<EventSystem>& events);

	~Client();

	[[nodiscard]] bool IsConnected() const { return _isConnected; }

	void Shutdown();

	//NOTE: onClosed fires once the goodbye is acked, or turned out undeliverable
	void Shutdown(DisconnectReason reason, std::function<void()> onClosed);

private:
	using Clock = DatagramLink::Clock;

	void Subscribe();
	void OnCommand(const AnyCommand& command) override;

	void Receive();
	void OnDatagram(std::string_view datagram, Clock::time_point now);
	void TryConnect();
	void ScheduleTick();
	void Tick();
	void Transmit(Clock::time_point now);
	void CloseSocket();

	void OnNetworkEndFrame(const NetworkEndFrameEvent&);
	void OnSlotAssigned(const PlayerSlotAssignedEvent& event);

	//NOTE: the only command Client reads itself - the rest are the applier's, and this one is not a
	//game fact but transport state, taken on the network thread before the queue
	void OnDisconnect(const Disconnect& command);
	void HandleDisconnect();
	void HandleProtocolError();
	void ScheduleReconnect();

	udp::endpoint _endpoint;
	udp::socket _socket;
	boost::asio::steady_timer _tickTimer;
	boost::asio::steady_timer _reconnectTimer;
	std::array<char, DatagramLink::kMaxDatagramSize> _receiveBuffer{};
	//NOTE: a new one for every connection attempt - its number is what the server tells attempts apart by
	std::optional<DatagramLink> _link{};
	Clock::time_point _attemptStartedAt{};
	ReplicationApplier _replicationIn;
	ReplicationPublisher _replicationOut;
	std::vector<EventSubscription> _subs{};
	std::vector<EventSubscription> _inputSubs{};
	std::atomic<bool> _isConnected{};
	bool _reconnectPending{};
	//NOTE: tells our own cancellation apart from a dropped link, so teardown does not reconnect
	std::atomic<bool> _isShuttingDown{};
	//NOTE: a goodbye is not one of these - the host sends it while restarting the same mode, and
	//the client is the only side that can dial back
	std::atomic<bool> _isLinkUnrecoverable{};
	std::atomic<bool> _isWaitingForSeat{};
	unsigned char _reconnectAttempts{};
	bool _reconnectAbandoned{};
	//NOTE: set once the goodbye is out - the socket closes when it is acked or the linger runs out
	std::function<void()> _onClosed{};
	Clock::time_point _closeDeadline{};
	static constexpr unsigned char kMaxReconnectAttempts{10u};
	static constexpr std::chrono::milliseconds kConnectTimeout{500};
	static constexpr std::chrono::milliseconds kReconnectDelay{500};
	static constexpr std::chrono::milliseconds kFullServerRetry{std::chrono::seconds{3}};
};
}//namespace network::commands
