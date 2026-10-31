#pragma once

#include "Discovery.h"
#include "enums/MatchRules.h"
#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/udp.hpp>
#include <cstdint>
#include <functional>
#include <list>

namespace network
{
using boost::asio::ip::udp;

//NOTE: the server side of discovery - listens on kDiscoveryPort and answers with the game port it got
class DiscoveryBeacon final
{
public:
	DiscoveryBeacon(boost::asio::io_context& ioContext, const boost::asio::ip::address& host,
					std::uint16_t gamePort, std::uint8_t seats, MatchRules rules,
					std::function<std::uint8_t()> freeSeats);

	~DiscoveryBeacon();

	DiscoveryBeacon(const DiscoveryBeacon&) = delete;
	DiscoveryBeacon& operator=(const DiscoveryBeacon&) = delete;
	DiscoveryBeacon(DiscoveryBeacon&&) = delete;
	DiscoveryBeacon& operator=(DiscoveryBeacon&&) = delete;

	void Shutdown();

	[[nodiscard]] discovery::Reply Answer() const;

private:
	//NOTE: a socket probes come in by, with the last one's sender and bytes
	struct Inbox final
	{
		udp::socket socket;
		udp::endpoint sender{};
		//NOTE: room enough to tell a probe from anything else - a longer datagram is not one either way
		std::array<char, 16u> buffer{};
	};

	//NOTE: false when the address is taken, or is not this machine's
	bool Listen(Inbox& inbox, const boost::asio::ip::address& address);
	//NOTE: on every link this machine has - a server listening over IPv6 is found by the group, not a broadcast
	void JoinGroupIPv6();
	//NOTE: Linux delivers the group only to a socket bound to it, and ff02:: is bound to one link at a time
	void ListenToGroupIPv6(const boost::asio::ip::address& host);
	void Receive(Inbox& inbox);

	//NOTE: bound to the server's address - every answer goes out of it, so the asker reads that address
	Inbox _inbox;
	//NOTE: Linux delivers a broadcast only to a socket bound to its address - 255.255.255.255 or the subnet one
	Inbox _broadcastInbox;
	Inbox _subnetInbox;
	//NOTE: a list - each one's receive holds a reference to it
	std::list<Inbox> _groupInboxes{};
	//NOTE: on "::" a link-local asker is answered from the link-local address - the further one is named in the answer
	const bool _isOnEveryAddress;
	const std::uint16_t _gamePort;
	const std::uint8_t _seats;
	const MatchRules _rules;
	const std::uint32_t _serverId;
	std::function<std::uint8_t()> _freeSeats;
};
}//namespace network
