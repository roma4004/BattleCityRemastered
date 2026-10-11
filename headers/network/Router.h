#pragma once

#include "enums/RouterState.h"
#include <igd_desc_parse.h>
#include <miniupnpc.h>
#include <string>

namespace network
{
//NOTE: the router to the internet, asked over UPnP while this is built - seconds, so it is built on a thread
class Router final
{
public:
	//NOTE: looked for on the network - eight seconds when nothing answers
	Router();
	//NOTE: the one a search found before, asked at once where its description lies
	explicit Router(const std::string& descriptionUrl);
	~Router();

	Router(const Router&) = delete;
	Router& operator=(const Router&) = delete;
	Router(Router&&) = delete;
	Router& operator=(Router&&) = delete;

	[[nodiscard]] RouterState GetState() const noexcept { return _state; }
	//NOTE: this machine's address on the router's network
	[[nodiscard]] const std::string& InsideAddress() const noexcept { return _insideAddress; }
	//NOTE: the router's own address on the internet - a public one only when Online
	[[nodiscard]] const std::string& OutsideAddress() const noexcept { return _outsideAddress; }
	//NOTE: what finds this router again without a search - empty when none answered
	[[nodiscard]] std::string DescriptionUrl() const;
	//NOTE: what its port mappings are asked of - only once it is connected
	[[nodiscard]] const char* ControlUrl() const noexcept { return _urls.controlURL; }
	[[nodiscard]] const char* ServiceType() const noexcept { return _data.first.servicetype; }

private:
	UPNPUrls _urls{};
	IGDdatas _data{};
	std::string _insideAddress{};
	std::string _outsideAddress{};
	RouterState _state{};
};
}//namespace network
