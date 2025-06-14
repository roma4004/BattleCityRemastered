#include "../../headers/components/TextureManager.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/pawns/Pawn.h"
#include "../../headers/utils/PixelUtils.h" //TODO: pregenerate health bar texture
#include <SDL.h>

TextureManager::TextureManager(std::shared_ptr<SDL_Texture> texture, std::shared_ptr<SDL_Renderer> renderer,
                               std::shared_ptr<EventSystem> events, std::shared_ptr<Window> window)
	: _renderer(std::move(renderer)),
	  _texture(std::move(texture)),
	  _events(std::move(events)),
	  _window{std::move(window)}
{
	Subscribe();
}

TextureManager::~TextureManager()
{
	Unsubscribe();
}

void TextureManager::Subscribe() const
{
	_events->AddListener<const BaseObj*>("DrawObj", _name, [this](const BaseObj* baseObj) { this->Draw(baseObj); });
	_events->AddListener<const BaseObj*>("DrawHealthBarObj", _name, [this](const BaseObj* baseObj) { this->DrawHealthBar(baseObj); });
}

void TextureManager::Unsubscribe() const
{
	_events->RemoveListener<const BaseObj*>("DrawObj", _name);
	_events->RemoveListener<const BaseObj*>("DrawHealthBarObj", _name);
}

inline void TextureManager::SetPixel(const size_t x, const size_t y, const int color) const
{
	if (x < _window->size.x && y < _window->size.y)
	{
		const size_t rowSize = _window->size.x;
		_window->buffer.get()[y * rowSize + x] = color;
	}
}

void TextureManager::DrawHealthBar(const BaseObj* obj) const
{
	//TODO: fix recenter health bar when pickup star bonus

	const auto width = static_cast<unsigned int>(_window->size.x);
	const ObjRectangle rect = obj->GetRect();
	size_t y = static_cast<size_t>(rect.y) - 10;
	for (const size_t maxY = y + 5; y < maxY; ++y)
	{
		size_t x = static_cast<size_t>(rect.x) + 2;
		for (const size_t maxX = x + obj->GetHealth() / 3; x < maxX; ++x)
		{
			const unsigned int tankColor = obj->GetColor();
			if (x < _window->size.x && y < _window->size.y)
			{
				int& targetColor = _window->buffer.get()[y * width + x];
				targetColor = static_cast<int>(
					PixelUtils::BlendPixel(targetColor, PixelUtils::ChangeAlpha(tankColor, 127)));
				SetPixel(x, y, targetColor);
			}
		}
	}
}

SDL_Rect TextureManager::RectToSdlRect(const ObjRectangle& rect)
{
	return SDL_Rect{
			static_cast<int>(rect.x),
			static_cast<int>(rect.y),
			static_cast<int>(rect.w),
			static_cast<int>(rect.h)};
}

void TextureManager::RectDraw(const BaseObj* obj) const
{
	// if (!obj->GetIsAlive())
	// {
	// 	return;
	// }

	const ObjRectangle rect = obj->GetRect();
	int startY = static_cast<int>(rect.y);
	const int startX = static_cast<int>(rect.x);
	const size_t windowWidth = _window->size.x;
	const int height = static_cast<int>(rect.h);
	const int width = static_cast<int>(rect.w);
	const int color = obj->GetColor();
	const auto buffer = _window->buffer.get();

	const int maxY = startY + height;
	const int maxX = startX + width;
	for (; startY < maxY; ++startY)
	{
		for (int x = startX; x < maxX; ++x)
		{
			const size_t offset = startY * windowWidth + startX;
			const int rowWidth = maxX - startX;
			std::ranges::fill_n(buffer + offset, rowWidth, color);
		}
	}
}

void TextureManager::Draw(const BaseObj* obj) const
{
	const ObjRectangle rect = obj->GetRect();
	const SDL_Rect destRect = RectToSdlRect(rect);

	SDL_Rect textureRect{};
	const auto pawn = dynamic_cast<const Pawn*>(obj);
	if (const auto& name = obj->GetName(); //TODO: replace with enum
		name == "Enemy")
	{
		textureRect = RectToSdlRect(_offset.enemy);
		textureRect.x += pawn->_animationId * 13;
	}
	else if (name == "Player1" || name == "CoopBot1")
	{
		textureRect = RectToSdlRect(_offset.playerOne);
		textureRect.x += pawn->_animationId * 13;
	}
	else if (name == "Player2" || name == "CoopBot2")
	{
		textureRect = RectToSdlRect(_offset.playerTwo);
		textureRect.x += pawn->_animationId * 13;
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
	// else if (name == "SteelWall") {}
	// else if (name == "WaterTile") {}
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
