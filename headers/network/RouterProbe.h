#pragma once

#include "enums/RouterState.h"
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace network
{
struct FoundRouter final
{
	RouterState state{};
	//NOTE: this machine's address on the router's network - what a port is forwarded to
	std::string insideAddress{};
	//NOTE: the router's own address on the internet
	std::string outsideAddress{};
	//NOTE: what a server hands to Router, so it opens its port without a search of its own - empty when none answered
	std::string descriptionUrl{};
};

//NOTE: looked for while the host sets the match up, so the port opens the moment the server starts
class RouterProbe final
{
public:
	RouterProbe();

	RouterProbe(const RouterProbe&) = delete;
	RouterProbe& operator=(const RouterProbe&) = delete;
	RouterProbe(RouterProbe&&) = delete;
	RouterProbe& operator=(RouterProbe&&) = delete;

	//NOTE: once, when the search is over
	[[nodiscard]] std::optional<FoundRouter> Poll();

private:
	//NOTE: shared, as nobody waits for the thread
	struct Handover
	{
		std::mutex mutex{};
		std::optional<FoundRouter> found{};
	};

	std::shared_ptr<Handover> _handover{};
};
}//namespace network
