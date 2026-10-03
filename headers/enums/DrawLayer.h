#pragma once

#include <cstddef>

//NOTE: in paint order - driven over, standing on, covering
enum class DrawLayer : char8_t
{
	Ground,
	World,
	Overlay
};

inline constexpr std::size_t kDrawLayerCount{static_cast<std::size_t>(DrawLayer::Overlay) + 1u};
