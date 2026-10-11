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
	//NOTE: a provider's NAT (CGNAT) stands between the router and the internet - the port stays closed from outside
	BehindProvider,
	//NOTE: was open, and the router would not keep it so - asked again until it does
	Lost,
	//NOTE: open behind another router - the host passes the port on there by hand, to this router's outside address
	BehindRouter,
	//NOTE: nothing came of asking for a while - still asked, and whatever comes later replaces this
	NoAnswer,

	lastId
};
