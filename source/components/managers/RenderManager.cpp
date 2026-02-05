#include "components/managers/RenderManager.h"
#include "Point.h"
#include "components/EventSystem.h"
#include "enums/Direction.h"
#include "enums/TextureOffset.h"
#include <SDL_rect.h>
#include <SDL_render.h>
#include <ranges>

RenderManager::RenderManager(const std::shared_ptr<EventSystem>& events, const std::shared_ptr<SDL_Renderer>& renderer,
                             const std::shared_ptr<TTF_Font>& menuFont, const std::shared_ptr<SDL_Texture>& menuLogo,
                             const std::shared_ptr<SDL_Texture>& atlasTexture, UPoint windowSize)
	: _name{"RenderManager"},
	  _events{events},
	  _renderer{renderer},
	  _font{menuFont},
	  _menuLogo{menuLogo},
	  _atlasTexture{atlasTexture},
	  _fpsRectangle{.x = static_cast<int>(windowSize.x) - 80, .y = 20, .w = 40, .h = 40}
//TODO: dynamic adjust and resize
{
	GenerateFpsTextures();

	Subscribe();

	_padding = 25;
	const auto windowWidth = static_cast<unsigned int>(windowSize.x);
	_windowHeight = static_cast<int>(windowSize.y);
	_height = _windowHeight - _padding * 3;
	constexpr int sideBarWidth = 228;
	_width = windowWidth - sideBarWidth - _padding;

	PregenerateMenuBackgroundPixels();
	PregenerateMenuBackgroundTexture();
}

RenderManager::~RenderManager()
{
	Unsubscribe();
	ClearFpsTextureCache();
	ClearColorTextureCache();
}

void RenderManager::ClearColorTextureCache()
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

void RenderManager::ClearFpsTextureCache()
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

void RenderManager::Subscribe()
{
	_events->AddListener("PreTickUpdate", _name, [this](const float /*deltaTime*/) { this->ClearFrame(); });
	_events->AddListener("RenderText", _name, [this](const Point pos, const unsigned int color, const std::string& text)
	{
		TextToRender(pos, IntToColor(color), text);
	});

	_events->AddListener("RenderMenuBackground", _name, [this](const Point pos)
	{
		DrawBackground(pos);
	});

	_events->AddListener("RenderMenuLogo", _name, [this](const Point pos)
	{
		DrawMenuLogo(pos);
	});

	_events->AddListener("RenderPauseText", _name, [this]()
	{
		constexpr TextureOffset offset{};
		constexpr SDL_Rect rect{.x = 135, .y = 142, .w = 300, .h = 75};

		SDL_Rect srcRect{static_cast<int>(offset.pauseText.x),
		                 static_cast<int>(offset.pauseText.y),
		                 static_cast<int>(offset.pauseText.w),
		                 static_cast<int>(offset.pauseText.h)};
		SDL_RenderCopy(_renderer.get(), _atlasTexture.get(), &srcRect, &rect);
	});

	_events->AddListener("RenderGameOverText", _name, [this]()
	{
		constexpr TextureOffset offset{};
		constexpr SDL_Rect rect{.x = 200, .y = 242, .w = 200, .h = 75};

		SDL_Rect srcrect{static_cast<int>(offset.gameOverText.x),
		                 static_cast<int>(offset.gameOverText.y),
		                 static_cast<int>(offset.gameOverText.w),
		                 static_cast<int>(offset.gameOverText.h)};
		SDL_RenderCopy(_renderer.get(), _atlasTexture.get(), &srcrect, &rect);
	});

	_events->AddListener("RenderGameWonText", _name, [this]()
	{
		DrawGameWonText();
	});

	_events->AddListener("RenderColorTexture", _name, [this](const ObjRectangle rect, const unsigned int color)
	{
		const SDL_Rect destRect = RectToSdlRect(rect);

		SDL_Texture* colorTexture = CreateColorTexture(color);

		SDL_RenderCopy(_renderer.get(), colorTexture, nullptr, &destRect);
	});

	_events->AddListener("RenderTexture", _name,
	                     [this](const ObjRectangle& textureRect, const ObjRectangle& destRect, const Direction dir)
	                     {
		                     DrawTexture(textureRect, destRect, dir);
	                     });

	_events->AddListener("RenderFPS", _name, [this](const size_t fps)
	{
		RenderFPS(fps);
	});

	_events->AddListener(
			"RenderHealthBar", _name,
			[this](const ObjRectangle rect, const int health, const unsigned int color)
			{
				DrawHealthBar(rect, health, color);
			});
}

void RenderManager::Unsubscribe() const
{
	_events->RemoveListener("PreTickUpdate", _name);
	_events->RemoveListener("RenderText", _name);
	_events->RemoveListener("RenderMenuBackground", _name);
	_events->RemoveListener("RenderMenuLogo", _name);
	_events->RemoveListener("RenderPauseText", _name);
	_events->RemoveListener("RenderGameOverText", _name);
	_events->RemoveListener("RenderColorTexture", _name);
	_events->RemoveListener("RenderTexture", _name);
	_events->RemoveListener("RenderFPS", _name);
	_events->RemoveListener("RenderHealthBar", _name);
	_events->RemoveListener("RenderGameWonText", _name);
}

void RenderManager::DrawPauseText() const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect rect{.x = 135, .y = 142, .w = 300, .h = 75};

	SDL_Rect srcrect{static_cast<int>(offset.pauseText.x),
	                 static_cast<int>(offset.pauseText.y),
	                 static_cast<int>(offset.pauseText.w),
	                 static_cast<int>(offset.pauseText.h)};
	SDL_RenderCopy(_renderer.get(), _atlasTexture.get(), &srcrect, &rect);
}

void RenderManager::DrawGameOverText() const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect rect{.x = 200, .y = 242, .w = 200, .h = 75};

	SDL_Rect srcRect{static_cast<int>(offset.gameOverText.x),
	                 static_cast<int>(offset.gameOverText.y),
	                 static_cast<int>(offset.gameOverText.w),
	                 static_cast<int>(offset.gameOverText.h)};
	SDL_RenderCopy(_renderer.get(), _atlasTexture.get(), &srcRect, &rect);
}

void RenderManager::DrawGameWonText() const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect rect{.x = 200, .y = 242, .w = 200, .h = 75};
	SDL_Rect srcrect{static_cast<int>(offset.gameWonText.x),
					 static_cast<int>(offset.gameOverText.y),
					 static_cast<int>(offset.gameOverText.w),
					 static_cast<int>(offset.gameOverText.h)};
	SDL_RenderCopy(_renderer.get(), _atlasTexture.get(), &srcrect, &rect);
}

void RenderManager::PregenerateMenuBackgroundPixels()
{
	_menuBackground = std::make_shared<unsigned int[]>(_height * _width);
	for (int y = 0; y < _height; ++y)
	{
		for (int x = 0; x < _width; ++x)
		{
			constexpr unsigned int menuColor = 0x91808080;// Alpha channel set to 0x80 for semi-transparency
			_menuBackground[y * _width + x] = menuColor;
		}
	}
}

unsigned int RenderManager::ColorToInt(const SDL_Color& color)
{
	return (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
}

SDL_Color RenderManager::IntToColor(const unsigned int color)
{
	return {
			static_cast<Uint8>((color >> 16) & 0xFF),
			static_cast<Uint8>((color >> 8) & 0xFF),
			static_cast<Uint8>(color & 0xFF),
			static_cast<Uint8>((color >> 24) & 0xFF)
	};
}

unsigned int RenderManager::ComponentsToColor(const Uint8 r, const Uint8 g, const Uint8 b, const Uint8 a)
{
	return (a << 24) | (r << 16) | (g << 8) | b;
}

// blend menu panel and menu texture background
void RenderManager::DrawBackground(Point pos) const
{
	const SDL_Rect rect{pos.x, pos.y, _width, _height};

	SDL_UpdateTexture(_backgroundTexture.get(), &rect, _menuBackground.get(), _width << 2);
	SDL_RenderCopy(_renderer.get(), _backgroundTexture.get(), nullptr, &rect);
}

void RenderManager::DrawMenuLogo(Point pos) const
{
	const SDL_Rect rect{.x = pos.x + 135, .y = pos.y + 42, .w = 300, .h = 75};

	SDL_RenderCopy(_renderer.get(), _menuLogo.get(), nullptr, &rect);//TODO: extract render to renderManager
}

void RenderManager::TextToRender(const Point& pos, const SDL_Color& color, const int value) const
{
	TextToRender(pos, color, std::to_string(value));
}

void RenderManager::TextToRender(const Point pos, const SDL_Color color, const std::string& text) const
{
	if (!_font || !_renderer)
	{
		return;
	}

	const std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)> surface(
			TTF_RenderText_Solid(_font.get(), text.c_str(), color),
			SDL_FreeSurface);
	if (!surface)
	{
		return;
	}

	const std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> texture(
			SDL_CreateTextureFromSurface(_renderer.get(), surface.get()),
			SDL_DestroyTexture);
	if (!texture)
	{
		return;
	}

	const SDL_Rect textRect{pos.x, pos.y, surface->w, surface->h};
	SDL_RenderCopy(_renderer.get(), texture.get(), nullptr, &textRect);
}

void RenderManager::PregenerateMenuBackgroundTexture()
{
	// SDL_SetRenderDrawBlendMode(_renderer.get(), SDL_BLENDMODE_BLEND);
	_backgroundTexture = std::shared_ptr<SDL_Texture>(
			SDL_CreateTexture(_renderer.get(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
			                  _width, _height),
			SDL_DestroyTexture);
	SDL_SetTextureBlendMode(_backgroundTexture.get(), SDL_BLENDMODE_BLEND);

	const SDL_Rect rect{_padding, _padding, _width, _height};
	SDL_UpdateTexture(_backgroundTexture.get(), &rect, _menuBackground.get(), _width << 2);
}

inline SDL_Rect RenderManager::RectToSdlRect(const ObjRectangle& rect)
{
	return SDL_Rect{
			static_cast<int>(rect.x),
			static_cast<int>(rect.y),
			static_cast<int>(rect.w),
			static_cast<int>(rect.h)};
}

void RenderManager::SetRenderDrawColor(const unsigned int color, const Uint8 transparency = 255) const
{
	const Uint8 r = (color >> 16) & 0xFF;
	const Uint8 g = (color >> 8) & 0xFF;
	const Uint8 b = color & 0xFF;
	const Uint8 a = transparency;

	SDL_SetRenderDrawColor(_renderer.get(), r, g, b, a);
}

SDL_Texture* RenderManager::CreateColorTexture(const unsigned int color)
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

void RenderManager::ClearFrame() const
{
	SDL_SetRenderDrawColor(_renderer.get(), 0, 0, 0, 255);
	SDL_RenderClear(_renderer.get());
}

std::pair<double, SDL_RendererFlip> RenderManager::GetRotateAndAngleAndFlip(const Direction dir)
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

void RenderManager::DrawTexture(const ObjRectangle& textureRect, const ObjRectangle& destRect, const Direction dir)
{
	//local angle and flip for texture
	auto [angle, flip] = GetRotateAndAngleAndFlip(dir);

	//TODO: move work with sdl to utils to reduce dependencies

	const SDL_Rect sdlTextureRect = RectToSdlRect(textureRect);
	const SDL_Rect sldDestRect = RectToSdlRect(destRect);
	SDL_RenderCopyEx(_renderer.get(), _atlasTexture.get(), &sdlTextureRect, &sldDestRect, angle, nullptr, flip);
}

void RenderManager::GenerateFpsTextures()
{
	_fpsTextures.clear();

	for (size_t i = 0u; i <= 1000u; ++i)
	{
		std::string text = std::to_string(i);
		constexpr SDL_Color textColor = {140, 0, 255, 255};

		SDL_Surface* surface = TTF_RenderText_Solid(_font.get(), text.c_str(), textColor);
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


void RenderManager::RenderFPS(const size_t fps)//TODO: move to framePerSecondManager
{
	if (fps)
	{
		// Copy the texture with FPS to the renderer
		SDL_RenderCopy(_renderer.get(), _fpsTextures[fps], nullptr, &_fpsRectangle);
	}

	SDL_RenderPresent(_renderer.get());
}

void RenderManager::DrawHealthBar(const ObjRectangle rect, const int health, const unsigned int color) const
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
