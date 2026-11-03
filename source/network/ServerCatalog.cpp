#include "network/ServerCatalog.h"
#include "network/Catalog.h"
#include "network/Discovery.h"
#include "network/Endpoints.h"
#include "utils/Log.h"
#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/asio/ip/v6_only.hpp>
#include <boost/system/error_code.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>

namespace network
{
namespace
{
//NOTE: an IPv6 host has a /64 to itself - counted by the address, it could list from as many as it likes
[[nodiscard]] boost::asio::ip::address NetworkOf(const boost::asio::ip::address& address)
{
	if (address.is_v4())
	{
		return address;
	}

	auto bytes{address.to_v6().to_bytes()};
	std::fill(bytes.begin() + 8, bytes.end(), std::uint8_t{});

	return boost::asio::ip::address_v6{bytes};
}
}//namespace

ServerCatalog::ServerCatalog(boost::asio::io_context& ioContext, const ServerAddress& address)
	: _socket{ioContext}
	, _sweep{ioContext}
{
	boost::system::error_code ec;
	const auto host{boost::asio::ip::make_address(address.host, ec)};
	if (!ec)
	{
		std::ignore = _socket.open(host.is_v4() ? udp::v4() : udp::v6(), ec);
	}

	//NOTE: dual-stack - one list for both protocols
	if (!ec && host.is_v6() && host.is_unspecified())
	{
		boost::system::error_code dualEc;
		std::ignore = _socket.set_option(boost::asio::ip::v6_only{false}, dualEc);
	}

	if (!ec)
	{
		std::ignore = _socket.bind(udp::endpoint{host, address.port}, ec);
	}

	if (ec)
	{
		Log::Error("catalog: cannot listen on " + address.host + ':' + std::to_string(address.port) + " - "
				   + ec.message());
		std::ignore = _socket.close(ec);

		return;
	}

	_boundPort = _socket.local_endpoint(ec).port();
	Receive();
	Sweep();
}

// NOLINTNEXTLINE(bugprone-exception-escape) - cancel() throws only on an error the timer service never sets
ServerCatalog::~ServerCatalog() { Shutdown(); }

void ServerCatalog::Shutdown()
{
	std::ignore = _sweep.cancel();
	boost::system::error_code ec;
	std::ignore = _socket.cancel(ec);
	std::ignore = _socket.close(ec);
}

void ServerCatalog::Receive()
{
	const auto onReceived = [this](const boost::system::error_code& ec, const std::size_t size)
	{
		if (ec == boost::asio::error::operation_aborted || !_socket.is_open())
		{
			return;
		}

		if (!ec)
		{
			Take(std::string_view{_buffer.data(), size}, Clock::now());
		}

		Receive();
	};
	_socket.async_receive_from(boost::asio::buffer(_buffer), _sender, onReceived);
}

void ServerCatalog::Take(const std::string_view datagram, const Clock::time_point now)
{
	//NOTE: a dual-stack socket hears IPv4 mapped into IPv6 - listed and limited as IPv4
	boost::asio::ip::address sender{_sender.address()};
	if (sender.is_v6() && sender.to_v6().is_v4_mapped())
	{
		sender = boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped, sender.to_v6());
	}

	const auto isHeard = [now](const auto& entry) { return now - entry.second.heardAt <= catalog::kSilenceLimit; };
	if (const auto nonce{catalog::ParseTagged(datagram, discovery::kProbe)})
	{
		Answers& answers{_answers[sender]};
		if (now - answers.since >= catalog::kAnswerWindow)
		{
			answers = Answers{.since = now};
		}

		if (answers.count == catalog::kAnswerBurst)
		{
			return;
		}

		++answers.count;

		//NOTE: a datagram per server, no retry - an unanswered probe asks again
		boost::system::error_code ec;
		for (const auto& [server, entry]: _entries | std::views::filter(isHeard))
		{
			const auto bytes{catalog::Pack(
					catalog::Listing{.heartbeat = entry.heartbeat, .host = server.address(), .nonce = *nonce})};
			std::ignore = _socket.send_to(boost::asio::buffer(bytes), _sender, {}, ec);
		}

		std::ignore = _socket.send_to(boost::asio::buffer(catalog::PackListEnd(*nonce)), _sender, {}, ec);

		return;
	}

	const auto heartbeat{catalog::ParseHeartbeat(datagram)};
	if (!heartbeat)
	{
		return;
	}

	//NOTE: only with the cookie echoed - a forged sender never hears it
	if (!_cookies.IsValid(heartbeat->cookie, _sender, heartbeat->reply.gamePort, now))
	{
		const auto challenge{catalog::Pack(catalog::Challenge{
				.token = heartbeat->token, .cookie = _cookies.For(_sender, heartbeat->reply.gamePort, now)})};
		boost::system::error_code ec;
		std::ignore = _socket.send_to(boost::asio::buffer(challenge), _sender, {}, ec);

		return;
	}

	const udp::endpoint server{sender, heartbeat->reply.gamePort};
	//NOTE: a server heard lately speaks with the token it came with - a restart takes over once the old one is silent
	if (const auto known{_entries.find(server)};
		known != _entries.end() && isHeard(*known) && known->second.heartbeat.token != heartbeat->token)
	{
		return;
	}

	if (heartbeat->reply.seats == 0u)
	{
		if (_entries.erase(server) > 0u)
		{
			Log::Info("catalog: " + server.address().to_string() + ':' + std::to_string(server.port()) + " left");
		}

		return;
	}

	//NOTE: ordered by address, then port - the sender's servers lie together from port 0; a silent one is not counted
	const boost::asio::ip::address network{NetworkOf(sender)};
	const auto isSameNetwork = [&network](const auto& entry) { return NetworkOf(entry.first.address()) == network; };
	const auto ofSender{std::ranges::subrange{_entries.lower_bound(udp::endpoint{network, 0u}), _entries.end()}
						| std::views::take_while(isSameNetwork)};
	if (!_entries.contains(server)
		&& (std::ranges::count_if(ofSender, isHeard) >= catalog::kServersPerAddress
			|| _entries.size() >= catalog::kMaxServers))
	{
		return;
	}

	if (_entries.insert_or_assign(server, Entry{.heartbeat = *heartbeat, .heardAt = now}).second)
	{
		Log::Info("catalog: listing " + server.address().to_string() + ':' + std::to_string(server.port()));
	}
}

void ServerCatalog::Sweep()
{
	_sweep.expires_after(catalog::kAnswerWindow);
	_sweep.async_wait([this](const boost::system::error_code& ec)
	{
		if (ec || !_socket.is_open())
		{
			return;
		}

		const auto now{Clock::now()};
		const auto isSilent = [now](const auto& entry) { return now - entry.second.heardAt > catalog::kSilenceLimit; };
		for (const udp::endpoint& server: _entries | std::views::filter(isSilent) | std::views::keys)
		{
			Log::Info("catalog: " + server.address().to_string() + ':' + std::to_string(server.port())
					  + " fell silent");
		}

		std::erase_if(_entries, isSilent);
		const auto isPast = [now](const auto& answers) { return now - answers.second.since >= catalog::kAnswerWindow; };
		std::erase_if(_answers, isPast);
		Sweep();
	});
}
}//namespace network
