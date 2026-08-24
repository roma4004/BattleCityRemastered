#pragma once

#include "PeerLink.h"
#include "enums/DisconnectReason.h"
#include "enums/InputSignal.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class EventSystem;

namespace network::commands
{
using boost::asio::ip::tcp;

//NOTE: the incoming half of the server - one per connected client. Outgoing replication is Server's.
class Session final : public PeerLink, public std::enable_shared_from_this<Session>
{
public:
	Session(tcp::socket sock, const std::shared_ptr<EventSystem>& events);

	~Session();

	void Start();
	//NOTE: shared, not copied - the same frame goes to every session and stays alive while it is written
	void DoWrite(std::shared_ptr<const std::string> message);
	void Shutdown();

	//NOTE: onClosed fires once the goodbye is written, or turned out undeliverable
	void Shutdown(DisconnectReason reason, std::function<void()> onClosed);

private:
	using InputEmitter = std::function<void(EventSystem&, const std::string&, bool)>;

	void RegisterCommandHandlers();
	void OnSignalEvent(const AnyCommand& command);
	void OnKeyStateChange(const AnyCommand& command);
	void OnDisconnect(const AnyCommand& command);

	static const std::unordered_map<InputSignal, InputEmitter> kInputEmitters;

	bool _isPeerGone{false};
};
}//namespace network::commands
