#include "components/managers/TextureManager.h"
#include "enums/TextureOffset.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/managers/AnimationManager.h"
#include "utils/ColliderUtils.h"
#include "utils/Log.h"

namespace
{
constexpr double kAtlasCellSize{16.0};
constexpr int kBonusSpawnFrames{3};
//NOTE: the bonus icon is a 15x14 box inside its 16x16 cell, framed by a single atlas pixel
constexpr double kBonusBoxWidth{15.0};
constexpr double kBonusBoxHeight{14.0};
}//namespace

TextureManager::TextureManager(const std::shared_ptr<EventSystem>& events)
	: _animationManager{std::make_unique<AnimationManager>(events)}
	, _events{events}
{
	Subscribe();
}

void TextureManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &TextureManager::Draw));
	_subs.push_back(_events->AddListener(this, &TextureManager::DrawAnimation));
}

//NOTE: entity names only - DrawObjEvent comes from Bullet, Obstacle and Bonus. UI reads
//TextureOffset directly and never lands here.
ObjRectangle TextureManager::GetTextureRect(const std::string& name) const
{
	ObjRectangle textureRect{};
	if (name.starts_with("Bonus"))
	{
		textureRect = GetBonusTextureRect(name);
	}
	else if (name == "Bullet")//TODO: replace with enum TextureType
	{
		textureRect = TextureOffset::kBullet;
	}
	else if (name == "Eagle")
	{
		textureRect = TextureOffset::kEagle;
	}
	else if (name == "BrickWall")
	{
		textureRect = TextureOffset::kBrick;
	}
	else if (name == "SteelWall")
	{
		textureRect = TextureOffset::kSteel;
	}
	else if (name == "Bush")
	{
		textureRect = TextureOffset::kBush;
	}
	else if (name == "Ice")
	{
		textureRect = TextureOffset::kIce;
	}

	return textureRect;
}

ObjRectangle TextureManager::GetTankTextureRect(const std::string& name) const
{
	ObjRectangle textureRect{};
	if (name.starts_with("Enemy"))
	{
		textureRect = TextureOffset::kEnemy;
	}
	else if (name.ends_with("1"))
	{
		textureRect = TextureOffset::kPlayerOne;
	}
	else if (name.ends_with("2"))
	{
		textureRect = TextureOffset::kPlayerTwo;
	}

	return textureRect;
}

ObjRectangle TextureManager::GetBonusTextureRect(const std::string& name) const
{
	if (name.ends_with("Helmet"))
	{
		return TextureOffset::kBonusHelmet;
	}
	else if (name.ends_with("Timer"))
	{
		return TextureOffset::kBonusTimer;
	}
	else if (name.ends_with("Shovel"))
	{
		return TextureOffset::kBonusShovel;
	}
	else if (name.ends_with("Star"))
	{
		return TextureOffset::kBonusStar;
	}
	else if (name.ends_with("Grenade"))
	{
		return TextureOffset::kBonusGrenade;
	}
	else if (name.ends_with("Tank"))
	{
		return TextureOffset::kBonusTank;
	}
	else if (name.ends_with("Caliber"))
	{
		return TextureOffset::kBonusCaliber;
	}
	else if (name.ends_with("Ship"))
	{
		return TextureOffset::kBonusShip;
	}

	Log::Error("TextureManager::GetBonusTextureRect: unrecognized bonus name '" + name + "'");
	return ObjRectangle{};
}

TextureManager::AtlasFrames TextureManager::GetAnimFrames(const AnimationType type, const std::string& name,
														  const ObjRectangle rect, ObjRectangle& destRect) const
{
	switch (type)
	{
		case AnimationType::Tank_Move:
			return AtlasFrames{.first = GetTankTextureRect(name)};
		case AnimationType::Water_Flow:
			//NOTE: the water frames sit to the left of the offset, so they are walked backwards
			return AtlasFrames{.first = TextureOffset::kWater, .step = -1};
		case AnimationType::Bullet_Explosion:
			destRect = rect.GetScaledBy(3.0);
			return AtlasFrames{.first = TextureOffset::kBulletExplosion};
		case AnimationType::Tank_Explosion:
			destRect = rect.GetScaledBy(1.3);
			return AtlasFrames{.first = TextureOffset::kTankExplosion};
		case AnimationType::Tank_Spawn:
			return AtlasFrames{.first = TextureOffset::kTankSpawn};
		case AnimationType::Bonus_Spawn:
		{
			//NOTE: the tank spawn burst entered from its last frame and walked backwards, so the bonus
			//shrinks into place instead of blooming out of it
			ObjRectangle lastFrame = TextureOffset::kTankSpawn;
			lastFrame.x += (kBonusSpawnFrames - 1) * kAtlasCellSize;

			return AtlasFrames{.first = lastFrame, .step = -1};
		}
		case AnimationType::Helmet_Effect:
			return AtlasFrames{.first = TextureOffset::kHelmetEffect};
		case AnimationType::Count:
			break;
	}

	Log::Error("TextureManager::GetAnimFrames: unrecognized animation type");

	return AtlasFrames{};
}

void TextureManager::Draw(const DrawObjEvent& event) const
{
	const auto& [rect, dir, name, rimColor] = event;
	const ObjRectangle destRect = rect;
	const ObjRectangle textureRect = GetTextureRect(name);
	if (constexpr ObjRectangle defaultSdlRect{};
		ColliderUtils::AreEqualAbsolute(textureRect.x, defaultSdlRect.x)
		&& ColliderUtils::AreEqualAbsolute(textureRect.y, defaultSdlRect.y)
		&& ColliderUtils::AreEqualAbsolute(textureRect.w, defaultSdlRect.w)
		&& ColliderUtils::AreEqualAbsolute(textureRect.h, defaultSdlRect.h))
	{
		//NOTE: fallback draw to non-texture, rectangle filled by color
		_events->EmitEvent(RenderColorTextureEvent{.rect = rect});
	}

	_events->EmitEvent(RenderTextureEvent{.textureRect = textureRect, .destRect = destRect, .dir = dir});

	if (rimColor != 0u)
	{
		DrawRim(textureRect, destRect, dir, rimColor);
	}
}

//NOTE: the frame is one atlas pixel of the sprite's own outline, so it is redrawn from the atlas rather
//than stroked over - that way it lands exactly on the pixels it recolors, at any scale
void TextureManager::DrawRim(const ObjRectangle& textureRect, const ObjRectangle& destRect, const Direction dir,
							 const unsigned int color) const
{
	constexpr double thickness{1.0};
	const double scaleX = destRect.w / kAtlasCellSize;
	const double scaleY = destRect.h / kAtlasCellSize;

	const auto emitSlice = [this, &textureRect, &destRect, scaleX, scaleY, dir, color]
	(const double x, const double y, const double w, const double h)
	{
		_events->EmitEvent(
				RenderTextureEvent{
						.textureRect = {.x = textureRect.x + x, .y = textureRect.y + y, .w = w, .h = h},
						.destRect = {.x = destRect.x + x * scaleX,
									 .y = destRect.y + y * scaleY,
									 .w = w * scaleX,
									 .h = h * scaleY},
						.dir = dir,
						.color = color});
	};

	emitSlice(0.0, 0.0, kBonusBoxWidth, thickness);
	emitSlice(0.0, kBonusBoxHeight - thickness, kBonusBoxWidth, thickness);
	emitSlice(0.0, 0.0, thickness, kBonusBoxHeight);
	emitSlice(kBonusBoxWidth - thickness, 0.0, thickness, kBonusBoxHeight);
}

void TextureManager::DrawAnimation(const DrawAnimationEvent& event) const
{
	const auto& [rect, dir, frame, scale, type, name] = event;
	ObjRectangle destRect = rect;
	auto [textureRect, step] = GetAnimFrames(type, name, rect, destRect);
	textureRect.x += static_cast<double>(frame * scale * step);
	if (constexpr ObjRectangle defaultSdlRect{};
		ColliderUtils::AreEqualAbsolute(textureRect.x, defaultSdlRect.x)
		&& ColliderUtils::AreEqualAbsolute(textureRect.y, defaultSdlRect.y)
		&& ColliderUtils::AreEqualAbsolute(textureRect.w, defaultSdlRect.w)
		&& ColliderUtils::AreEqualAbsolute(textureRect.h, defaultSdlRect.h))
	{
		//NOTE: fallback draw to non-texture, rectangle filled by color
		_events->EmitEvent(RenderColorTextureEvent{.rect = rect});
	}

	_events->EmitEvent(RenderTextureEvent{.textureRect = textureRect, .destRect = destRect, .dir = dir});
}
