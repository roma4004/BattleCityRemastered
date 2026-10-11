#include "network/RouterProbe.h"
#include "network/Endpoints.h"
#include "network/Router.h"
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

namespace network
{
RouterProbe::RouterProbe()
	: _handover{std::make_shared<Handover>()}
{
	//NOTE: let go of - a router that answers and then keeps silent holds it twenty seconds, longer than a close waits
	std::thread{[handover = _handover]
	{
		const Router router{};
		//NOTE: with no router to say it, this machine's own on its network is the one a port is forwarded to
		FoundRouter found{.state = router.GetState(),
						  .insideAddress = router.InsideAddress().empty() ? LocalAddress() : router.InsideAddress(),
						  .outsideAddress = router.OutsideAddress(),
						  .descriptionUrl = router.DescriptionUrl()};
		const std::scoped_lock lock{handover->mutex};
		handover->found = std::move(found);
	}}.detach();
}

std::optional<FoundRouter> RouterProbe::Poll()
{
	const std::scoped_lock lock{_handover->mutex};

	return std::exchange(_handover->found, std::nullopt);
}
}//namespace network
