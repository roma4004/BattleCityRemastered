#include "components/managers/TextureManager.h"
#include "components/EventSystem.h"
#include "components/managers/AnimationManager.h"
#include "entities/pawns/Pawn.h"
#include "enums/Direction.h"

TextureManager::TextureManager(const UPoint windowSize, const std::shared_ptr<EventSystem>& events)
	: _windowSize{windowSize}
	, _animationManager{std::make_unique<AnimationManager>(events)}
	, _events{events}
{
	Subscribe();
}

TextureManager::~TextureManager()
{
	Unsubscribe();
}

void TextureManager::Subscribe() const
{
	//TODO: RAII for subscribe, maybe unique ptr or any wrapper for auto unsubscribe when obj die.
	_events->AddListener(
			"DrawObj", _name,
			[this](const ObjRectangle rect, const Direction dir, const std::string& name, const unsigned int color)
			{
				this->Draw(rect, dir, name, color);
			});
	_events->AddListener(
			"DrawAnimation", _name,
			[this](const ObjRectangle rect, const Direction dir, const int step, const int scale,
				   const std::string& name, const unsigned int color)
			{
				this->DrawAnimation(rect, dir, step, scale, name, color);
			});
}

void TextureManager::Unsubscribe() const { _events->RemoveAllListeners(_name); }

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
	else if (name == "Grass")
	{
		textureRect = _offset.grass;
	}
	else if (name == "Ice")
	{
		textureRect = _offset.ice;
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

	//TODO: add assert
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

	//TODO: add assert
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
		textureRect = _offset.smallExplosion;
	}
	else if (name == "TankExplosion")
	{
		destRect = rect.GetScaledBy(1.3f);
		textureRect = _offset.bigExplosion;
	}
	else if (name == "SpawnAnimation")
	{
		textureRect = _offset.spawnAnim;
	}

	return textureRect;
}


void TextureManager::Draw(const ObjRectangle rect, const Direction dir, const std::string& name,
						  const unsigned int color) const
{
	const ObjRectangle destRect = rect;
	const ObjRectangle textureRect = GetTextureRect(name);
	if (constexpr ObjRectangle defaultSdlRect{};
		textureRect.x == defaultSdlRect.x//TODO: fix correct float conversion
		&& textureRect.y == defaultSdlRect.y
		&& textureRect.w == defaultSdlRect.w
		&& textureRect.h == defaultSdlRect.h)
	{
		_events->EmitEvent("RenderColorTexture", rect, color);
		//NOTE: fallback draw to non-texture, rectangle filled by color
	}

	_events->EmitEvent("RenderTexture", textureRect, destRect, dir);
}

void TextureManager::DrawAnimation(const ObjRectangle rect, const Direction dir, const int step, const int scale,
								   const std::string& name, const unsigned int color) const
{
	ObjRectangle destRect = rect;
	ObjRectangle textureRect = GetAnimTextureRect(name, rect, destRect);
	textureRect.x += step * scale;
	if (constexpr ObjRectangle defaultSdlRect{};
		textureRect.x == defaultSdlRect.x
		&& textureRect.y == defaultSdlRect.y//TODO: incorrect float comparison in whole class
		&& textureRect.w == defaultSdlRect.w
		&& textureRect.h == defaultSdlRect.h)
	{
		_events->EmitEvent("RenderColorTexture", rect, color);
		//NOTE: fallback draw to non-texture, rectangle filled by color
	}

	_events->EmitEvent("RenderTexture", textureRect, destRect, dir);
}