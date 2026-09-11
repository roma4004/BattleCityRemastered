#pragma once

#include "enums/DisconnectReason.h"
#include "PeerLink.h"
#include "ReplicationApplier.h"
#include "ReplicationPublisher.h"
#include "commands/AnyCommand.h"
#include "components/EventSystem.h"
#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct NetworkEndFrameEvent;
struct PlayerSlotAssignedEvent;
class EventSystem;

namespace network::commands
{
using boost::asio::ip::tcp;

class Client final : public PeerLink, public std::enable_shared_from_this<Client>
{
public:
	Client(boost::asio::io_context& ioContext, std::string host, uint16_t port,
		   const std::shared_ptr<EventSystem>& events);

	~Client();

	[[nodiscard]] bool IsConnected() const { return _isConnected; }

	void Shutdown();

	//NOTE: onClosed fires once the goodbye is on the wire, or turned out undeliverable
	void Shutdown(DisconnectReason reason, std::function<void()> onClosed);

private:
	void Subscribe();
	void OnCommand(const AnyCommand& command) override;

	void StartReading();
	void TryConnect();

	void OnNetworkEndFrame(const NetworkEndFrameEvent&);
	void OnSlotAssigned(const PlayerSlotAssignedEvent& event);

	//NOTE: the only command Client reads itself - the rest are the applier's, and this one is not a
	//game fact but transport state, taken on the network thread before the queue
	void OnDisconnect(const Disconnect& command);
	void HandleDisconnect();
	void HandleProtocolError();
	void ScheduleReconnect();

	boost::asio::steady_timer _reconnectTimer;
	tcp::endpoint _endpoint;
	ReplicationApplier _replicationIn;
	ReplicationPublisher _replicationOut;
	std::vector<EventSubscription> _subs{};
	std::vector<EventSubscription> _inputSubs{};
	std::atomic<bool> _isConnected{};
	bool _reconnectPending{false};
	//NOTE: tells our own cancellation apart from a dropped link, so teardown does not reconnect
	std::atomic<bool> _isShuttingDown{false};
	//NOTE: a goodbye is not one of these - the host sends it while restarting the same mode, and
	//the client is the only side that can dial back
	std::atomic<bool> _isLinkUnrecoverable{false};
	std::atomic<bool> _isWaitingForSeat{false};
	unsigned char _reconnectAttempts{0u};
	//NOTE: one drop can be reported twice, by the read and by the write - give up once
	bool _reconnectAbandoned{false};
	static constexpr unsigned char kMaxReconnectAttempts{10u};
	static constexpr unsigned short kReconnectDelayMs{500u};
	static constexpr unsigned short kFullServerRetryMs{3000u};
};
}//namespace network::commands
