#pragma once

class INetworkNode
{
public:
	INetworkNode() = default;
	virtual ~INetworkNode() = default;

	virtual void ProcessNetworkCommands() = 0;
};
