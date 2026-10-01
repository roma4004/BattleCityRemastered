#include "components/managers/RenderManager.h"
#include "utils/SdlRenderUtils.h"
#include "geometry/Point.h"
#include "application/GameConfig.h"
#include "application/SDL_Config.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/PlayerSlot.h"
#include "geometry/ObjRectangle.h"
#include "utils/Log.h"
#include <algorithm>
#include <cmath>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <utility>

RenderManager::RenderManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig,
							 SDL_Config& sdlConfig)
	: _events{events}
	, _gameConfig{gameConfig}
	, _sdlConfig{sdlConfig}
{
	Subscribe();

	CreateColorTexture(kGrayColor);
	ApplyLogicalSize();
}

void RenderManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &RenderManager::ClearFrame));
	_subs.push_back(_events->AddListener(this, &RenderManager::PresentFrame));
	_subs.push_back(_events->AddListener(this, &RenderManager::OnGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &RenderManager::OnPlayerSlotAssigned));

	_subs.push_back(_events->AddListener(this, &RenderManager::DrawColorTexture));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawTexture));

	_subs.push_back(_events->AddListener(this, &RenderManager::DrawHealthBar));

	_subs.push_back(_events->AddListener(this, &RenderManager::OnWorldGeometryChanged));
	_subs.push_back(_events->AddListener(this, &RenderManager::OnWindowSizeChangedTo));

	_subs.push_back(_events->AddListener(this, &RenderManager::OnRenderTargetsReset));
	_subs.push_back(_events->AddListener(this, &RenderManager::OnRenderDeviceReset));
}

//NOTE: the 1x1 color texture is the only render target here
void RenderManager::OnRenderTargetsReset(const RenderTargetsResetEvent&)
{
	CreateColorTexture(kGrayColor);
}

void RenderManager::OnRenderDeviceReset(const RenderDeviceResetEvent&)
{
	_colorTextureCache.clear();

	CreateColorTexture(kGrayColor);

	//NOTE: images come back from their surfaces
	if (const auto recreated{_sdlConfig.RecreateTexturesFromSurfaces()};
		!recreated)
	{
		Log::Error(recreated.error().stage + ": " + recreated.error().detail);
	}

	//NOTE: a replaced renderer has no logical size; idempotent on one that survived
	ApplyLogicalSize();
}

void RenderManager::OnWorldGeometryChanged(const WorldGeometryChangedEvent&) { ApplyLogicalSize(); }

void RenderManager::OnWindowSizeChangedTo(const WindowSizeChangedToEvent&) { SnapWindowToLogicalAspect(); }

void RenderManager::SnapWindowToLogicalAspect() const
{
	const UPoint logicalSize{_gameConfig.LogicalSize()};
	if (logicalSize.x == 0u || logicalSize.y == 0u)
	{
		return;
	}

	const auto logicalWidth{static_cast<double>(logicalSize.x)};
	const auto logicalHeight{static_cast<double>(logicalSize.y)};

	SDL_Window* window{_sdlConfig.sdlWindow.get()};

	//NOTE: a maximized window is the window manager's to size - reshaping it here only fights it, so bars stay
	if ((SDL_GetWindowFlags(window) & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN)) != 0u)
	{
		return;
	}

	int windowWidth{};
	int windowHeight{};
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	//NOTE: the mean of both axes, so either edge dragged keeps roughly the asked size and the field's shape
	double scale{(static_cast<double>(windowWidth) / logicalWidth
				  + static_cast<double>(windowHeight) / logicalHeight) / 2.0};

	//NOTE: the desktop is the ceiling - a window the screen cannot hold is worse than a smaller one
	if (SDL_Rect usable{};
		SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(window), &usable))
	{
		scale = std::min(scale, std::min(static_cast<double>(usable.w) / logicalWidth,
										 static_cast<double>(usable.h) / logicalHeight));
	}

	const int snappedWidth{std::max(1, static_cast<int>(std::lround(logicalWidth * scale)))};
	const int snappedHeight{std::max(1, static_cast<int>(std::lround(logicalHeight * scale)))};
	if (snappedWidth == windowWidth && snappedHeight == windowHeight)
	{
		return;
	}

	SDL_SetWindowSize(window, snappedWidth, snappedHeight);
}

void RenderManager::ApplyLogicalSize()
{
	const UPoint logicalSize{_gameConfig.LogicalSize()};

	//NOTE: letterbox, not stretch - text sizing rests on the equal scale, and the bars are answered by the window shape
	SDL_SetRenderLogicalPresentation(_sdlConfig.renderer.get(),
									 static_cast<int>(logicalSize.x),
									 static_cast<int>(logicalSize.y),
									 SDL_LOGICAL_PRESENTATION_LETTERBOX);

	SnapWindowToLogicalAspect();
}

void RenderManager::SetRenderDrawColor(const unsigned int color, const Uint8 transparency) const
{
	const auto r{static_cast<Uint8>((color >> 16u) & 0xFFu)};
	const auto g{static_cast<Uint8>((color >> 8u) & 0xFFu)};
	const auto b{static_cast<Uint8>(color & 0xFFu)};
	const Uint8 a{transparency};

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

	SDL_RenderClear(_sdlConfig.renderer.get());

	SDL_SetRenderTarget(_sdlConfig.renderer.get(), nullptr);

	_colorTextureCache.insert_or_assign(color, std::move(colorTexture));
}

//NOTE: the clear covers the whole window - gray first, the field painted black over it, so only the bars stay gray
void RenderManager::ClearFrame(const PreTickUpdateEvent&) const
{
	SetRenderDrawColor(kGrayColor);
	SDL_RenderClear(_sdlConfig.renderer.get());

	const UPoint battlefieldSize{_gameConfig.battlefieldSize};
	const SDL_Rect field{
			.x = 0, .y = 0, .w = static_cast<int>(battlefieldSize.x), .h = static_cast<int>(battlefieldSize.y)};
	SDL_SetRenderDrawColor(_sdlConfig.renderer.get(), 0u, 0u, 0u, 255u);
	SdlRenderUtils::FillRect(_sdlConfig.renderer.get(), field);
}

void RenderManager::PresentFrame(const PresentFrameEvent&) const
{
	SDL_RenderPresent(_sdlConfig.renderer.get());
}

void RenderManager::OnGameModeChangedTo(const GameModeChangedToEvent& event)
{
	_titleMode = event.mode;
	//NOTE: a new match hands out seats again, and the old one would name the wrong window
	_titleSlot.reset();

	UpdateWindowTitle();
}

void RenderManager::OnPlayerSlotAssigned(const PlayerSlotAssignedEvent& event)
{
	_titleSlot = event.slot;

	UpdateWindowTitle();
}

//NOTE: two clients look alike, so the caption carries the seat - the keyboard half that drives this one
void RenderManager::UpdateWindowTitle() const
{
	std::string title{SDL_Config::kWindowTitle};

	if (IsHost(_titleMode))
	{
		title += " - host";
	}
	else if (IsClient(_titleMode))
	{
		title += _titleSlot ? (*_titleSlot == PlayerSlot::P1 ? " - client P1" : " - client P2")
							: " - client, waiting for a seat";
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
	const ObjRectangle rect{event.rect};
	const SDL_Rect dstRect{SdlRenderUtils::RectToSdlRect(rect)};
	if (const auto it{_colorTextureCache.find(kGrayColor)}; it != _colorTextureCache.end())
	{
		SdlRenderUtils::RenderCopy(_sdlConfig.renderer.get(), it->second.get(), dstRect);
	}
}

void RenderManager::DrawTexture(const RenderTextureEvent& event) const
{
	auto [angle, flip] = GetRotateAndAngleAndFlip(event.dir);
	const SDL_FRect src{SdlRenderUtils::ToFRect(SdlRenderUtils::RectToSdlRect(event.textureRect))};
	const SDL_FRect dst{SdlRenderUtils::ToFRect(SdlRenderUtils::RectToSdlRect(event.destRect))};
	SDL_Texture* atlas{_sdlConfig.atlasTexture.get()};

	if (event.color != 0u)
	{
		const auto [r, g, b, a] = SdlRenderUtils::IntToColor(event.color);
		SDL_SetTextureColorMod(atlas, r, g, b);
		SDL_RenderTextureRotated(_sdlConfig.renderer.get(), atlas, &src, &dst, angle, nullptr, flip);
		//NOTE: one atlas serves every draw - the tint has to be off again before the next one
		SDL_SetTextureColorMod(atlas, 255u, 255u, 255u);

		return;
	}

	SDL_RenderTextureRotated(_sdlConfig.renderer.get(), atlas, &src, &dst, angle, nullptr, flip);
}

void RenderManager::DrawHealthBar(const RenderHealthBarEvent& event) const
{
	const auto& [rect, health] = event;
	const float pixelsPerHealthPoint{static_cast<float>(rect.w) / 100.0f};
	const float healthWidth{static_cast<float>(health) * pixelsPerHealthPoint};
	if (healthWidth <= 0.f)
	{
		return;
	}

	const auto centerX{static_cast<int>(rect.x + rect.w / 2.0)};
	const auto barWidthInt{static_cast<int>(healthWidth)};
	const int healthPosX{centerX - (barWidthInt / 2)};

	const SDL_Rect healthBarRect{.x = healthPosX, .y = static_cast<int>(rect.y) - 10, .w = barWidthInt, .h = 5};

	unsigned int color;
	if (health > 70)
	{
		constexpr unsigned int colorGreen{0x408000u};
		color = colorGreen;
	}
	else if (health > 30)
	{
		constexpr unsigned int colorYellow{0xEAEA00u};
		color = colorYellow;
	}
	else
	{
		constexpr unsigned int colorRed{0xFF8080u};
		color = colorRed;
	}

	SetRenderDrawColor(color, 127u);
	SdlRenderUtils::FillRect(_sdlConfig.renderer.get(), healthBarRect);
}
