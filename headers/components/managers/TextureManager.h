#pragma once

#include "AnimationManager.h"
#include "components/EventSystem.h"
#include "enums/TextureOffset.h"
#include <memory>

enum class Direction : char8_t;
using Uint8 = uint8_t;
class BaseObj;
class EventSystem;
struct DrawObjEvent;
struct DrawAnimationEvent;

class TextureManager final
{
	std::string _name{"TextureManager"};
	TextureOffset _offset{};
	std::unique_ptr<AnimationManager> _animationManager{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Subscribe();

	void Draw(const DrawObjEvent& event) const;
	void DrawAnimation(const DrawAnimationEvent& event) const;

	[[nodiscard]] ObjRectangle GetAnimTextureRect(const std::string& name, ObjRectangle rect,
												  ObjRectangle& destRect) const;
	[[nodiscard]] ObjRectangle GetTextureRect(const std::string& name) const;
	[[nodiscard]] ObjRectangle GetTankTextureRect(const std::string& name) const;
	[[nodiscard]] ObjRectangle GetBonusTextureRect(const std::string& name) const;
	[[nodiscard]] ObjRectangle GetTextTextureRect(const std::string& name) const;

public:
	explicit TextureManager(const std::shared_ptr<EventSystem>& events);

	~TextureManager() = default;
};
