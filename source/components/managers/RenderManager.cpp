#include "components/managers/RenderManager.h"
#include "Point.h"
#include "application/GameConfig.h"
#include "application/SDL_Config.h"
#include "components/EventSystem.h"
#include "enums/Direction.h"
#include "enums/TextureOffset.h"
#include <SDL_render.h>
#include <SDL_ttf.h>

RenderManager::RenderManager(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig, SDL_Config& sdlConfig)
	: _name{"RenderManager"}
	, _events{events}
	, _gameConfig{gameConfig}
	, _sdlConfig{sdlConfig}
	, _fpsRectangle{CalcFpsPos(gameConfig.windowSize)}
{
	GenerateFpsTextures();

	//TODO: SDL_SetWindowTitle(gameConfig.sdlWindow.get(), "current GameMode");
	Subscribe();

	InitMenu(gameConfig);
}

RenderManager::~RenderManager()
{
	Unsubscribe();
	ClearFpsTextureCache();
	ClearColorTextureCache();
}

void RenderManager::ClearColorTextureCache()
{
	for (auto& texture: _colorTextureCache | std::views::values)
	{
		if (texture != nullptr)
		{
			SDL_DestroyTexture(texture.get());
			texture = nullptr;
		}
	}

	_colorTextureCache.clear();
}

void RenderManager::ClearFpsTextureCache()
{
	for (auto& texture: _fpsTextures | std::views::values)
	{
		if (texture != nullptr)
		{
			SDL_DestroyTexture(texture.get());
			texture = nullptr;
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
	_events->AddListener("RenderMenuSelectorIcon", _name, [this](const Point pos) { DrawSelectorIcon(pos); });
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

	_events->AddListener("WindowSizeChangedTo", _name, [this](const UPoint& newSize)
	{
		this->_gameConfig.windowSize = newSize;//TODO: find better place for this responsibility
		this->_fpsRectangle = CalcFpsPos(newSize);

		SDL_RenderSetLogicalSize(this->_sdlConfig.renderer.get(),
								 static_cast<int>(newSize.x),
								 static_cast<int>(newSize.y));
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
	SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), &srcRect, &dstRect);
}

void RenderManager::DrawGameOverText() const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect dstRect{.x = 200, .y = 152, .w = 200, .h = 75};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.gameOverText.x),
							   .y = static_cast<int>(offset.gameOverText.y),
							   .w = static_cast<int>(offset.gameOverText.w),
							   .h = static_cast<int>(offset.gameOverText.h)};
	SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), &srcRect, &dstRect);
}

void RenderManager::DrawGameWonText() const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect dstRect{.x = 250, .y = 152, .w = 120, .h = 85};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.gameWonText.x),
							   .y = static_cast<int>(offset.gameWonText.y),
							   .w = static_cast<int>(offset.gameWonText.w),
							   .h = static_cast<int>(offset.gameWonText.h)};
	SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), &srcRect, &dstRect);
}

void RenderManager::DrawRightSideBar() const
{
	SDL_Rect backgroundRect{RectToSdlRect(TextureOffset{}.rightSideBar)};
	backgroundRect.x = static_cast<int>(_gameConfig.windowSize.x - _gameConfig.sideBarWidth);
	constexpr unsigned int color{0xFF808080u};
	constexpr Uint8 a{(color >> 24u) & 0xFFu};
	constexpr Uint8 r{(color >> 16u) & 0xFFu};
	constexpr Uint8 g{(color >> 8u) & 0xFFu};
	constexpr Uint8 b{(color >> 0u) & 0xFFu};
	SDL_SetRenderDrawColor(_sdlConfig.renderer.get(), r, g, b, a);
	SDL_RenderFillRect(_sdlConfig.renderer.get(), &backgroundRect);
}

void RenderManager::DrawEnemyIconBackground() const
{
	constexpr TextureOffset offset{};
	constexpr int padding{55};
	const int posX{static_cast<int>(_gameConfig.windowSize.x - _gameConfig.sideBarWidth) + padding};
	const SDL_Rect dstRect{.x = posX, .y = 60, .w = 71, .h = 277};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.enemyIconBackground.x),
							   .y = static_cast<int>(offset.enemyIconBackground.y),
							   .w = static_cast<int>(offset.enemyIconBackground.w),
							   .h = static_cast<int>(offset.enemyIconBackground.h)};
	SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), &srcRect, &dstRect);
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
		constexpr int iconBackgroundPadding{55};
		constexpr int iconPadding{iconBackgroundPadding + 5};
		const Point startPos{.x = static_cast<int>(_gameConfig.windowSize.x - _gameConfig.sideBarWidth) + iconPadding,
							 .y = 65};
		constexpr Point imageSize{.x = 30, .y = 25};
		constexpr Point padding{.x = 1, .y = 2};

		const int row{i / columns};
		const int col{i % columns};
		const int posX{startPos.x + col * (imageSize.x + padding.x)};
		const int posY{startPos.y + row * (imageSize.y + padding.y)};

		SDL_Rect destRect = {.x = posX, .y = posY, .w = imageSize.x, .h = imageSize.y};
		SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), &srcRect, &destRect);
	}
}

void RenderManager::DrawPlayerOneIcons(const unsigned short respawnCount) const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.playerOneIcon.x),
							   .y = static_cast<int>(offset.playerOneIcon.y),
							   .w = static_cast<int>(offset.playerOneIcon.w),
							   .h = static_cast<int>(offset.playerOneIcon.h)};

	constexpr int padding{55};
	const int posX{static_cast<int>(_gameConfig.windowSize.x - _gameConfig.sideBarWidth) + padding};
	const SDL_Rect rect{.x = posX, .y = 350, .w = 71, .h = 70};
	SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), &srcRect, &rect);

	constexpr bool isMediumFontSize{true};
	constexpr int textPadding{38};
	TextToRender(Point{.x = posX + textPadding, .y = 390}, IntToColor(2u), respawnCount, isMediumFontSize);
}

void RenderManager::DrawPlayerTwoIcons(const unsigned short respawnCount) const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.playerTwoIcon.x),
							   .y = static_cast<int>(offset.playerTwoIcon.y),
							   .w = static_cast<int>(offset.playerTwoIcon.w),
							   .h = static_cast<int>(offset.playerTwoIcon.h)};

	constexpr int padding{55};
	const int posX{static_cast<int>(_gameConfig.windowSize.x - _gameConfig.sideBarWidth) + padding};
	const SDL_Rect rect{.x = posX, .y = 420, .w = 71, .h = 70};
	SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), &srcRect, &rect);

	constexpr bool isMediumFontSize{true};
	constexpr int textPadding{38};
	TextToRender(Point{.x = posX + textPadding, .y = 460}, IntToColor(2u), respawnCount, isMediumFontSize);
}

void RenderManager::DrawStageNumber(const unsigned short currentStageNumber) const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.stageNumberFlag.x),
							   .y = static_cast<int>(offset.stageNumberFlag.y),
							   .w = static_cast<int>(offset.stageNumberFlag.w),
							   .h = static_cast<int>(offset.stageNumberFlag.h)};

	constexpr int padding{55};
	const int posX{static_cast<int>(_gameConfig.windowSize.x - _gameConfig.sideBarWidth) + padding};
	const SDL_Rect rect{.x = posX, .y = 490, .w = 71, .h = 95};
	SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), &srcRect, &rect);

	constexpr bool isMediumFontSize{true};
	constexpr int textPadding{38};
	TextToRender(Point{.x = posX + textPadding, .y = 555}, IntToColor(2u), currentStageNumber, isMediumFontSize);
}

unsigned int RenderManager::ColorToInt(const SDL_Color& color)
{
	return (color.a << 24u) | (color.r << 16u) | (color.g << 8u) | (color.b << 0u);
}

SDL_Color RenderManager::IntToColor(const unsigned int color)
{
	return SDL_Color{.r = static_cast<Uint8>((color >> 16u) & 0xFFu),
					 .g = static_cast<Uint8>((color >> 8u) & 0xFFu),
					 .b = static_cast<Uint8>((color >> 0u) & 0xFFu),
					 .a = static_cast<Uint8>((color >> 24u) & 0xFFu)};
}

unsigned int RenderManager::ComponentsToColor(const Uint8 r, const Uint8 g, const Uint8 b, const Uint8 a)
{
	return (a << 24u) | (r << 16u) | (g << 8u) | (b << 0u);
}

// blend menu panel and menu texture background
void RenderManager::DrawMenuBackground(const Point pos) const
{
	const SDL_Rect backgroundRect{.x = pos.x + static_cast<int>(_menuParams.padding / 2u),
								  .y = pos.y + static_cast<int>(_menuParams.padding / 2u),
								  .w = static_cast<int>(_menuParams.panelSize.x),
								  .h = static_cast<int>(_menuParams.panelSize.y)};
	constexpr unsigned int color{0x91808080u};
	constexpr Uint8 a{(color >> 24u) & 0xFFu};
	constexpr Uint8 r{(color >> 16u) & 0xFFu};
	constexpr Uint8 g{(color >> 8u) & 0xFFu};
	constexpr Uint8 b{(color >> 0u) & 0xFFu};
	SDL_SetRenderDrawColor(_sdlConfig.renderer.get(), r, g, b, a);
	SDL_RenderFillRect(_sdlConfig.renderer.get(), &backgroundRect);
}

void RenderManager::DrawMenuLogo(const Point pos) const
{
	const SDL_Rect rect{.x = pos.x + 135, .y = pos.y + 42, .w = 300, .h = 75};
	SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.logoTexture.get(), nullptr, &rect);
}

void RenderManager::DrawSelectorIcon(const Point pos) const
{
	const SDL_Rect rect{.x = pos.x, .y = pos.y, .w = 30, .h = 30};
	SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.selectorIconTexture.get(), nullptr, &rect);
}

void RenderManager::RenderCopyWithClipping(SDL_Texture* texture, const SDL_Rect srcRect, const SDL_Rect dstRect) const
{
	SDL_RenderCopy(_sdlConfig.renderer.get(), texture, &srcRect, &dstRect);
}

void RenderManager::RenderCopy(SDL_Texture* texture, const SDL_Rect dstRect) const
{
	SDL_RenderCopy(_sdlConfig.renderer.get(), texture, nullptr, &dstRect);
}

void RenderManager::DrawXBoxHint(const Point pos) const
{
	RenderCopy(_sdlConfig.xboxTextures[3].get(), {.x = pos.x - 75, .y = pos.y + 93, .w = 30, .h = 30});//View button
	RenderCopy(_sdlConfig.xboxTextures[2].get(), {.x = pos.x - 75, .y = pos.y + 123, .w = 30, .h = 30});//Menu button
	RenderCopy(_sdlConfig.xboxTextures[5].get(), {.x = pos.x - 75, .y = pos.y + 153, .w = 30, .h = 30});//Y button
	RenderCopy(_sdlConfig.xboxTextures[0].get(), {.x = pos.x - 75, .y = pos.y + 183, .w = 30, .h = 30});//Dpad button
	RenderCopy(_sdlConfig.xboxTextures[1].get(), {.x = pos.x - 75, .y = pos.y + 63, .w = 30, .h = 30});//Home button
	RenderCopy(_sdlConfig.xboxTextures[4].get(), {.x = pos.x - 75, .y = pos.y + 213, .w = 30, .h = 30});//A button
}

void RenderManager::DrawPS5Hint(const Point pos) const
{
	RenderCopy(_sdlConfig.ps5Textures[0].get(), {.x = pos.x, .y = pos.y - 60, .w = 30, .h = 30});//Create button
	RenderCopy(_sdlConfig.ps5Textures[4].get(), {.x = pos.x, .y = pos.y - 28, .w = 30, .h = 30});//Options button
	RenderCopy(_sdlConfig.ps5Textures[5].get(), {.x = pos.x, .y = pos.y + 5, .w = 30, .h = 30});//Triangle button
	RenderCopy(_sdlConfig.ps5Textures[2].get(), {.x = pos.x, .y = pos.y + 33, .w = 30, .h = 30});//Dpad button
	RenderCopy(_sdlConfig.ps5Textures[3].get(), {.x = pos.x, .y = pos.y - 90, .w = 30, .h = 30});//Home button
	RenderCopy(_sdlConfig.ps5Textures[1].get(), {.x = pos.x, .y = pos.y + 63, .w = 30, .h = 30});//Cross button
}

void RenderManager::TextToRender(const Point& pos, const SDL_Color& color, const int value,
								 const bool isMediumFontSize = false) const
{
	TextToRender(pos, color, std::to_string(value), isMediumFontSize);
}

void RenderManager::TextToRender(const Point pos, const SDL_Color color, const std::string& text,
								 const bool isMediumFontSize) const
{
	if (!_sdlConfig.fontMedium || !_sdlConfig.fontSmall || !_sdlConfig.renderer)
	{
		return;
	}

	//TODO: optimize draw call with cache non changed text part
	// save text surface in render field (lazy init)
	const auto currentFont = isMediumFontSize ? _sdlConfig.fontMedium.get() : _sdlConfig.fontSmall.get();
	const std::unique_ptr<SDL_Surface, void (*)(SDL_Surface*)> surface(
			TTF_RenderText_Solid(currentFont, text.c_str(), color), SDL_FreeSurface);
	if (!surface)
	{
		return;
	}

	//TODO: optimize draw call with cache non changed text part
	// save text texture in render field (lazy init)
	const std::unique_ptr<SDL_Texture, void (*)(SDL_Texture*)> texture(
			SDL_CreateTextureFromSurface(_sdlConfig.renderer.get(), surface.get()), SDL_DestroyTexture);
	if (!texture)
	{
		return;
	}

	const SDL_Rect textRect{.x = pos.x, .y = pos.y, .w = surface->w, .h = surface->h};
	SDL_RenderCopy(_sdlConfig.renderer.get(), texture.get(), nullptr, &textRect);
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
	const Uint8 r = (color >> 16u) & 0xFFu;
	const Uint8 g = (color >> 8u) & 0xFFu;
	const Uint8 b = color & 0xFFu;
	const Uint8 a = transparency;

	SDL_SetRenderDrawColor(_sdlConfig.renderer.get(), r, g, b, a);
}

void RenderManager::CreateColorTexture(const unsigned int color)
{
	auto colorTexture{std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>(
			SDL_CreateTexture(
					_sdlConfig.renderer.get(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 1, 1),
			SDL_DestroyTexture)};

	SDL_SetRenderTarget(_sdlConfig.renderer.get(), colorTexture.get());

	SetRenderDrawColor(color);

	SDL_RenderClear(_sdlConfig.renderer.get());// Fill rect with color

	SDL_SetRenderTarget(_sdlConfig.renderer.get(), nullptr);

	_colorTextureCache.insert_or_assign(color, std::move(colorTexture));
}

void RenderManager::ClearFrame() const
{
	SDL_SetRenderDrawColor(_sdlConfig.renderer.get(), 0u, 0u, 0u, 255u);
	SDL_RenderClear(_sdlConfig.renderer.get());
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
	}

	return std::make_pair(0.0, SDL_FLIP_NONE);
}

void RenderManager::DrawColorTexture(const ObjRectangle rect)
{
	const SDL_Rect dstRect = RectToSdlRect(rect);
	constexpr unsigned int grayColor = 0x808080u;
	if (const auto it = _colorTextureCache.find(grayColor); it != _colorTextureCache.end())
	{
		SDL_RenderCopy(_sdlConfig.renderer.get(), it->second.get(), nullptr, &dstRect);
	}
}

void RenderManager::DrawTexture(const ObjRectangle& texture, const ObjRectangle& dest, const Direction dir) const
{
	//local angle and flip for texture
	auto [angle, flip] = GetRotateAndAngleAndFlip(dir);
	const SDL_Rect srcRect = RectToSdlRect(texture);
	const SDL_Rect dstRect = RectToSdlRect(dest);
	SDL_RenderCopyEx(
			_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), &srcRect, &dstRect, angle, nullptr, flip);
}

void RenderManager::GenerateFpsTextures()
{
	_fpsTextures.clear();

	for (unsigned int i = 0u; i <= 1000u; ++i)
	{
		std::string text = std::to_string(i);
		constexpr SDL_Color textColor = {.r = 140u, .g = 0u, .b = 255u, .a = 255u};

		std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> surface{
				TTF_RenderText_Solid(_sdlConfig.fontMedium.get(), text.c_str(), textColor),
				SDL_FreeSurface};
		if (!surface)
		{
			SDL_Log("Failed to create surface for FPS %u: %s", i, SDL_GetError());
			continue;
		}

		std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture{
				SDL_CreateTextureFromSurface(_sdlConfig.renderer.get(), surface.get()),
				SDL_DestroyTexture};
		if (!texture)
		{
			SDL_Log("Failed to create texture for FPS %u: %s", i, SDL_GetError());
			continue;
		}

		_fpsTextures.emplace(i, std::move(texture));
	}
}


void RenderManager::RenderFPS(const unsigned int fps)
{
	if (fps)
	{
		// Copy the texture with FPS to the renderer
		if (const auto it = _fpsTextures.find(fps); it != _fpsTextures.end())
		{
			SDL_RenderCopy(_sdlConfig.renderer.get(), it->second.get(), nullptr, &_fpsRectangle);
		}
	}

	//TODO:extract to separated subscription
	SDL_RenderPresent(_sdlConfig.renderer.get());
}

void RenderManager::DrawHealthBar(const ObjRectangle rect, const int health) const
{
	const float pixelsPerHealthPoint = static_cast<float>(rect.w) / 100.0f;
	const float healthWidth = static_cast<float>(health) * pixelsPerHealthPoint;
	if (healthWidth <= 0)
		return;

	const int offset = health > 100 ? (health - 100) / 2 : 0;
	const SDL_Rect healthBarRect = {.x = static_cast<int>(rect.x) + 2 - offset / 3,
									.y = static_cast<int>(rect.y) - 10,
									.w = static_cast<int>(healthWidth),
									.h = 5};

	unsigned int color;
	if (health > 70)
	{
		constexpr unsigned int colorGreen = 0x408000u;
		color = colorGreen;
	}
	else if (health > 30)
	{
		constexpr unsigned int colorYellow = 0xEAEA00u;
		color = colorYellow;
	}
	else
	{
		constexpr unsigned int colorRed = 0xFF8080u;
		color = colorRed;
	}

	SetRenderDrawColor(color, 127u);
	SDL_RenderFillRect(_sdlConfig.renderer.get(), &healthBarRect);
}

void RenderManager::InitMenu(const GameConfig& gameConfig)
{
	_menuParams.Init(gameConfig.windowSize, gameConfig.sideBarWidth);

	constexpr unsigned int grayColor = 0x808080u;
	CreateColorTexture(grayColor);
}

SDL_Rect RenderManager::CalcFpsPos(const UPoint& newSize)
{
	constexpr int rightPadding = 105;
	constexpr int topPadding = 15;
	constexpr int fpsSize = 40;
	const int posX = static_cast<int>(newSize.x) - rightPadding;
	return SDL_Rect{.x = posX, .y = topPadding, .w = fpsSize, .h = fpsSize};
}
