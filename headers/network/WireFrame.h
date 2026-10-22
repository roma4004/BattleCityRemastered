#pragma once

#include "utils/Uuid.h"
#include <string>
#include <utility>
#include <vector>

namespace network
{
//NOTE: reliable commands as one message, each Latest one apart, so it replaces its entity's older value on the way out
struct WireFrame final
{
	std::string reliable{};
	std::vector<std::pair<Uuid, std::string>> latest{};
	//NOTE: the reliable message replaces the peer's whole field, so every Latest value sent before it is void
	bool isSnapshot{};
};
}//namespace network
