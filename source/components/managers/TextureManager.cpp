#include "components/managers/TextureManager.h"
#include "enums/AnimationType.h"
#include "enums/TextureOffset.h"
#include "enums/TextureType.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
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
	: _events{events}
{
	Subscribe();
}

void TextureManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &TextureManager::Draw));
	_subs.push_back(_events->AddListener(this, &TextureManager::DrawAnimation));
}

//NOTE: entity sprites only - DrawObjEvent comes from Bullet, Obstacle and Bonus. UI reads
//TextureOffset directly and never lands here.
ObjRectangle TextureManager::GetTextureRect(const TextureType texture)
{
	switch (texture)
	{
		case TextureType::Bullet:
			return TextureOffset::kBullet;
		case TextureType::Eagle:
			return TextureOffset::kEagle;
		case TextureType::BrickWall:
			return TextureOffset::kBrick;
		case TextureType::SteelWall:
			return TextureOffset::kSteel;
		case TextureType::Bush:
			return TextureOffset::kBush;
		case TextureType::Ice:
			return TextureOffset::kIce;
		case TextureType::BonusTimer:
			return TextureOffset::kBonusTimer;
		case TextureType::BonusHelmet:
			return TextureOffset::kBonusHelmet;
		case TextureType::BonusGrenade:
			return TextureOffset::kBonusGrenade;
		case TextureType::BonusTank:
			return TextureOffset::kBonusTank;
		case TextureType::BonusStar:
			return TextureOffset::kBonusStar;
		case TextureType::BonusShovel:
			return TextureOffset::kBonusShovel;
		case TextureType::BonusCaliber:
			return TextureOffset::kBonusCaliber;
		case TextureType::BonusShip:
			return TextureOffset::kBonusShip;
		case TextureType::None:
			break;
	}

	Log::Error("TextureManager::GetTextureRect: nothing to draw for this texture");

	return ObjRectangle{};
}

ObjRectangle TextureManager::GetTankTextureRect(const Author author)
{
	switch (author)
	{
		case Author::Enemy1:
		case Author::Enemy2:
		case Author::Enemy3:
		case Author::Enemy4:
			return TextureOffset::kEnemy;
		case Author::Player1:
			return TextureOffset::kPlayer1;
		case Author::Player2:
			return TextureOffset::kPlayer2;
		case Author::None:
		case Author::lastId:
			break;
	}

	return ObjRectangle{};
}

TextureManager::AtlasFrames TextureManager::GetAnimFrames(const AnimationType type, const Author author,
														  const ObjRectangle rect, ObjRectangle& destRect) const
{
	switch (type)
	{
		case AnimationType::Tank_Move:
			return AtlasFrames{.first = GetTankTextureRect(author)};
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
	const auto& [rect, dir, texture, rimColor] = event;
	const ObjRectangle destRect = rect;
	const ObjRectangle textureRect = GetTextureRect(texture);
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
	const auto& [rect, dir, frame, scale, type, author] = event;
	ObjRectangle destRect = rect;
	auto [textureRect, step] = GetAnimFrames(type, author, rect, destRect);
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
