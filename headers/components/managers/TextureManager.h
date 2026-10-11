#pragma once

#include "geometry/ObjRectangle.h"
#include <memory>

enum class AnimationType : char8_t;
enum class Direction : char8_t;
struct Sprite;
class AnimatedObject;
class EventSystem;

class TextureManager final
{
	std::shared_ptr<EventSystem> _events{nullptr};

	void DrawRim(const ObjRectangle& textureRect, const ObjRectangle& destRect, Direction dir,
				 unsigned int color) const;

	//NOTE: where the frames sit in the atlas, and which way they run from there
	struct AtlasFrames
	{
		ObjRectangle first{};
		int step{1};
	};

	[[nodiscard]] static AtlasFrames GetAnimFrames(AnimationType type, ObjRectangle rect, ObjRectangle& destRect);

public:
	explicit TextureManager(const std::shared_ptr<EventSystem>& events);

	//NOTE: called by the scene painter directly, not over the bus
	void Draw(const Sprite& sprite) const;
	void Draw(const AnimatedObject& animation) const;
};
