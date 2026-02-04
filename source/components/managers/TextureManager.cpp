#include "components/managers/TextureManager.h"
#include "components/EventSystem.h"
#include "components/managers/AnimationManager.h"
#include "entities/pawns/Pawn.h"
#include "enums/Direction.h"

TextureManager::TextureManager(const UPoint windowSize, const std::shared_ptr<EventSystem>& events)
	: _windowSize{windowSize},
	  _animationManager{std::make_unique<AnimationManager>(events)},
	  _events{events}
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
	_events->AddListener(
			"DrawTankAnimation", _name,
			[this](const ObjRectangle rect, const Direction dir, const int step, const int scale,
			       const std::string& name, const unsigned int color)
			{
				this->DrawTankAnimation(rect, dir, step, scale, name, color);
			});
}

void TextureManager::Unsubscribe() const
{
	_events->RemoveListener("DrawObj", _name);
	_events->RemoveListener("DrawAnimation", _name);
}

ObjRectangle TextureManager::GetTextureRect(const std::string& name) const
{
	ObjRectangle textureRect{};
	if (name == "Bullet")//TODO: replace with enum TextureType
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
	else if (name == "BonusHelmet")
	{
		textureRect = _offset.bonusHelmet;
	}
	else if (name == "BonusTimer")
	{
		textureRect = _offset.bonusTimer;
	}
	else if (name == "BonusShovel")
	{
		textureRect = _offset.bonusShovel;
	}
	else if (name == "BonusStar")
	{
		textureRect = _offset.bonusStar;
	}
	else if (name == "BonusGrenade")
	{
		textureRect = _offset.bonusGrenade;
	}
	else if (name == "BonusTank")
	{
		textureRect = _offset.bonusTank;
	}
	else if (name == "BonusCaliber")
	{
		textureRect = _offset.bonusCaliber;
	}
	else if (name == "PauseText")
	{
		textureRect = _offset.pauseText;
	}
	else if (name == "GameOverText")
	{
		textureRect = _offset.gameOverText;
	}

	return textureRect;
}

ObjRectangle TextureManager::GetTankTextureRect(const std::string& name) const
{
	ObjRectangle textureRect{};
	if (name == "Enemy1" || name == "Enemy2" || name == "Enemy3" || name == "Enemy4")
	{
		textureRect = _offset.enemy;
	}
	else if (name == "Player1" || name == "CoopBot1")
	{
		textureRect = _offset.playerOne;
	}
	else if (name == "Player2" || name == "CoopBot2")
	{
		textureRect = _offset.playerTwo;
	}

	return textureRect;
}

ObjRectangle TextureManager::GetAnimTextureRect(const std::string& name, const ObjRectangle rect,
                                                ObjRectangle& destRect) const
{
	ObjRectangle textureRect{};
	if (name == "Water")
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
		textureRect.x == defaultSdlRect.x
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
		textureRect.x == defaultSdlRect.x && textureRect.y == defaultSdlRect.y
		//TODO: incorrect float comparison in whole class
		&& textureRect.w == defaultSdlRect.w && textureRect.h == defaultSdlRect.h)
	{
		_events->EmitEvent("RenderColorTexture", rect, color);
		//NOTE: fallback draw to non-texture, rectangle filled by color
	}

	_events->EmitEvent("RenderTexture", textureRect, destRect, dir);
}

void TextureManager::DrawTankAnimation(const ObjRectangle destRect, const Direction dir, const int step,
                                       const int scale, const std::string& name, const unsigned int color) const
{
	ObjRectangle textureRect = GetTankTextureRect(name);
	textureRect.x += step * scale;
	if (constexpr ObjRectangle defaultSdlRect{};
		textureRect.x == defaultSdlRect.x && textureRect.y == defaultSdlRect.y
		&& textureRect.w == defaultSdlRect.w && textureRect.h == defaultSdlRect.h)
	{
		_events->EmitEvent("RenderColorTexture", destRect, color);
		//NOTE: fallback draw to non-texture, rectangle filled by color
	}

	_events->EmitEvent("RenderTexture", textureRect, destRect, dir);
}
