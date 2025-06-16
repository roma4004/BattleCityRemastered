#include "../../headers/components/TextureManager.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/pawns/Pawn.h"
#include <SDL.h>
#include <ranges>

TextureManager::TextureManager(const UPoint windowSize, std::shared_ptr<SDL_Texture> texture,
                               std::shared_ptr<SDL_Renderer> renderer, std::shared_ptr<EventSystem> events)
	: _windowSize{windowSize},
	  _renderer(std::move(renderer)),
	  _texture(std::move(texture)),
	  _events(std::move(events))
{
	Subscribe();
}

TextureManager::~TextureManager()
{
	Unsubscribe();
	ClearColorTextureCache();
}

void TextureManager::Subscribe() const
{
	_events->AddListener<const BaseObj*>("DrawObj", _name, [this](const BaseObj* baseObj) { this->Draw(baseObj); });
	_events->AddListener<const BaseObj*>("DrawHealthBarObj", _name, [this](const BaseObj* baseObj)
	{
		this->DrawHealthBar(baseObj);
	});
}

void TextureManager::Unsubscribe() const
{
	_events->RemoveListener<const BaseObj*>("DrawObj", _name);
	_events->RemoveListener<const BaseObj*>("DrawHealthBarObj", _name);
}

void TextureManager::ClearColorTextureCache() const
{
	for (const auto& texture: _colorTextureCache | std::views::values)
	{
		if (texture)
		{
			SDL_DestroyTexture(texture);
		}
	}

	_colorTextureCache.clear();
}

void TextureManager::SetRenderDrawColor(const int color, const Uint8 transparency = 255) const
{
	const Uint8 r = (color >> 16) & 0xFF;
	const Uint8 g = (color >> 8) & 0xFF;
	const Uint8 b = color & 0xFF;
	const Uint8 a = transparency;

	SDL_SetRenderDrawColor(_renderer.get(), r, g, b, a);
}

void TextureManager::DrawHealthBar(const BaseObj* obj) const
{
	//TODO: fix recenter health bar when pickup star bonus

	const ObjRectangle rect = obj->GetRect();
	const int healthWidth = obj->GetHealth() / 3;

	if (healthWidth <= 0)
		return;

	const SDL_Rect healthBarRect = {static_cast<int>(rect.x) + 2, static_cast<int>(rect.y) - 10, healthWidth, 5};

	SetRenderDrawColor(obj->GetColor(), 127);

	SDL_BlendMode blendMode;
	SDL_GetRenderDrawBlendMode(_renderer.get(), &blendMode);//backup blendmode type
	SDL_SetRenderDrawBlendMode(_renderer.get(), SDL_BLENDMODE_BLEND);//set blendmode type
	SDL_RenderFillRect(_renderer.get(), &healthBarRect);
	SDL_SetRenderDrawBlendMode(_renderer.get(), blendMode);//restore blendmode type
}

SDL_Rect TextureManager::RectToSdlRect(const ObjRectangle& rect)
{
	return SDL_Rect{
			static_cast<int>(rect.x),
			static_cast<int>(rect.y),
			static_cast<int>(rect.w),
			static_cast<int>(rect.h)};
}

SDL_Texture* TextureManager::CreateColorTexture(const int color) const
{
	if (const auto it = _colorTextureCache.find(color);
		it != _colorTextureCache.end())
	{
		return it->second;
	}

	SDL_Texture* colorTexture =
			SDL_CreateTexture(_renderer.get(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 1, 1);

	SDL_SetRenderTarget(_renderer.get(), colorTexture);

	SetRenderDrawColor(color);

	SDL_RenderClear(_renderer.get());// Fill rect with color

	SDL_SetRenderTarget(_renderer.get(), nullptr);

	_colorTextureCache[color] = colorTexture;

	return colorTexture;
}

void TextureManager::RectDraw(const BaseObj* obj) const
{
	const ObjRectangle rect = obj->GetRect();
	const int color = obj->GetColor();
	const SDL_Rect destRect = RectToSdlRect(rect);

	SDL_Texture* colorTexture = CreateColorTexture(color);

	SDL_RenderCopy(_renderer.get(), colorTexture, nullptr, &destRect);
}

void TextureManager::Draw(const BaseObj* obj) const
{
	const ObjRectangle rect = obj->GetRect();
	const SDL_Rect destRect = RectToSdlRect(rect);

	SDL_Rect textureRect{};
	const auto pawn = dynamic_cast<const Pawn*>(obj);
	if (const auto& name = obj->GetName();//TODO: replace with enum
		name == "Enemy1" || name == "Enemy2" || name == "Enemy3" || name == "Enemy4")
	{
		textureRect = RectToSdlRect(_offset.enemy);
		textureRect.x += pawn->_animationId * 16;
	}
	else if (name == "Player1" || name == "CoopBot1")
	{
		textureRect = RectToSdlRect(_offset.playerOne);
		textureRect.x += pawn->_animationId * 16;
	}
	else if (name == "Player2" || name == "CoopBot2")
	{
		textureRect = RectToSdlRect(_offset.playerTwo);
		textureRect.x += pawn->_animationId * 16;
	}
	else if (name == "Bullet")
	{
		textureRect = RectToSdlRect(_offset.bullet);
	}
	else if (name == "Eagle")
	{
		textureRect = RectToSdlRect(_offset.eagle);
	}
	else if (name == "BrickWall")
	{
		textureRect = RectToSdlRect(_offset.brick);
	}
	else if (name == "SteelWall")
	{
		textureRect = RectToSdlRect(_offset.steel);
	}
	else if (name == "Grass")
	{
		textureRect = RectToSdlRect(_offset.grass);
	}
	else if (name == "Ice")
	{
		textureRect = RectToSdlRect(_offset.ice);
	}
	else if (name == "Water")
	{
		const auto water = dynamic_cast<const WaterTile*>(obj);
		textureRect = RectToSdlRect(_offset.water);
		textureRect.x -= water->_animFrame;
	}
	else if (name == "BonusHelmet")
	{
		textureRect = RectToSdlRect(_offset.bonusHelmet);
	}
	else if (name == "BonusTimer")
	{
		textureRect = RectToSdlRect(_offset.bonusTimer);
	}
	else if (name == "BonusShovel")
	{
		textureRect = RectToSdlRect(_offset.bonusShovel);
	}
	else if (name == "BonusStar")
	{
		textureRect = RectToSdlRect(_offset.bonusStar);
	}
	else if (name == "BonusGrenade")
	{
		textureRect = RectToSdlRect(_offset.bonusGrenade);
	}
	else if (name == "BonusTank")
	{
		textureRect = RectToSdlRect(_offset.bonusTank);
	}
	/*else if (name == "BonusCaliber") 
	{
		textureRect = RectToSdlRect(_offset.bonusCaliber);
	}*/
	else
	{
		RectDraw(obj);
	}

	//local angle and flip for texture
	double angle = 0.0;
	SDL_RendererFlip flip = SDL_FLIP_NONE;

	if (pawn != nullptr)
	{
		switch (pawn->GetDirection())
		{
			case UP:
				angle = 0;
				flip = SDL_FLIP_NONE;
				break;

			case LEFT:
				angle = -90;
				flip = SDL_FLIP_NONE;
				break;

			case DOWN:
				angle = 0;
				flip = SDL_FLIP_VERTICAL;
				break;

			case RIGHT:
				angle = 90;
				flip = SDL_FLIP_NONE;
				break;
		}
	}

	//TODO: move work with sdl to utils to reduce dependencies
	SDL_RenderCopyEx(_renderer.get(), _texture.get(), &textureRect, &destRect, angle, nullptr, flip);
}
