#pragma once

#include <string>

struct ServerSendStatisticsEvent final
{
	std::string eventName;
	std::string author;
	std::string fraction;
};

struct ClientReceivedStatisticsEvent final
{
	std::string eventName;
	std::string author;
	std::string fraction;
};

struct TankStatisticsEvent final
{
	std::string who;
	std::string author;
	std::string fraction;
};
