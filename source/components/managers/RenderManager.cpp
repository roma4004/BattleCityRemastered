#include "components/managers/RenderManager.h"
#include "Point.h"
#include "components/EventSystem.h"
#include "enums/Direction.h"
#include "enums/TextureOffset.h"
#include <SDL_rect.h>
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <ranges>

RenderManager::RenderManager(const std::shared_ptr<EventSystem>& events, const std::shared_ptr<SDL_Renderer>& renderer,
							 const std::shared_ptr<TTF_Font>& fontSmall, const std::shared_ptr<TTF_Font>& fontMedium,
							 const std::shared_ptr<SDL_Texture>& menuLogo, const std::shared_ptr<SDL_Texture>& atlas,
							 const std::shared_ptr<SDL_Texture>& joyIcon, const std::shared_ptr<SDL_Texture>& xBoxHint,
							 const std::shared_ptr<SDL_Texture>& pS5Hint, const UPoint windowSize)
	: _name{"RenderManager"}
	, _events{events}
	, _renderer{renderer}
	, _fontSmall{fontSmall}
	, _fontMedium{fontMedium}
	, _menuLogo{menuLogo}
	, _atlas{atlas}
	, _joyIcon{joyIcon}
	, _xBoxHint{xBoxHint}
	, _pS5Hint{pS5Hint}
	, _fpsRectangle{.x = static_cast<int>(windowSize.x) - 80, .y = 20, .w = 40, .h = 40}
//TODO: dynamic adjust and resize
{
	GenerateFpsTextures();

	Subscribe();

	//TODO: move menu init to separated method and separated menuParams structure
	_menuPadding = 25;
	const auto windowWidth = static_cast<unsigned int>(windowSize.x);
	_windowHeight = static_cast<int>(windowSize.y);
	_menuHeight = _windowHeight - _menuPadding * 3;
	constexpr int sideBarWidth = 228;
	_menuWidth = static_cast<int>(windowWidth) - sideBarWidth - _menuPadding;

	PregenerateMenuBackgroundPixels();
	PregenerateMenuBackgroundTexture();

	constexpr unsigned int grayColor = 0x808080;
	_colorTexture = {CreateColorTexture(grayColor), SDL_DestroyTexture};
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
	_events->AddListener("PreTickUpdate", _name, [this](const double /*deltaTime*/) { this->ClearFrame(); });
	_events->AddListener("RenderText", _name, [this](const Point pos, const unsigned int color, const std::string& text)
	{
		TextToRender(pos, IntToColor(color), text);
	});

	_events->AddListener("RenderMenuBackground", _name, [this](const Point pos) { DrawMenuBackground(pos); });
	_events->AddListener("RenderMenuLogo", _name, [this](const Point pos) { DrawMenuLogo(pos); });
	_events->AddListener("RenderMenuJoyIcon", _name, [this](const Point pos) { DrawJoyIcon(pos); });
	_events->AddListener("RenderMenuXBoxHint", _name, [this](const Point pos) { DrawXBoxHint(pos); });
	_events->AddListener("RenderMenuPS5Hint", _name, [this](const Point pos) { DrawPS5Hint(pos); });

	_events->AddListener("RenderPauseText", _name, [this]() { DrawPauseText(); });
	_events->AddListener("RenderGameOverText", _name, [this]() { DrawGameOverText(); });
	_events->AddListener("RenderGameWonText", _name, [this]() { DrawGameWonText(); });

	_events->AddListener("RenderColorTexture", _name, [this](const ObjRectangle rect)
	{
		this->DrawColorTexture(rect);
	});

	_events->AddListener(
			"RenderTexture", _name,
			[this](const ObjRectangle& textureRect, const ObjRectangle& destRect, const Direction dir)
			{
				this->DrawTexture(textureRect, destRect, dir);
			});

	_events->AddListener("RenderFPS", _name, [this](const unsigned int fps) { RenderFPS(fps); });

	_events->AddListener("RenderHealthBar", _name, [this](const ObjRectangle rect, const int health)
	{
		this->DrawHealthBar(rect, health);
	});
	_events->AddListener("RenderRightSideBar", _name, [this]() { this->DrawRightSideBar(); });
	_events->AddListener("RenderEnemyIconBackground", _name, [this]() { this->DrawEnemyIconBackground(); });
	_events->AddListener("RenderEnemyIcons", _name, [this](const unsigned short respawnCount)
	{
		this->DrawEnemyIcons(respawnCount);
	});
	_events->AddListener("RenderPlayerOneIcon", _name, [this](const unsigned short respawnCount)
	{
		this->DrawPlayerOneIcons(respawnCount);
	});
	_events->AddListener("RenderPlayerTwoIcon", _name, [this](const unsigned short respawnCount)
	{
		this->DrawPlayerTwoIcons(respawnCount);
	});
	_events->AddListener("RenderStageNumber", _name, [this](const unsigned short stageNumber)
	{
		this->DrawStageNumber(stageNumber);
	});
}

void RenderManager::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void RenderManager::DrawPauseText() const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect dstRect{.x = 135, .y = 142, .w = 300, .h = 75};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.pauseText.x),
							   .y = static_cast<int>(offset.pauseText.y),
							   .w = static_cast<int>(offset.pauseText.w),
							   .h = static_cast<int>(offset.pauseText.h)};
	SDL_RenderCopy(_renderer.get(), _atlas.get(), &srcRect, &dstRect);
}

void RenderManager::DrawGameOverText() const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect dstRect{.x = 200, .y = 152, .w = 200, .h = 75};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.gameOverText.x),
							   .y = static_cast<int>(offset.gameOverText.y),
							   .w = static_cast<int>(offset.gameOverText.w),
							   .h = static_cast<int>(offset.gameOverText.h)};
	SDL_RenderCopy(_renderer.get(), _atlas.get(), &srcRect, &dstRect);
}

void RenderManager::DrawGameWonText() const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect dstRect{.x = 250, .y = 152, .w = 120, .h = 85};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.gameWonText.x),
							   .y = static_cast<int>(offset.gameWonText.y),
							   .w = static_cast<int>(offset.gameWonText.w),
							   .h = static_cast<int>(offset.gameWonText.h)};
	SDL_RenderCopy(_renderer.get(), _atlas.get(), &srcRect, &dstRect);
}

void RenderManager::DrawRightSideBar() const { DrawColorTexture(TextureOffset{}.rightSideBar); }

void RenderManager::DrawEnemyIconBackground() const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect dstRect{.x = 680, .y = 60, .w = 71, .h = 277};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.enemyIconBackground.x),
							   .y = static_cast<int>(offset.enemyIconBackground.y),
							   .w = static_cast<int>(offset.enemyIconBackground.w),
							   .h = static_cast<int>(offset.enemyIconBackground.h)};
	SDL_RenderCopy(_renderer.get(), _atlas.get(), &srcRect, &dstRect);
}

void RenderManager::DrawEnemyIcons(const int numberOfIcons) const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.enemyIcon.x),
							   .y = static_cast<int>(offset.enemyIcon.y),
							   .w = static_cast<int>(offset.enemyIcon.w),
							   .h = static_cast<int>(offset.enemyIcon.h)};

	for (int i = 0; i < numberOfIcons; ++i)
	{
		constexpr int columns{2};
		constexpr int leftUpCornerX{685};
		constexpr int leftUpCornerY{65};
		constexpr int imageWidth{30};
		constexpr int imageHeight{25};
		constexpr int spacingX = 1;
		constexpr int spacingY = 2;

		const int row = i / columns;
		const int col = i % columns;
		const int xAxis = leftUpCornerX + col * (imageWidth + spacingX);
		const int yAxis = leftUpCornerY + row * (imageHeight + spacingY);

		SDL_Rect destRect = {.x = xAxis, .y = yAxis, .w = imageWidth, .h = imageHeight};
		SDL_RenderCopy(_renderer.get(), _atlas.get(), &srcRect, &destRect);
	}
}

void RenderManager::DrawPlayerOneIcons(const unsigned short respawnCount) const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.playerOneIcon.x),
							   .y = static_cast<int>(offset.playerOneIcon.y),
							   .w = static_cast<int>(offset.playerOneIcon.w),
							   .h = static_cast<int>(offset.playerOneIcon.h)};
	constexpr SDL_Rect rect{.x = 680, .y = 350, .w = 71, .h = 70};
	SDL_RenderCopy(_renderer.get(), _atlas.get(), &srcRect, &rect);

	constexpr bool isMediumFontSize = true;
	TextToRender(Point{.x = 718, .y = 390}, IntToColor(2), respawnCount, isMediumFontSize);
}

void RenderManager::DrawPlayerTwoIcons(const unsigned short respawnCount) const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.playerTwoIcon.x),
							   .y = static_cast<int>(offset.playerTwoIcon.y),
							   .w = static_cast<int>(offset.playerTwoIcon.w),
							   .h = static_cast<int>(offset.playerTwoIcon.h)};
	constexpr SDL_Rect rect{.x = 680, .y = 420, .w = 71, .h = 70};
	SDL_RenderCopy(_renderer.get(), _atlas.get(), &srcRect, &rect);

	constexpr bool isMediumFontSize = true;
	TextToRender(Point{.x = 718, .y = 460}, IntToColor(2), respawnCount, isMediumFontSize);
}

void RenderManager::DrawStageNumber(const unsigned short currentStageNumber) const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.stageNumberFlag.x),
							   .y = static_cast<int>(offset.stageNumberFlag.y),
							   .w = static_cast<int>(offset.stageNumberFlag.w),
							   .h = static_cast<int>(offset.stageNumberFlag.h)};
	constexpr SDL_Rect rect{.x = 680, .y = 490, .w = 71, .h = 95};
	SDL_RenderCopy(_renderer.get(), _atlas.get(), &srcRect, &rect);

	constexpr bool isMediumFontSize = true;
	TextToRender(Point{.x = 718, .y = 555}, IntToColor(2), currentStageNumber, isMediumFontSize);
}

void RenderManager::PregenerateMenuBackgroundPixels()
{
	_menuBackground = std::make_shared<unsigned int[]>(
			static_cast<size_t>(_menuHeight) * static_cast<size_t>(_menuWidth));
	for (int y = 0; y < _menuHeight; ++y)
	{
		for (int x = 0; x < _menuWidth; ++x)
		{
			constexpr unsigned int menuColor = 0x91808080;// Alpha channel set to 0x80 for semi-transparency gray
			_menuBackground[y * _menuWidth + x] = menuColor;
		}
	}
}

unsigned int RenderManager::ColorToInt(const SDL_Color& color)
{
	return (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
}

SDL_Color RenderManager::IntToColor(const unsigned int color)
{
	return SDL_Color{.r = static_cast<Uint8>((color >> 16) & 0xFF),
					 .g = static_cast<Uint8>((color >> 8) & 0xFF),
					 .b = static_cast<Uint8>(color & 0xFF),
					 .a = static_cast<Uint8>((color >> 24) & 0xFF)};
}

unsigned int RenderManager::ComponentsToColor(const Uint8 r, const Uint8 g, const Uint8 b, const Uint8 a)
{
	return (a << 24) | (r << 16) | (g << 8) | b;
}

// blend menu panel and menu texture background
void RenderManager::DrawMenuBackground(const Point pos) const
{
	const SDL_Rect rect{.x = pos.x, .y = pos.y, .w = _menuWidth, .h = _menuHeight};
	SDL_UpdateTexture(_menuBackgroundTexture.get(), &rect, _menuBackground.get(), _menuWidth << 2);
	SDL_RenderCopy(_renderer.get(), _menuBackgroundTexture.get(), nullptr, &rect);
}

void RenderManager::DrawMenuLogo(const Point pos) const
{
	const SDL_Rect rect{.x = pos.x + 135, .y = pos.y + 42, .w = 300, .h = 75};
	SDL_RenderCopy(_renderer.get(), _menuLogo.get(), nullptr, &rect);
}

void RenderManager::DrawJoyIcon(const Point pos) const
{
	const SDL_Rect rect{.x = pos.x, .y = pos.y, .w = 30, .h = 30};
	SDL_RenderCopy(_renderer.get(), _joyIcon.get(), nullptr, &rect);
}

void RenderManager::RenderCopyWithClipping(SDL_Texture* texture, const SDL_Rect srcRect, const SDL_Rect dstRect) const
{
	SDL_RenderCopy(_renderer.get(), texture, &srcRect, &dstRect);
}

void RenderManager::DrawXBoxHint(const Point pos) const
{
	RenderCopyWithClipping(_xBoxHint.get(), {.x = 935, .y = 365, .w = 65, .h = 65},
						   {.x = pos.x - 65, .y = pos.y + 93, .w = 25, .h = 25});//View button

	RenderCopyWithClipping(_xBoxHint.get(), {.x = 1115, .y = 365, .w = 65, .h = 65},
						   {.x = pos.x - 65, .y = pos.y + 123, .w = 25, .h = 25});//Menu button

	RenderCopyWithClipping(_xBoxHint.get(), {.x = 1330, .y = 270, .w = 95, .h = 95},
						   {.x = pos.x - 65, .y = pos.y + 153, .w = 25, .h = 25});//Y button

	RenderCopyWithClipping(_xBoxHint.get(), {.x = 790, .y = 490, .w = 220, .h = 220},
						   {.x = pos.x - 65, .y = pos.y + 183, .w = 25, .h = 25});//Dpad button

	RenderCopyWithClipping(_xBoxHint.get(), {.x = 1330, .y = 435, .w = 95, .h = 95},
						   {.x = pos.x - 65, .y = pos.y + 213, .w = 25, .h = 25});//A button
}

void RenderManager::DrawPS5Hint(const Point pos) const
{
	RenderCopyWithClipping(_pS5Hint.get(), {.x = 330, .y = 123, .w = 40, .h = 70},
						   {.x = pos.x + 10, .y = pos.y - 60, .w = 25, .h = 30});//Create button

	RenderCopyWithClipping(_pS5Hint.get(), {.x = 780, .y = 123, .w = 40, .h = 70},
						   {.x = pos.x + 10, .y = pos.y - 28, .w = 25, .h = 30});//Options button

	RenderCopyWithClipping(_pS5Hint.get(), {.x = 838, .y = 163, .w = 75, .h = 75},
						   {.x = pos.x + 10, .y = pos.y + 5, .w = 25, .h = 25});//Triangle button

	RenderCopyWithClipping(_pS5Hint.get(), {.x = 185, .y = 185, .w = 180, .h = 180},
						   {.x = pos.x + 10, .y = pos.y + 33, .w = 25, .h = 25});//Dpad button

	RenderCopyWithClipping(_pS5Hint.get(), {.x = 838, .y = 305, .w = 75, .h = 75},
						   {.x = pos.x + 10, .y = pos.y + 63, .w = 25, .h = 25});//Cross button
}

void RenderManager::TextToRender(const Point& pos, const SDL_Color& color, const int value,
								 const bool isMediumFontSize = false) const
{
	TextToRender(pos, color, std::to_string(value), isMediumFontSize);
}

void RenderManager::TextToRender(const Point pos, const SDL_Color color, const std::string& text,
								 const bool isMediumFontSize) const
{
	if (!_fontMedium || !_fontSmall || !_renderer)
	{
		return;
	}

	const auto currentFont = isMediumFontSize ? _fontMedium.get() : _fontSmall.get();
	const std::unique_ptr<SDL_Surface, void (*)(SDL_Surface*)> surface(
			TTF_RenderText_Solid(currentFont, text.c_str(), color), SDL_FreeSurface);
	if (!surface)
	{
		return;
	}

	const std::unique_ptr<SDL_Texture, void (*)(SDL_Texture*)> texture(
			SDL_CreateTextureFromSurface(_renderer.get(), surface.get()), SDL_DestroyTexture);
	if (!texture)
	{
		return;
	}

	const SDL_Rect textRect{.x = pos.x, .y = pos.y, .w = surface->w, .h = surface->h};
	SDL_RenderCopy(_renderer.get(), texture.get(), nullptr, &textRect);
}

void RenderManager::PregenerateMenuBackgroundTexture()
{
	// SDL_SetRenderDrawBlendMode(_renderer.get(), SDL_BLENDMODE_BLEND);
	_menuBackgroundTexture = std::shared_ptr<SDL_Texture>(
			SDL_CreateTexture(
					_renderer.get(),
					SDL_PIXELFORMAT_ARGB8888,
					SDL_TEXTUREACCESS_TARGET,
					_menuWidth,
					_menuHeight),
			SDL_DestroyTexture);
	SDL_SetTextureBlendMode(_menuBackgroundTexture.get(), SDL_BLENDMODE_BLEND);

	const SDL_Rect rect{.x = _menuPadding, .y = _menuPadding, .w = _menuWidth, .h = _menuHeight};
	SDL_UpdateTexture(_menuBackgroundTexture.get(), &rect, _menuBackground.get(), _menuWidth << 2);
}

inline SDL_Rect RenderManager::RectToSdlRect(const ObjRectangle& rect)
{
	return SDL_Rect{.x = static_cast<int>(rect.x),
					.y = static_cast<int>(rect.y),
					.w = static_cast<int>(rect.w),
					.h = static_cast<int>(rect.h)};
}

void RenderManager::SetRenderDrawColor(const unsigned int color, const Uint8 transparency) const
{
	const Uint8 r = (color >> 16) & 0xFF;
	const Uint8 g = (color >> 8) & 0xFF;
	const Uint8 b = color & 0xFF;
	const Uint8 a = transparency;

	SDL_SetRenderDrawColor(_renderer.get(), r, g, b, a);
}

SDL_Texture* RenderManager::CreateColorTexture(const unsigned int color)
{
	if (const auto it = _colorTextureCache.find(color); it != _colorTextureCache.end())
	{
		return it->second;
	}

	SDL_Texture* colorTexture = SDL_CreateTexture(
			_renderer.get(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 1, 1);

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

void RenderManager::DrawColorTexture(const ObjRectangle rect) const
{
	const SDL_Rect dstRect = RectToSdlRect(rect);
	SDL_RenderCopy(_renderer.get(), _colorTexture.get(), nullptr, &dstRect);
}

void RenderManager::DrawTexture(const ObjRectangle& texture, const ObjRectangle& dest, const Direction dir) const
{
	//local angle and flip for texture
	auto [angle, flip] = GetRotateAndAngleAndFlip(dir);
	const SDL_Rect srcRect = RectToSdlRect(texture);
	const SDL_Rect dstRect = RectToSdlRect(dest);
	SDL_RenderCopyEx(_renderer.get(), _atlas.get(), &srcRect, &dstRect, angle, nullptr, flip);
}

void RenderManager::GenerateFpsTextures()
{
	_fpsTextures.clear();

	for (size_t i = 0u; i <= 1000u; ++i)
	{
		std::string text = std::to_string(i);
		constexpr SDL_Color textColor = {.r = 140, .g = 0, .b = 255, .a = 255};

		SDL_Surface* surface = TTF_RenderText_Solid(_fontSmall.get(), text.c_str(), textColor);
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


void RenderManager::RenderFPS(const size_t fps)
{
	if (fps)
	{
		// Copy the texture with FPS to the renderer
		SDL_RenderCopy(_renderer.get(), _fpsTextures[fps], nullptr, &_fpsRectangle);
	}

	SDL_RenderPresent(_renderer.get());
}

void RenderManager::DrawHealthBar(const ObjRectangle rect, const int health) const
{
	const int healthWidth = health / 3;
	if (healthWidth <= 0)
		return;

	const int offset = health > 100 ? (health - 100) / 2 : 0;
	const SDL_Rect healthBarRect = {.x = static_cast<int>(rect.x) + 2 - offset / 3,
									.y = static_cast<int>(rect.y) - 10,
									.w = healthWidth,
									.h = 5};

	unsigned int color;
	if (health > 70)
	{
		constexpr unsigned int colorGreen = 0x408000;
		color = colorGreen;
	}
	else if (health > 30)
	{
		constexpr unsigned int colorYellow = 0xEAEA00;
		color = colorYellow;
	}
	else
	{
		constexpr unsigned int colorRed = 0xFF8080;
		color = colorRed;
	}

	SetRenderDrawColor(color, 127);

	SDL_BlendMode blendMode;
	SDL_GetRenderDrawBlendMode(_renderer.get(), &blendMode);//backup blendMode type
	SDL_SetRenderDrawBlendMode(_renderer.get(), SDL_BLENDMODE_BLEND);//set new blendMode type
	SDL_RenderFillRect(_renderer.get(), &healthBarRect);
	SDL_SetRenderDrawBlendMode(_renderer.get(), blendMode);//restore blendMode type
}
