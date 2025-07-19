#include "components/managers/TextureManager.h"
#include "components/EventSystem.h"
#include "components/managers/AnimationManager.h"
#include "entities/pawns/Pawn.h"
#include "enums/Direction.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <ranges>

TextureManager::TextureManager(const UPoint windowSize, std::shared_ptr<SDL_Texture> texture,
                               std::shared_ptr<SDL_Renderer> renderer, std::shared_ptr<TTF_Font> fpsFont,
                               std::shared_ptr<EventSystem> events, std::shared_ptr<AnimationManager> animationManager)
	: _windowSize{windowSize},
	  _renderer{std::move(renderer)},
	  _texture{std::move(texture)},
	  _events{std::move(events)},
	  _animationManager{std::move(animationManager)},
	  _fpsFont{std::move(fpsFont)},
	  _fpsRectangle{.x = static_cast<int>(windowSize.x) - 80, .y = 20, .w = 40, .h = 40}
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
	//TODO: RAII for subscribe, maybe unique ptr or any wrapper for auto unsubscribe when obj die.
	_events->AddListener(
			"DrawObj", _name,
			[this](const ObjRectangle rect, const Direction dir, const std::string& name, const int color)
			{
				this->Draw(rect, dir, name, color);
			});
	_events->AddListener(
			"DrawAnimation", _name,
			[this](const ObjRectangle rect, const Direction dir, const int step, const int scale,
			       const std::string& name, const int color)
			{
				this->DrawAnimation(rect, dir, step, scale, name, color);
			});
	_events->AddListener("DrawHealthBarObj", _name, [this](const ObjRectangle rect, const int health, const int color)
	{
		this->DrawHealthBar(rect, health, color);
	});
}

void TextureManager::Unsubscribe() const
{
	_events->RemoveListener("DrawObj", _name);
	_events->RemoveListener("DrawAnimation", _name);
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

void TextureManager::DrawHealthBar(const ObjRectangle rect, const int health, const int color) const
{
	//TODO: fix recenter health bar when pickup star bonus

	const int healthWidth = health / 3;

	if (healthWidth <= 0)
		return;

	const SDL_Rect healthBarRect = {static_cast<int>(rect.x) + 2, static_cast<int>(rect.y) - 10, healthWidth, 5};

	SetRenderDrawColor(color, 127);

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

void TextureManager::RectDraw(const ObjRectangle rect, const int color)
{
	const SDL_Rect destRect = RectToSdlRect(rect);

	SDL_Texture* colorTexture = CreateColorTexture(color);

	SDL_RenderCopy(_renderer.get(), colorTexture, nullptr, &destRect);
}

SDL_Rect TextureManager::GetTextureRect(const std::string& name) const
{
	SDL_Rect textureRect{};
	if (name == "Bullet")//TODO: replace with enum TextureType
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

	return textureRect;
}

SDL_Rect TextureManager::GetAnimTextureRect(const std::string& name, const ObjRectangle rect, SDL_Rect& destRect) const
{
	SDL_Rect textureRect{};
	if (name == "Enemy1" || name == "Enemy2" || name == "Enemy3" || name == "Enemy4")//TODO: replace with enum
	{
		textureRect = RectToSdlRect(_offset.enemy);
	}
	else if (name == "Player1" || name == "CoopBot1")
	{
		textureRect = RectToSdlRect(_offset.playerOne);
	}
	else if (name == "Player2" || name == "CoopBot2")
	{
		textureRect = RectToSdlRect(_offset.playerTwo);
	}
	else if (name == "Water")
	{
		textureRect = RectToSdlRect(_offset.water);
	}
	// else if (name == "Bullet")
	// {
	// 	textureRect = RectToSdlRect(_offset.bullet);
	// }
	else if (name == "BulletExplosion")
	{
		destRect = RectToSdlRect(rect.GetScale(4.f).GetCenter());
		textureRect = RectToSdlRect(_offset.smallExplosion);
	}
	else if (name == "TankExplosion")
	{
		textureRect = RectToSdlRect(_offset.bigExplosion);
	}
	else if (name == "SpawnAnimation")
	{
		textureRect = RectToSdlRect(_offset.spawnAnim);
	}

	return textureRect;
}

std::pair<double, SDL_RendererFlip> TextureManager::GetRotateAndAngleAndFlip(const Direction dir)
{
	switch (dir)
	{
		case Direction::UP:
			return std::make_pair(0.0, SDL_FLIP_NONE);
		case Direction::LEFT:
			return std::make_pair(-90.0, SDL_FLIP_NONE);
		case Direction::DOWN:
			return std::make_pair(0.0, SDL_FLIP_VERTICAL);
		case Direction::RIGHT:
			return std::make_pair(90.0, SDL_FLIP_NONE);
		default:
			return std::make_pair(0.0, SDL_FLIP_NONE);
	}
}

void TextureManager::Draw(const ObjRectangle rect, const Direction dir, const std::string& name, const int color)
{
	const SDL_Rect destRect = RectToSdlRect(rect);
	const SDL_Rect textureRect = GetTextureRect(name);
	if (constexpr SDL_Rect defaultSdlRect{};
		textureRect.x == defaultSdlRect.x
		&& textureRect.y == defaultSdlRect.y
		&& textureRect.w == defaultSdlRect.w
		&& textureRect.h == defaultSdlRect.h)
	{
		RectDraw(rect, color);//NOTE: fallback draw to non-texture, rectangle filled by color
	}

	DrawTexture(&textureRect, &destRect, dir);
}

void TextureManager::DrawAnimation(const ObjRectangle rect, const Direction dir, const int step, const int scale,
                                   const std::string& name, const int color)
{
	SDL_Rect destRect = RectToSdlRect(rect);
	SDL_Rect textureRect = GetAnimTextureRect(name, rect, destRect);
	textureRect.x += step * scale;
	if (constexpr SDL_Rect defaultSdlRect{};
		textureRect.x == defaultSdlRect.x && textureRect.y == defaultSdlRect.y
		&& textureRect.w == defaultSdlRect.w && textureRect.h == defaultSdlRect.h)
	{
		RectDraw(rect, color);//NOTE: fallback draw to non-texture, rectangle filled by color
	}

	DrawTexture(&textureRect, &destRect, dir);
}

void TextureManager::DrawTexture(const SDL_Rect* textureRect, const SDL_Rect* destRect, const Direction dir) const
{
	//local angle and flip for texture
	auto [angle, flip] = GetRotateAndAngleAndFlip(dir);

	//TODO: move work with sdl to utils to reduce dependencies
	SDL_RenderCopyEx(_renderer.get(), _texture.get(), textureRect, destRect, angle, nullptr, flip);
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
