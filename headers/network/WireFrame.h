#pragma once

#include "utils/Uuid.h"
#include <string>
#include <utility>
#include <vector>

namespace network
{
//NOTE: one frame archived by delivery - the reliable ones as a single message, every Latest one on its own,
//so it can replace the older value of its entity on the way out
struct WireFrame final
{
	std::string reliable{};
	std::vector<std::pair<Uuid, std::string>> latest{};
	//NOTE: the reliable message replaces the peer's whole field, so every Latest value sent before it is void
	bool isSnapshot{};
};
}//namespace network
