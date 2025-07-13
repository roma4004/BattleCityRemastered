#include "components/managers/TextureManager.h"
#include "components/EventSystem.h"
#include "components/managers/AnimationManager.h"
#include "entities/pawns/Pawn.h"
#include "enums/AnimationType.h"
#include "enums/Direction.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <ranges>

TextureManager::TextureManager(const UPoint windowSize, std::shared_ptr<SDL_Texture> texture,
                               std::shared_ptr<SDL_Renderer> renderer,
                               std::shared_ptr<TTF_Font> fpsFont, std::shared_ptr<EventSystem> events)
	: _windowSize{windowSize},
	  _renderer(std::move(renderer)),
	  _texture(std::move(texture)),
	  _events(events),
	  _fpsFont{std::move(fpsFont)},
	  _fpsRectangle{.x = static_cast<int>(windowSize.x) - 80, .y = 20, .w = 40, .h = 40},
	  _animationManager(events)
{
	GenerateFpsTextures();

	Subscribe();
}

TextureManager::~TextureManager()
{
	Unsubscribe();
	ClearFpsTextureCache();
	ClearColorTextureCache();
}

void TextureManager::Subscribe()
{
	_events->AddListener("DrawObj", _name, [this](const BaseObj* baseObj) { this->Draw(baseObj); });
	_events->AddListener("DrawHealthBarObj", _name, [this](const BaseObj* baseObj)
	{
		this->DrawHealthBar(baseObj);
	});
}

void TextureManager::Unsubscribe() const
{
	_events->RemoveListener("DrawObj", _name);
	_events->RemoveListener("DrawHealthBarObj", _name);
}

void TextureManager::ClearColorTextureCache()
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

void TextureManager::ClearFpsTextureCache()
{
	for (const auto& texture: _fpsTextures | std::views::values)
	{
		if (texture)
		{
			SDL_DestroyTexture(texture);
		}
	}

	_fpsTextures.clear();
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

SDL_Texture* TextureManager::CreateColorTexture(const int color)
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

void TextureManager::RectDraw(const BaseObj* obj)
{
	const ObjRectangle rect = obj->GetRect();
	const int color = obj->GetColor();
	const SDL_Rect destRect = RectToSdlRect(rect);

	SDL_Texture* colorTexture = CreateColorTexture(color);

	SDL_RenderCopy(_renderer.get(), colorTexture, nullptr, &destRect);
}

void TextureManager::Draw(const BaseObj* obj)
{
	const ObjRectangle rect = obj->GetRect();
	SDL_Rect destRect = RectToSdlRect(rect);

	SDL_Rect textureRect{};
	const auto pawn = dynamic_cast<const Pawn*>(obj);
	if (const auto& name = obj->GetName();//TODO: replace with enum
		name == "Enemy1" || name == "Enemy2" || name == "Enemy3" || name == "Enemy4")
	{
		textureRect = RectToSdlRect(_offset.enemy);
		textureRect.x += _animationManager.GetFrame(obj->GetUuid(), AnimationType::Tank_Animation) * 16;
	}
	else if (name == "Player1" || name == "CoopBot1")
	{
		textureRect = RectToSdlRect(_offset.playerOne);
		textureRect.x += _animationManager.GetFrame(obj->GetUuid(), AnimationType::Tank_Animation) * 16;
	}
	else if (name == "Player2" || name == "CoopBot2")
	{
		textureRect = RectToSdlRect(_offset.playerTwo);
		textureRect.x += _animationManager.GetFrame(obj->GetUuid(), AnimationType::Tank_Animation) * 16;
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
		textureRect = RectToSdlRect(_offset.water);
		textureRect.x -= _animationManager.GetWaterFrame();
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
	else if (name == "BonusCaliber")
	{
		textureRect = RectToSdlRect(_offset.bonusCaliber);
	}
	else if (name == "BulletExplosion")
	{
		if (int frame = _animationManager.GetFrame(obj->GetUuid(), AnimationType::Bullet_Explosion); frame != -1)
		{
			textureRect = RectToSdlRect(_offset.smallExplosion);
			destRect = RectToSdlRect(rect.GetScale(4.f).GetCenter());
			textureRect.x += frame * 16;
		}
		else
			return;
	}
	else if (name == "TankExplosion")
	{
		textureRect = RectToSdlRect(_offset.bigExplosion);
		textureRect.x += _animationManager.GetFrame(obj->GetUuid(), AnimationType::Tank_Explosion) * 32;
	}
	else if (name == "SpawnAnimation")
	{
		textureRect = RectToSdlRect(_offset.spawnAnim);
		textureRect.x += _animationManager.GetFrame(obj->GetUuid(), AnimationType::Spawn_Animation) * 16;
	}
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
			case Direction::UP:
				angle = 0;
				flip = SDL_FLIP_NONE;
				break;

			case Direction::LEFT:
				angle = -90;
				flip = SDL_FLIP_NONE;
				break;

			case Direction::DOWN:
				angle = 0;
				flip = SDL_FLIP_VERTICAL;
				break;

			case Direction::RIGHT:
				angle = 90;
				flip = SDL_FLIP_NONE;
				break;
		}
	}

	//TODO: move work with sdl to utils to reduce dependencies
	SDL_RenderCopyEx(_renderer.get(), _texture.get(), &textureRect, &destRect, angle, nullptr, flip);
}

void TextureManager::GenerateFpsTextures()
{
	_fpsTextures.clear();

	for (size_t i = 0u; i <= 1000u; ++i)
	{
		std::string text = std::to_string(i);
		constexpr SDL_Color textColor = {140, 0, 255, 255};

		SDL_Surface* surface = TTF_RenderText_Solid(_fpsFont.get(), text.c_str(), textColor);
		if (!surface)
		{
			SDL_Log("Failed to create surface for FPS %d: %s", i, SDL_GetError());
			continue;
		}

		SDL_Texture* texture = SDL_CreateTextureFromSurface(_renderer.get(), surface);
		SDL_FreeSurface(surface);

		if (!texture)
		{
			SDL_Log("Failed to create texture for FPS %d: %s", i, SDL_GetError());
			continue;
		}

		_fpsTextures[i] = texture;
	}
}

void TextureManager::ClearFrame() const
{
	SDL_SetRenderDrawColor(_renderer.get(), 0, 0, 0, 255);
	SDL_RenderClear(_renderer.get());
}

void TextureManager::DisplayFrame(const size_t fps)
{
	if (fps)
	{
		// Copy the texture with FPS to the renderer
		SDL_RenderCopy(_renderer.get(), _fpsTextures[fps], nullptr, &_fpsRectangle);
	}

	SDL_RenderPresent(_renderer.get());
}
