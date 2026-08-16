#include "components/managers/TextureManager.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/managers/AnimationManager.h"
#include "utils/ColliderUtils.h"
#include <cassert>

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

ObjRectangle TextureManager::GetTextureRect(const std::string& name) const
{
	ObjRectangle textureRect{};
	if (name.starts_with("Bonus"))
	{
		textureRect = GetBonusTextureRect(name);
	}
	else if (name.ends_with("Text"))
	{
		textureRect = GetTextTextureRect(name);
	}
	else if (name == "Bullet")//TODO: replace with enum TextureType
	{
		textureRect = _offset.bullet;
	}
	else if (name == "Eagle")
	{
		textureRect = _offset.eagle;
	}
	else if (name == "BrickWall")
	{
		textureRect = _offset.brick;
	}
	else if (name == "SteelWall")
	{
		textureRect = _offset.steel;
	}
	else if (name == "Bush")
	{
		textureRect = _offset.bush;
	}
	else if (name == "Ice")
	{
		textureRect = _offset.ice;
	}
	else if (name == "RightSideBar")
	{
		textureRect = _offset.rightSideBar;
	}
	else if (name == "EnemyIcon")
	{
		textureRect = _offset.enemyIcon;
	}
	else if (name == "EnemyIconBackground")
	{
		textureRect = _offset.enemyIconBackground;
	}
	else if (name == "PlayerOneIcon")
	{
		textureRect = _offset.playerOneIcon;
	}
	else if (name == "PlayerTwoIcon")
	{
		textureRect = _offset.playerTwoIcon;
	}
	else if (name == "StageNumberFlag")
	{
		textureRect = _offset.stageNumberFlag;
	}

	return textureRect;
}

ObjRectangle TextureManager::GetTankTextureRect(const std::string& name) const
{
	ObjRectangle textureRect{};
	if (name.starts_with("Enemy"))
	{
		textureRect = _offset.enemy;
	}
	else if (name.ends_with("1"))
	{
		textureRect = _offset.playerOne;
	}
	else if (name.ends_with("2"))
	{
		textureRect = _offset.playerTwo;
	}

	return textureRect;
}

ObjRectangle TextureManager::GetBonusTextureRect(const std::string& name) const
{
	if (name.ends_with("Helmet"))
	{
		return _offset.bonusHelmet;
	}
	else if (name.ends_with("Timer"))
	{
		return _offset.bonusTimer;
	}
	else if (name.ends_with("Shovel"))
	{
		return _offset.bonusShovel;
	}
	else if (name.ends_with("Star"))
	{
		return _offset.bonusStar;
	}
	else if (name.ends_with("Grenade"))
	{
		return _offset.bonusGrenade;
	}
	else if (name.ends_with("Tank"))
	{
		return _offset.bonusTank;
	}
	else if (name.ends_with("Caliber"))
	{
		return _offset.bonusCaliber;
	}

	assert(false && "TextureManager::GetBonusTextureRect: unrecognized bonus name suffix");
	return ObjRectangle{};
}

ObjRectangle TextureManager::GetTextTextureRect(const std::string& name) const
{
	if (name.starts_with("Pause"))
	{
		return _offset.pauseText;
	}

	if (name.starts_with("GameOver"))
	{
		return _offset.gameOverText;
	}

	if (name.starts_with("gameWon"))
	{
		return _offset.gameWonText;
	}

	assert(false && "TextureManager::GetTextTextureRect: unrecognized text name prefix");
	return ObjRectangle{};
}

ObjRectangle TextureManager::GetAnimTextureRect(const std::string& name, const ObjRectangle rect,
												ObjRectangle& destRect) const
{
	ObjRectangle textureRect{};
	if (name.ends_with("1") || name.ends_with("2") || name.ends_with("3") || name.ends_with("4"))
	{
		textureRect = GetTankTextureRect(name);
	}
	else if (name == "Water")
	{
		textureRect = _offset.water;
	}
	// else if (name == "Bullet")
	// {
	// 	textureRect = _offset.bullet);
	// }
	else if (name == "BulletExplosion")
	{
		destRect = rect.GetScaledBy(3.f);
		textureRect = _offset.bulletExplosion;
	}
	else if (name == "TankExplosion")
	{
		destRect = rect.GetScaledBy(1.3f);
		textureRect = _offset.tankExplosion;
	}
	else if (name == "TankSpawn")
	{
		textureRect = _offset.tankSpawn;
	}
	else if (name.ends_with("HelmetEffect"))
	{
		textureRect = _offset.helmetEffect;
	}

	return textureRect;
}

void TextureManager::Draw(const DrawObjEvent& event) const
{
	const auto& [rect, dir, name] = event;
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
}

void TextureManager::DrawAnimation(const DrawAnimationEvent& event) const
{
	const auto& [rect, dir, frame, scale, name] = event;
	ObjRectangle destRect = rect;
	ObjRectangle textureRect = GetAnimTextureRect(name, rect, destRect);
	const int direction = name == "Water" ? -1 : 1;//NOTE: water's frames are played back-to-front frames flow
	textureRect.x += static_cast<float>(frame * scale * direction);
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
