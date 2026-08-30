#pragma once

#include "AnimationManager.h"
#include "components/EventSystem.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class AnimationType : char8_t;

enum class Direction : char8_t;
using Uint8 = uint8_t;
class BaseObj;
class EventSystem;
struct DrawObjEvent;
struct DrawAnimationEvent;

class TextureManager final
{
	std::unique_ptr<AnimationManager> _animationManager{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Subscribe();

	void Draw(const DrawObjEvent& event) const;
	void DrawRim(const ObjRectangle& textureRect, const ObjRectangle& destRect, Direction dir,
				 unsigned int color) const;
	void DrawAnimation(const DrawAnimationEvent& event) const;

	//NOTE: where the frames sit in the atlas, and which way they run from there
	struct AtlasFrames
	{
		ObjRectangle first{};
		int step{1};
	};

	[[nodiscard]] AtlasFrames GetAnimFrames(AnimationType type, const std::string& name, ObjRectangle rect,
											ObjRectangle& destRect) const;
	[[nodiscard]] ObjRectangle GetTextureRect(const std::string& name) const;
	[[nodiscard]] ObjRectangle GetTankTextureRect(const std::string& name) const;
	[[nodiscard]] ObjRectangle GetBonusTextureRect(const std::string& name) const;

public:
	explicit TextureManager(const std::shared_ptr<EventSystem>& events);

	~TextureManager() = default;
};
