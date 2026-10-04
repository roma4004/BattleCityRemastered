#pragma once

#include <igd_desc_parse.h>
#include <miniupnpc.h>
#include <string>

namespace network
{
//NOTE: the router this machine reaches the internet through, asked over UPnP while this is built - which takes
//seconds, eight when nothing answers, so it is built on a thread
class Router final
{
public:
	enum class State : char8_t
	{
		//NOTE: nothing on the network answered
		Silent,
		//NOTE: something answered, but no router connected to the internet
		Offline,
		//NOTE: connected, but its own outside address is a private one - another NAT stands between
		BehindProvider,
		Online
	};

	Router();
	~Router();

	Router(const Router&) = delete;
	Router& operator=(const Router&) = delete;
	Router(Router&&) = delete;
	Router& operator=(Router&&) = delete;

	[[nodiscard]] State GetState() const noexcept { return _state; }
	//NOTE: this machine's address on the router's network
	[[nodiscard]] const std::string& InsideAddress() const noexcept { return _insideAddress; }
	//NOTE: the router's own address on the internet - a public one only when Online
	[[nodiscard]] const std::string& OutsideAddress() const noexcept { return _outsideAddress; }
	//NOTE: what its port mappings are asked of - only once it is connected
	[[nodiscard]] const char* ControlUrl() const noexcept { return _urls.controlURL; }
	[[nodiscard]] const char* ServiceType() const noexcept { return _data.first.servicetype; }

private:
	UPNPUrls _urls{};
	IGDdatas _data{};
	std::string _insideAddress{};
	std::string _outsideAddress{};
	State _state{};
};
}//namespace network
