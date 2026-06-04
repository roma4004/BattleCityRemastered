#pragma once

#include "AnimationManager.h"
#include "Point.h"
#include "enums/TextureOffset.h"
#include <memory>

enum class Direction : char8_t;
using Uint8 = uint8_t;
class BaseObj;
class EventSystem;

class TextureManager final
{
	std::string _name{"TextureManager"};
	UPoint _windowSize{};
	TextureOffset _offset{};
	std::unique_ptr<AnimationManager> _animationManager{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};

	void Subscribe() const;
	void Unsubscribe() const;

	void Draw(ObjRectangle rect, Direction dir, const std::string& name, unsigned int color) const;
	void DrawAnimation(ObjRectangle rect, Direction dir, int step, int scale, const std::string& name,
					   unsigned int color) const;

	[[nodiscard]] ObjRectangle GetAnimTextureRect(const std::string& name, ObjRectangle rect,
												  ObjRectangle& destRect) const;
	[[nodiscard]] ObjRectangle GetTextureRect(const std::string& name) const;
	[[nodiscard]] ObjRectangle GetTankTextureRect(const std::string& name) const;
	[[nodiscard]] ObjRectangle GetBonusTextureRect(const std::string& name) const;
	[[nodiscard]] ObjRectangle GetTextTextureRect(const std::string& name) const;

public:
	TextureManager(UPoint windowSize, const std::shared_ptr<EventSystem>& events);

	~TextureManager();
};
