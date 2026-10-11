#pragma once

//NOTE: what became of asking for the router this machine reaches the internet through
enum class RouterState : char8_t
{
	//NOTE: nothing answered - nothing on the network, or not the router a search found before
	Silent,
	//NOTE: something answered, but no router connected to the internet
	Offline,
	//NOTE: connected, its outside address a provider's (100.64.0.0/10) - that NAT lets nobody in, whatever is opened
	BehindProvider,
	//NOTE: connected, its outside address a private one - the router in front has to pass the port on as well
	BehindRouter,
	Online
};
