#include "components/managers/TextureManager.h"
#include "enums/AnimationType.h"
#include "enums/TextureOffset.h"
#include "enums/TextureType.h"
#include "components/AnimatedObjects.h"
#include "components/EventSystem.h"
#include "components/Sprite.h"
#include "components/events/AnimationRenderEvents.h"
#include "entities/TankModelSpec.h"
#include "enums/Direction.h"
#include "geometry/Point.h"
#include "utils/MathUtils.h"
#include "utils/Log.h"
#include <optional>

namespace
{
constexpr double kAtlasCellSize{16.0};
constexpr int kBonusSpawnFrames{3};

//NOTE: entity sprites only - UI reads TextureOffset directly and never lands here
ObjRectangle GetTextureRect(const TextureType texture)
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
		case TextureType::Helmet:
			return TextureOffset::kHelmetEffect;
		case TextureType::Tank:
		case TextureType::None:
			break;
	}

	Log::Error("TextureManager::GetTextureRect: nothing to draw for this texture");

	return ObjRectangle{};
}

std::optional<ObjRectangle> QuarterOf(const Author author)
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
		case Author::Player3:
			return TextureOffset::kPlayer3;
		case Author::Player4:
			return TextureOffset::kPlayer4;
		case Author::None:
		case Author::lastId:
			break;
	}

	return std::nullopt;
}

//NOTE: two frames per facing; a long barrel loses its tip
FPoint FacingShift(const Direction dir)
{
	constexpr double facing{2.0 * kAtlasCellSize};
	switch (dir)
	{
		case Direction::UP:
			return FPoint{.y = 1.0};
		case Direction::LEFT:
			return FPoint{.x = facing + 1.0};
		case Direction::DOWN:
			return FPoint{.x = 2.0 * facing};
		case Direction::RIGHT:
			return FPoint{.x = 3.0 * facing};
	}

	return FPoint{};
}

//NOTE: the seat picks the color quarter, SpriteRowOf the row in it - player tiers first, then enemy models
ObjRectangle GetTankTextureRect(const Author author, const TankModel model, const unsigned short tier,
								const Direction dir)
{
	const double rowOffset{SpriteRowOf(model, tier) * kAtlasCellSize};
	const FPoint facing{FacingShift(dir)};
	const auto placed = [rowOffset, facing](const ObjRectangle& quarter)
	{
		return ObjRectangle{.x = quarter.x + facing.x,
							.y = quarter.y + rowOffset + facing.y,
							.w = quarter.w,
							.h = quarter.h};
	};

	return QuarterOf(author).transform(placed).value_or(ObjRectangle{});
}

ObjRectangle AtlasRectOf(const Sprite& sprite)
{
	ObjRectangle rect{sprite.texture == TextureType::Tank
							  ? GetTankTextureRect(sprite.author, sprite.model, sprite.tier, sprite.dir)
							  : GetTextureRect(sprite.texture)};
	rect.x += sprite.frame * kAtlasCellSize;

	return rect;
}
}//namespace

TextureManager::TextureManager(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

TextureManager::AtlasFrames TextureManager::GetAnimFrames(const AnimationType type, const ObjRectangle rect,
														  ObjRectangle& destRect)
{
	switch (type)
	{
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
			//NOTE: the tank spawn burst played backwards from its last frame, so the bonus shrinks into place
			ObjRectangle lastFrame{TextureOffset::kTankSpawn};
			lastFrame.x += (kBonusSpawnFrames - 1) * kAtlasCellSize;

			return AtlasFrames{.first = lastFrame, .step = -1};
		}
		case AnimationType::Count:
			break;
	}

	Log::Error("TextureManager::GetAnimFrames: unrecognized animation type");

	return AtlasFrames{};
}

void TextureManager::Draw(const Sprite& sprite) const
{
	const ObjRectangle destRect{sprite.rect};
	const ObjRectangle textureRect{AtlasRectOf(sprite)};
	if (constexpr ObjRectangle defaultSdlRect{};
		MathUtils::AreEqualAbsolute(textureRect.x, defaultSdlRect.x)
		&& MathUtils::AreEqualAbsolute(textureRect.y, defaultSdlRect.y)
		&& MathUtils::AreEqualAbsolute(textureRect.w, defaultSdlRect.w)
		&& MathUtils::AreEqualAbsolute(textureRect.h, defaultSdlRect.h))
	{
		_events->EmitEvent(RenderColorTextureEvent{.rect = destRect});
	}

	//NOTE: a tank has a frame per facing, with its own light
	const Direction turn{sprite.texture == TextureType::Tank ? Direction::UP : sprite.dir};
	_events->EmitEvent(RenderTextureEvent{.textureRect = textureRect, .destRect = destRect, .dir = turn});

	if (sprite.rimColor != 0u)
	{
		DrawRim(textureRect, destRect, sprite.dir, sprite.rimColor);
	}
}

//NOTE: the rim is the sprite's outline redrawn from the atlas, not stroked, so it hits exactly its pixels at any scale
void TextureManager::DrawRim(const ObjRectangle& textureRect, const ObjRectangle& destRect, const Direction dir,
							 const unsigned int color) const
{
	constexpr double thickness{1.0};
	const double scaleX{destRect.w / kAtlasCellSize};
	const double scaleY{destRect.h / kAtlasCellSize};

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

	constexpr double width{TextureOffset::kBonusBoxWidth};
	constexpr double height{TextureOffset::kBonusBoxHeight};
	emitSlice(0.0, 0.0, width, thickness);
	emitSlice(0.0, height - thickness, width, thickness);
	emitSlice(0.0, 0.0, thickness, height);
	emitSlice(width - thickness, 0.0, thickness, height);
}

void TextureManager::Draw(const AnimatedObject& animation) const
{
	const ObjRectangle& rect{animation.rect};
	ObjRectangle destRect{rect};
	auto [textureRect, step] = GetAnimFrames(animation.type, rect, destRect);
	textureRect.x += static_cast<double>(animation.currentFrameIndex * animation.scale * step);
	if (constexpr ObjRectangle defaultSdlRect{};
		MathUtils::AreEqualAbsolute(textureRect.x, defaultSdlRect.x)
		&& MathUtils::AreEqualAbsolute(textureRect.y, defaultSdlRect.y)
		&& MathUtils::AreEqualAbsolute(textureRect.w, defaultSdlRect.w)
		&& MathUtils::AreEqualAbsolute(textureRect.h, defaultSdlRect.h))
	{
		_events->EmitEvent(RenderColorTextureEvent{.rect = rect});
	}

	_events->EmitEvent(RenderTextureEvent{.textureRect = textureRect, .destRect = destRect, .dir = animation.dir});
}
