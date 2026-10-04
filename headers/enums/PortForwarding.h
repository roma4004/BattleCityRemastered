#pragma once

//NOTE: what became of the server's port on the router - the lobby says it, so the host knows whether the
//address to give out lets anybody in
enum class PortForwarding : char8_t
{
	//NOTE: the router is being looked for and asked - a few seconds
	Opening,
	Open,
	//NOTE: nothing on the network answers UPnP - turned off on the router, or no router at all
	NoRouter,
	//NOTE: the router said no - the port is taken by another machine, or nobody may open one
	Refused,
	//NOTE: the router's own outside address is not the internet's - a provider's NAT (CGNAT) or a second router
	//stands between, and a port opened here stays closed from outside
	BehindProvider,

	lastId
};
