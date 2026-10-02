#include "network/Endpoints.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/system/error_code.hpp>
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>

namespace
{
//NOTE: any address past the local network will do
constexpr auto kOutsideAddress{"8.8.8.8"};
constexpr std::uint16_t kOutsidePort{53};
}//namespace

namespace network
{
//NOTE: connecting UDP sends nothing - the OS only picks the outgoing interface, whose address others reach
std::string LocalAddress()
{
	using boost::asio::ip::udp;

	boost::asio::io_context ioContext;
	udp::socket socket{ioContext};
	boost::system::error_code ec;
	std::ignore = socket.open(udp::v4(), ec);
	if (!ec)
	{
		std::ignore = socket.connect(udp::endpoint{boost::asio::ip::make_address(kOutsideAddress), kOutsidePort}, ec);
	}

	if (ec)
	{
		return kDefaultHost;
	}

	const udp::endpoint local{socket.local_endpoint(ec)};
	if (ec || local.address().is_unspecified())
	{
		return kDefaultHost;
	}

	return local.address().to_string();
}

bool IsThisMachine(const std::string_view host)
{
	boost::system::error_code ec;
	const auto address{boost::asio::ip::make_address(std::string{host}, ec)};

	return !ec && (address.is_loopback() || address.is_unspecified() || address.to_string() == LocalAddress());
}

std::optional<std::string> ParseHost(const std::string_view value)
{
	//NOTE: Windows make_address takes "1.2.3.4:4321" and drops the port; one colon is a port, IPv6 has two
	if (value.contains('[') || value.contains(']') || std::ranges::count(value, ':') == 1)
	{
		return std::nullopt;
	}

	boost::system::error_code ec;
	const auto parsed{boost::asio::ip::make_address(std::string{value}, ec)};
	if (ec)
	{
		return std::nullopt;
	}

	return parsed.to_string();
}

std::optional<std::uint16_t> ParsePort(const std::string_view value)
{
	if (value == "auto")
	{
		return std::uint16_t{};
	}

	unsigned number{};
	const auto* const last{value.data() + value.size()};
	const auto [ptr, error] = std::from_chars(value.data(), last, number);
	if (error != std::errc{} || ptr != last || number > 65535u)
	{
		return std::nullopt;
	}

	return static_cast<std::uint16_t>(number);
}

std::optional<ServerAddress> ParseServerAddress(const std::string_view value)
{
	std::string_view host{value};
	std::optional<std::string_view> port{};
	if (value.starts_with('['))
	{
		const auto closing{value.find(']')};
		if (closing == std::string_view::npos)
		{
			return std::nullopt;
		}

		host = value.substr(1, closing - 1);
		if (const std::string_view rest{value.substr(closing + 1)}; !rest.empty())
		{
			if (!rest.starts_with(':'))
			{
				return std::nullopt;
			}

			port = rest.substr(1);
		}
	}
	else if (std::ranges::count(value, ':') == 1)
	{
		const auto colon{value.find(':')};
		host = value.substr(0, colon);
		port = value.substr(colon + 1);
	}
	else if (const auto colon{value.rfind(':')}; colon != std::string_view::npos && !ParseHost(value))
	{
		host = value.substr(0, colon);
		port = value.substr(colon + 1);
	}

	const std::optional<std::string> parsedHost{ParseHost(host)};
	if (!parsedHost)
	{
		return std::nullopt;
	}

	if (!port)
	{
		return ServerAddress{.host = *parsedHost};
	}

	const std::optional<std::uint16_t> parsedPort{ParsePort(*port)};
	if (!parsedPort)
	{
		return std::nullopt;
	}

	return ServerAddress{.host = *parsedHost, .port = *parsedPort};
}
}//namespace network
