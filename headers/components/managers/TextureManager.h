#pragma once

#include "components/EventSystem.h"
#include "enums/Author.h"
#include "geometry/ObjRectangle.h"
#include <memory>
#include <vector>

enum class AnimationType : char8_t;
enum class Direction : char8_t;
enum class TankModel : char8_t;
struct Sprite;
struct DrawAnimationEvent;
class BaseObj;
class EventSystem;

class TextureManager final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Subscribe();

	void DrawRim(const ObjRectangle& textureRect, const ObjRectangle& destRect, Direction dir,
				 unsigned int color) const;
	void DrawAnimation(const DrawAnimationEvent& event) const;

	//NOTE: where the frames sit in the atlas, and which way they run from there
	struct AtlasFrames
	{
		ObjRectangle first{};
		int step{1};
	};

	[[nodiscard]] static AtlasFrames GetAnimFrames(AnimationType type, Author author, TankModel model,
												   unsigned short tier, ObjRectangle rect, ObjRectangle& destRect);

public:
	explicit TextureManager(const std::shared_ptr<EventSystem>& events);

	//NOTE: called by the scene painter directly, not over the bus
	void Draw(const Sprite& sprite) const;
};
