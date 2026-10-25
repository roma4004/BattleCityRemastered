#include "network/ServerCatalog.h"
#include "network/Catalog.h"
#include "network/Discovery.h"
#include "network/Endpoints.h"
#include "utils/Log.h"
#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/v6_only.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>

namespace network
{
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

	if (datagram == discovery::kProbe)
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
		const auto isHeard = [now](const auto& entry) { return now - entry.second.heardAt <= catalog::kSilenceLimit; };
		for (const auto& [server, entry]: _entries | std::views::filter(isHeard))
		{
			const auto bytes{catalog::Pack(catalog::Listing{.heartbeat = entry.heartbeat, .host = server.address()})};
			std::ignore = _socket.send_to(boost::asio::buffer(bytes), _sender, {}, ec);
		}

		std::ignore = _socket.send_to(boost::asio::buffer(catalog::kListEnd), _sender, {}, ec);

		return;
	}

	const auto heartbeat{catalog::ParseHeartbeat(datagram)};
	if (!heartbeat)
	{
		return;
	}

	const udp::endpoint server{sender, heartbeat->reply.gamePort};
	if (heartbeat->reply.seats == 0u)
	{
		if (_entries.erase(server) > 0u)
		{
			Log::Info("catalog: " + server.address().to_string() + ':' + std::to_string(server.port()) + " left");
		}

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
