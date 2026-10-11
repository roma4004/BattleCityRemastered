#pragma once

#include <cstddef>

//NOTE: in paint order - driven over, standing on, flashing over them, covering
enum class DrawLayer : char8_t
{
	Ground,
	World,
	//NOTE: under the bush - cover is meant to hide a helmet as much as the tank wearing it
	Effects,
	Overlay
};

inline constexpr std::size_t kDrawLayerCount{static_cast<std::size_t>(DrawLayer::Overlay) + 1u};
