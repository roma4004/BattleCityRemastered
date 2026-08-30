#include "components/managers/RenderManager.h"
#include "geometry/Point.h"
#include "application/GameConfig.h"
#include "application/SDL_Config.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/TextureOffset.h"
#include "utils/Log.h"
#include <algorithm>
#include <cmath>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <string>

RenderManager::RenderManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig, SDL_Config& sdlConfig)
	: _events{events}
	, _gameConfig{gameConfig}
	, _sdlConfig{sdlConfig}
	, _fpsBox{CalcFpsBox(gameConfig.battlefieldSize)}
	, _textCache{sdlConfig}
{
	Subscribe();

	InitMenu(gameConfig);
	ApplyLogicalSize();
}

void RenderManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &RenderManager::ClearFrame));
	_subs.push_back(_events->AddListener(this, &RenderManager::PresentFrame));
	_subs.push_back(_events->AddListener(this, &RenderManager::OnGameModeChangedTo));
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

	_subs.push_back(_events->AddListener(this, &RenderManager::OnWorldGeometryChanged));
	_subs.push_back(_events->AddListener(this, &RenderManager::OnWindowSizeChangedTo));

	_subs.push_back(_events->AddListener(this, &RenderManager::OnRenderTargetsReset));
	_subs.push_back(_events->AddListener(this, &RenderManager::OnRenderDeviceReset));
}

//NOTE: the 1x1 colour texture is the only render target here
void RenderManager::OnRenderTargetsReset(const RenderTargetsResetEvent&)
{
	CreateColorTexture(kGrayColor);
}

void RenderManager::OnRenderDeviceReset(const RenderDeviceResetEvent&)
{
	//NOTE: text comes back from the font, images from their surfaces
	_textCache.Clear();
	_colorTextureCache.clear();

	CreateColorTexture(kGrayColor);

	if (const auto recreated = _sdlConfig.RecreateTexturesFromSurfaces();
		!recreated)
	{
		Log::Error(recreated.error().stage + ": " + recreated.error().detail);
	}

	//NOTE: a replaced renderer has no logical size; idempotent on one that survived
	ApplyLogicalSize();
}

void RenderManager::OnRenderText(const RenderTextEvent& event) const
{
	TextToRender(event.pos, IntToColor(event.color), event.text);
}

void RenderManager::OnWorldGeometryChanged(const WorldGeometryChangedEvent&)
{
	_fpsBox = CalcFpsBox(_gameConfig.battlefieldSize);

	InitMenu(_gameConfig);
	ApplyLogicalSize();
}

void RenderManager::OnWindowSizeChangedTo(const WindowSizeChangedToEvent&) { SnapWindowToLogicalAspect(); }

void RenderManager::SnapWindowToLogicalAspect() const
{
	const UPoint logicalSize = _gameConfig.LogicalSize();
	if (logicalSize.x == 0u || logicalSize.y == 0u)
	{
		return;
	}

	const double logicalWidth = static_cast<double>(logicalSize.x);
	const double logicalHeight = static_cast<double>(logicalSize.y);

	SDL_Window* window = _sdlConfig.sdlWindow.get();

	//NOTE: a maximized or fullscreen window is the window manager's to size - reshaping it here only
	//fights it, so the letterbox stays and ClearFrame paints its bars instead
	if ((SDL_GetWindowFlags(window) & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN)) != 0u)
	{
		return;
	}

	int windowWidth{};
	int windowHeight{};
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	//NOTE: the mean of the two axes, so it does not matter which edge was dragged - the window keeps
	//roughly the size the drag asked for and takes the field's shape
	double scale = (static_cast<double>(windowWidth) / logicalWidth
					+ static_cast<double>(windowHeight) / logicalHeight) / 2.0;

	//NOTE: the desktop is the ceiling - a window the screen cannot hold is worse than a smaller one
	if (SDL_Rect usable{};
		SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(window), &usable))
	{
		scale = std::min(scale, std::min(static_cast<double>(usable.w) / logicalWidth,
										 static_cast<double>(usable.h) / logicalHeight));
	}

	const int snappedWidth = std::max(1, static_cast<int>(std::lround(logicalWidth * scale)));
	const int snappedHeight = std::max(1, static_cast<int>(std::lround(logicalHeight * scale)));
	if (snappedWidth == windowWidth && snappedHeight == windowHeight)
	{
		return;
	}

	SDL_SetWindowSize(window, snappedWidth, snappedHeight);
}

void RenderManager::ApplyLogicalSize()
{
	const UPoint logicalSize = _gameConfig.LogicalSize();

	//NOTE: letterbox, not stretch - the equal scale on both axes is what the text sizing rests on. The
	//bars it would add are answered by shaping the window itself, not by distorting the field.
	SDL_SetRenderLogicalPresentation(_sdlConfig.renderer.get(),
									 static_cast<int>(logicalSize.x),
									 static_cast<int>(logicalSize.y),
									 SDL_LOGICAL_PRESENTATION_LETTERBOX);

	SnapWindowToLogicalAspect();
}

void RenderManager::DrawPauseText(const RenderPauseTextEvent&) const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect dstRect{.x = 135, .y = 142, .w = 300, .h = 75};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.pauseText.x),
							   .y = static_cast<int>(offset.pauseText.y),
							   .w = static_cast<int>(offset.pauseText.w),
							   .h = static_cast<int>(offset.pauseText.h)};
	RenderCopyWithClipping(_sdlConfig.atlasTexture.get(), srcRect, dstRect);
}

void RenderManager::DrawGameOverText(const RenderGameOverTextEvent&) const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect dstRect{.x = 200, .y = 152, .w = 200, .h = 75};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.gameOverText.x),
							   .y = static_cast<int>(offset.gameOverText.y),
							   .w = static_cast<int>(offset.gameOverText.w),
							   .h = static_cast<int>(offset.gameOverText.h)};
	RenderCopyWithClipping(_sdlConfig.atlasTexture.get(), srcRect, dstRect);
}

void RenderManager::DrawGameWonText(const RenderGameWonTextEvent&) const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect dstRect{.x = 250, .y = 152, .w = 120, .h = 85};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.gameWonText.x),
							   .y = static_cast<int>(offset.gameWonText.y),
							   .w = static_cast<int>(offset.gameWonText.w),
							   .h = static_cast<int>(offset.gameWonText.h)};
	RenderCopyWithClipping(_sdlConfig.atlasTexture.get(), srcRect, dstRect);
}

void RenderManager::DrawRightSideBar(const RenderRightSideBarEvent&) const
{
	SDL_Rect backgroundRect{RectToSdlRect(TextureOffset{}.rightSideBar)};
	backgroundRect.x = static_cast<int>(_gameConfig.battlefieldSize.x);
	constexpr unsigned int color{0xFF808080u};
	constexpr Uint8 a{(color >> 24u) & 0xFFu};
	constexpr Uint8 r{(color >> 16u) & 0xFFu};
	constexpr Uint8 g{(color >> 8u) & 0xFFu};
	constexpr Uint8 b{(color >> 0u) & 0xFFu};
	SDL_SetRenderDrawColor(_sdlConfig.renderer.get(), r, g, b, a);
	FillRect(backgroundRect);
}

void RenderManager::DrawEnemyIconBackground(const RenderEnemyIconBackgroundEvent&) const
{
	constexpr TextureOffset offset{};
	const int posX{static_cast<int>(_gameConfig.battlefieldSize.x) + kEnemyIconColumnPadding};
	const SDL_Rect dstRect{.x = posX, .y = kEnemyIconBackgroundTop, .w = kEnemyIconBackgroundWidth, .h = 277};
	constexpr SDL_Rect srcRect{.x = static_cast<int>(offset.enemyIconBackground.x),
							   .y = static_cast<int>(offset.enemyIconBackground.y),
							   .w = static_cast<int>(offset.enemyIconBackground.w),
							   .h = static_cast<int>(offset.enemyIconBackground.h)};
	RenderCopyWithClipping(_sdlConfig.atlasTexture.get(), srcRect, dstRect);
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
		const Point startPos{.x = static_cast<int>(_gameConfig.battlefieldSize.x) + iconPadding,
							 .y = 65};
		constexpr Point imageSize{.x = 30, .y = 25};
		constexpr Point padding{.x = 1, .y = 2};

		const int row{i / columns};
		const int col{i % columns};
		const int posX{startPos.x + col * (imageSize.x + padding.x)};
		const int posY{startPos.y + row * (imageSize.y + padding.y)};

		SDL_Rect destRect = {.x = posX, .y = posY, .w = imageSize.x, .h = imageSize.y};
		RenderCopyWithClipping(_sdlConfig.atlasTexture.get(), srcRect, destRect);
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
	const int posX{static_cast<int>(_gameConfig.battlefieldSize.x) + padding};
	const SDL_Rect rect{.x = posX, .y = 350, .w = 71, .h = 70};
	RenderCopyWithClipping(_sdlConfig.atlasTexture.get(), srcRect, rect);

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
	const int posX{static_cast<int>(_gameConfig.battlefieldSize.x) + padding};
	const SDL_Rect rect{.x = posX, .y = 420, .w = 71, .h = 70};
	RenderCopyWithClipping(_sdlConfig.atlasTexture.get(), srcRect, rect);

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
	const int posX{static_cast<int>(_gameConfig.battlefieldSize.x) + padding};
	const SDL_Rect rect{.x = posX, .y = 490, .w = 71, .h = 95};
	RenderCopyWithClipping(_sdlConfig.atlasTexture.get(), srcRect, rect);

	constexpr bool isMediumFontSize{true};
	constexpr int textPadding{38};
	TextToRender(Point{.x = posX + textPadding, .y = 555}, IntToColor(2u), currentStageNumber, isMediumFontSize);
}

unsigned int RenderManager::ColorToInt(const SDL_Color& color)
{
	return ComponentsToColor(color.r, color.g, color.b, color.a);
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
	//NOTE: Uint8 promotes to int, and shifting 255 by 24 leaves the sign bit - the widening keeps the
	//whole thing unsigned
	return (Uint32{a} << 24u) | (Uint32{r} << 16u) | (Uint32{g} << 8u) | Uint32{b};
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
	FillRect(backgroundRect);
}

void RenderManager::DrawMenuLogo(const RenderMenuLogoEvent& event) const
{
	const Point pos = event.pos;
	const SDL_Rect rect{.x = pos.x + 135, .y = pos.y + 42, .w = 300, .h = 75};
	RenderCopy(_sdlConfig.logoTexture.get(), rect);
}

void RenderManager::DrawSelectorIcon(const RenderMenuSelectorIconEvent& event) const
{
	const Point pos = event.pos;
	const SDL_Rect rect{.x = pos.x, .y = pos.y, .w = 30, .h = 30};
	RenderCopy(_sdlConfig.selectorIconTexture.get(), rect);
}

void RenderManager::RenderCopyWithClipping(SDL_Texture* texture, const SDL_Rect srcRect, const SDL_Rect dstRect) const
{
	const SDL_FRect src = ToFRect(srcRect);
	const SDL_FRect dst = ToFRect(dstRect);
	SDL_RenderTexture(_sdlConfig.renderer.get(), texture, &src, &dst);
}

void RenderManager::RenderCopy(SDL_Texture* texture, const SDL_Rect dstRect) const
{
	const SDL_FRect dst = ToFRect(dstRect);
	SDL_RenderTexture(_sdlConfig.renderer.get(), texture, nullptr, &dst);
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

int RenderManager::BasePointSize(const bool isMediumFontSize)
{
	return isMediumFontSize ? SDL_Config::kFontSizePtMedium : SDL_Config::kFontSizePtSmall;
}

float RenderManager::CurrentRenderScale() const
{
	//NOTE: SDL3 keeps the logical presentation apart from the render scale, so SDL_GetRenderScale no
	//longer reports it - the letterbox rect is what maps a logical pixel onto the window
	int logicalWidth{};
	int logicalHeight{};
	SDL_RendererLogicalPresentation mode{SDL_LOGICAL_PRESENTATION_DISABLED};
	SDL_FRect presentation{};
	if (!SDL_GetRenderLogicalPresentation(_sdlConfig.renderer.get(), &logicalWidth, &logicalHeight, &mode)
		|| mode == SDL_LOGICAL_PRESENTATION_DISABLED
		|| logicalWidth <= 0
		|| !SDL_GetRenderLogicalPresentationRect(_sdlConfig.renderer.get(), &presentation))
	{
		return 1.f;
	}

	const float scale = presentation.w / static_cast<float>(logicalWidth);

	return scale > 0.f ? scale : 1.f;
}

void RenderManager::TextToRender(const Point pos, const SDL_Color color, const std::string& text,
								 const bool isMediumFontSize) const
{
	TextToRenderSized(pos, color, text, BasePointSize(isMediumFontSize));
}

void RenderManager::TextToRenderSized(const Point pos, const SDL_Color color, const std::string& text,
									  const int basePointSize) const
{
	const TextTextureCache::CachedText* cached = _textCache.Acquire(text, color, basePointSize, CurrentRenderScale());
	if (cached == nullptr)
	{
		return;
	}

	const SDL_Rect textRect{.x = pos.x, .y = pos.y, .w = cached->width, .h = cached->height};
	RenderCopy(cached->texture.get(), textRect);
}

void RenderManager::TextToRenderCentered(const SDL_Rect& box, const SDL_Color color, const std::string& text,
										 const int basePointSize) const
{
	const TextTextureCache::CachedText* cached = _textCache.Acquire(text, color, basePointSize,
																	CurrentRenderScale());
	if (cached == nullptr)
	{
		return;
	}

	const SDL_Rect textRect{.x = box.x + (box.w - cached->width) / 2,
							.y = box.y + (box.h - cached->height) / 2,
							.w = cached->width,
							.h = cached->height};
	RenderCopy(cached->texture.get(), textRect);
}

void RenderManager::TextToRenderInBox(const SDL_Rect& box, const SDL_Color color, const std::string& text,
									  const bool isMediumFontSize) const
{
	const TextTextureCache::CachedText* cached =
			_textCache.Acquire(text, color, BasePointSize(isMediumFontSize), CurrentRenderScale());
	if (cached == nullptr)
	{
		return;
	}

	RenderCopy(cached->texture.get(), box);
}

inline SDL_Rect RenderManager::RectToSdlRect(const ObjRectangle& rect)
{
	return SDL_Rect{.x = static_cast<int>(rect.x),
					.y = static_cast<int>(rect.y),
					.w = static_cast<int>(rect.w),
					.h = static_cast<int>(rect.h)};
}

SDL_FRect RenderManager::ToFRect(const SDL_Rect& rect)
{
	return SDL_FRect{.x = static_cast<float>(rect.x),
					 .y = static_cast<float>(rect.y),
					 .w = static_cast<float>(rect.w),
					 .h = static_cast<float>(rect.h)};
}

void RenderManager::FillRect(const SDL_Rect& rect) const
{
	const SDL_FRect target = ToFRect(rect);
	SDL_RenderFillRect(_sdlConfig.renderer.get(), &target);
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

//NOTE: the clear ignores the logical presentation and covers the whole window, so the gray goes down
//first and the field is painted black back over it - what stays gray is exactly the letterbox bars
void RenderManager::ClearFrame(const PreTickUpdateEvent&) const
{
	SetRenderDrawColor(kGrayColor);
	SDL_RenderClear(_sdlConfig.renderer.get());

	const UPoint logicalSize = _gameConfig.LogicalSize();
	SDL_SetRenderDrawColor(_sdlConfig.renderer.get(), 0u, 0u, 0u, 255u);
	FillRect(SDL_Rect{.x = 0,
					  .y = 0,
					  .w = static_cast<int>(logicalSize.x),
					  .h = static_cast<int>(logicalSize.y)});
}

void RenderManager::PresentFrame(const PresentFrameEvent&) const
{
	SDL_RenderPresent(_sdlConfig.renderer.get());
}

void RenderManager::OnGameModeChangedTo(const GameModeChangedToEvent& event) const { UpdateWindowTitle(event.mode); }

void RenderManager::UpdateWindowTitle(const GameMode gameMode) const
{
	std::string title{SDL_Config::kWindowTitle};

	if (IsHost(gameMode))
	{
		title += " - host";
	}
	else if (IsClient(gameMode))
	{
		title += " - client";
	}

	SDL_SetWindowTitle(_sdlConfig.sdlWindow.get(), title.c_str());
}

std::pair<double, SDL_FlipMode> RenderManager::GetRotateAndAngleAndFlip(const Direction dir)
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
	if (const auto it = _colorTextureCache.find(kGrayColor); it != _colorTextureCache.end())
	{
		RenderCopy(it->second.get(), dstRect);
	}
}

void RenderManager::DrawTexture(const RenderTextureEvent& event) const
{
	//local angle and flip for texture
	auto [angle, flip] = GetRotateAndAngleAndFlip(event.dir);
	const SDL_FRect src = ToFRect(RectToSdlRect(event.textureRect));
	const SDL_FRect dst = ToFRect(RectToSdlRect(event.destRect));
	SDL_Texture* atlas = _sdlConfig.atlasTexture.get();

	if (event.color != 0u)
	{
		const auto [r, g, b, a] = IntToColor(event.color);
		SDL_SetTextureColorMod(atlas, r, g, b);
		SDL_RenderTextureRotated(_sdlConfig.renderer.get(), atlas, &src, &dst, angle, nullptr, flip);
		//NOTE: one atlas serves every draw - the tint has to be off again before the next one
		SDL_SetTextureColorMod(atlas, 255u, 255u, 255u);

		return;
	}

	SDL_RenderTextureRotated(_sdlConfig.renderer.get(), atlas, &src, &dst, angle, nullptr, flip);
}

void RenderManager::RenderFPS(const RenderFPSEvent& event) const
{
	const unsigned int fps = event.fps;
	if (fps == 0u)
	{
		return;
	}

	constexpr SDL_Color textColor{.r = 140u, .g = 0u, .b = 255u, .a = 255u};
	//NOTE: three digits at the medium size are 72 px against a 71 px column - the pixel over the edge
	//buys reusing the one font opened at startup
	TextToRenderCentered(_fpsBox, textColor, std::to_string(fps), SDL_Config::kFontSizePtMedium);
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

	const int centerX = static_cast<int>(rect.x + rect.w / 2.0);
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
	FillRect(healthBarRect);
}

void RenderManager::InitMenu(const GameConfig& gameConfig)
{
	_menuParams.Init(gameConfig.LogicalSize(), gameConfig.sideBarWidth);

	CreateColorTexture(kGrayColor);
}

SDL_Rect RenderManager::CalcFpsBox(const UPoint& battlefieldSize)
{
	return SDL_Rect{.x = static_cast<int>(battlefieldSize.x) + kEnemyIconColumnPadding,
					.y = 0,
					.w = kEnemyIconBackgroundWidth,
					.h = kEnemyIconBackgroundTop};
}
