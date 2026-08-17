#include "components/managers/RenderManager.h"
#include "Point.h"
#include "application/GameConfig.h"
#include "application/SDL_Config.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/Direction.h"
#include "enums/TextureOffset.h"
#include <SDL_render.h>
#include <SDL_ttf.h>

RenderManager::RenderManager(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig, SDL_Config& sdlConfig)
	: _events{events}
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
	_subs.push_back(_events->AddListener(this, &RenderManager::ClearFrame));
	_subs.push_back(_events->AddListener(this, &RenderManager::OnRenderText));

	_subs.push_back(_events->AddListener(this, &RenderManager::DrawMenuBackground));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawMenuLogo));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawSelectorIcon));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawXBoxHint));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawPS5Hint));

	_subs.push_back(_events->AddListener(this, &RenderManager::DrawPauseText));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawGameOverText));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawGameWonText));

	_subs.push_back(_events->AddListener(this, &RenderManager::DrawColorTexture));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawTexture));

	_subs.push_back(_events->AddListener(this, &RenderManager::RenderFPS));

	_subs.push_back(_events->AddListener(this, &RenderManager::DrawHealthBar));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawRightSideBar));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawEnemyIconBackground));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawEnemyIcons));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawPlayerOneIcons));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawPlayerTwoIcons));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawStageNumber));

	_subs.push_back(_events->AddListener(this, &RenderManager::OnWindowSizeChangedTo));
}

void RenderManager::OnRenderText(const RenderTextEvent& event) const
{
	TextToRender(event.pos, IntToColor(event.color), event.text);
}

void RenderManager::OnWindowSizeChangedTo(const WindowSizeChangedToEvent& event)
{
	//NOTE: windowSize itself is WorldScaleManager's to set - it subscribes first, so by now it has
	_fpsRectangle = CalcFpsPos(event.newSize);

	SDL_RenderSetLogicalSize(_sdlConfig.renderer.get(), static_cast<int>(event.newSize.x),
							 static_cast<int>(event.newSize.y));
}

void RenderManager::DrawPauseText(const RenderPauseTextEvent&) const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect dstRect{.x = 135, .y = 142, .w = 300, .h = 75};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.pauseText.x),
							   .y = static_cast<int>(offset.pauseText.y),
							   .w = static_cast<int>(offset.pauseText.w),
							   .h = static_cast<int>(offset.pauseText.h)};
	SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), &srcRect, &dstRect);
}

void RenderManager::DrawGameOverText(const RenderGameOverTextEvent&) const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect dstRect{.x = 200, .y = 152, .w = 200, .h = 75};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.gameOverText.x),
							   .y = static_cast<int>(offset.gameOverText.y),
							   .w = static_cast<int>(offset.gameOverText.w),
							   .h = static_cast<int>(offset.gameOverText.h)};
	SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), &srcRect, &dstRect);
}

void RenderManager::DrawGameWonText(const RenderGameWonTextEvent&) const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect dstRect{.x = 250, .y = 152, .w = 120, .h = 85};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.gameWonText.x),
							   .y = static_cast<int>(offset.gameWonText.y),
							   .w = static_cast<int>(offset.gameWonText.w),
							   .h = static_cast<int>(offset.gameWonText.h)};
	SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), &srcRect, &dstRect);
}

void RenderManager::DrawRightSideBar(const RenderRightSideBarEvent&) const
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

void RenderManager::DrawEnemyIconBackground(const RenderEnemyIconBackgroundEvent&) const
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

void RenderManager::DrawEnemyIcons(const RenderEnemyIconsEvent& event) const
{
	const unsigned short numberOfIcons = event.count;
	constexpr TextureOffset offset{};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.enemyIcon.x),
							   .y = static_cast<int>(offset.enemyIcon.y),
							   .w = static_cast<int>(offset.enemyIcon.w),
							   .h = static_cast<int>(offset.enemyIcon.h)};

	for (unsigned short i = 0u; i < numberOfIcons; ++i)
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

void RenderManager::DrawPlayerOneIcons(const RenderPlayerOneIconEvent& event) const
{
	const unsigned short respawnCount = event.respawnCount;
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

void RenderManager::DrawPlayerTwoIcons(const RenderPlayerTwoIconEvent& event) const
{
	const unsigned short respawnCount = event.respawnCount;
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

void RenderManager::DrawStageNumber(const RenderStageNumberEvent& event) const
{
	const unsigned short currentStageNumber = event.stageNumber;
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
void RenderManager::DrawMenuBackground(const RenderMenuBackgroundEvent& event) const
{
	const Point pos = event.pos;
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

void RenderManager::DrawMenuLogo(const RenderMenuLogoEvent& event) const
{
	const Point pos = event.pos;
	const SDL_Rect rect{.x = pos.x + 135, .y = pos.y + 42, .w = 300, .h = 75};
	SDL_RenderCopy(_sdlConfig.renderer.get(), _sdlConfig.logoTexture.get(), nullptr, &rect);
}

void RenderManager::DrawSelectorIcon(const RenderMenuSelectorIconEvent& event) const
{
	const Point pos = event.pos;
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

void RenderManager::DrawXBoxHint(const RenderMenuXBoxHintEvent& event) const
{
	const Point pos = event.pos;
	RenderCopy(_sdlConfig.xboxTextures[3].get(), {.x = pos.x - 75, .y = pos.y + 93, .w = 30, .h = 30});//View button
	RenderCopy(_sdlConfig.xboxTextures[2].get(), {.x = pos.x - 75, .y = pos.y + 123, .w = 30, .h = 30});//Menu button
	RenderCopy(_sdlConfig.xboxTextures[5].get(), {.x = pos.x - 75, .y = pos.y + 153, .w = 30, .h = 30});//Y button
	RenderCopy(_sdlConfig.xboxTextures[0].get(), {.x = pos.x - 75, .y = pos.y + 183, .w = 30, .h = 30});//Dpad button
	RenderCopy(_sdlConfig.xboxTextures[1].get(), {.x = pos.x - 75, .y = pos.y + 63, .w = 30, .h = 30});//Home button
	RenderCopy(_sdlConfig.xboxTextures[4].get(), {.x = pos.x - 75, .y = pos.y + 213, .w = 30, .h = 30});//A button
}

void RenderManager::DrawPS5Hint(const RenderMenuPS5HintEvent& event) const
{
	const Point pos = event.pos;
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

void RenderManager::ClearFrame(const PreTickUpdateEvent&) const
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

void RenderManager::DrawColorTexture(const RenderColorTextureEvent& event)
{
	const ObjRectangle rect = event.rect;
	const SDL_Rect dstRect = RectToSdlRect(rect);
	constexpr unsigned int grayColor = 0x808080u;
	if (const auto it = _colorTextureCache.find(grayColor); it != _colorTextureCache.end())
	{
		SDL_RenderCopy(_sdlConfig.renderer.get(), it->second.get(), nullptr, &dstRect);
	}
}

void RenderManager::DrawTexture(const RenderTextureEvent& event) const
{
	//local angle and flip for texture
	auto [angle, flip] = GetRotateAndAngleAndFlip(event.dir);
	const SDL_Rect srcRect = RectToSdlRect(event.textureRect);
	const SDL_Rect dstRect = RectToSdlRect(event.destRect);
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


void RenderManager::RenderFPS(const RenderFPSEvent& event)
{
	const unsigned int fps = event.fps;
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

void RenderManager::DrawHealthBar(const RenderHealthBarEvent& event) const
{
	const auto& [rect, health] = event;
	const float pixelsPerHealthPoint = static_cast<float>(rect.w) / 100.0f;
	const float healthWidth = static_cast<float>(health) * pixelsPerHealthPoint;
	if (healthWidth <= 0.f)
	{
		return;
	}

	const int centerX = static_cast<int>(rect.x + rect.w / 2.f);
	const int barWidthInt = static_cast<int>(healthWidth);
	const int healthPosX = centerX - (barWidthInt / 2);

	const SDL_Rect healthBarRect{.x = healthPosX, .y = static_cast<int>(rect.y) - 10, .w = barWidthInt, .h = 5};

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
