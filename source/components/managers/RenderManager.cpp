#include "components/managers/RenderManager.h"
#include "components/UiLayout.h"
#include "components/UiTable.h"
#include "enums/UiIcon.h"
#include "utils/MathUtils.h"
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
#include "enums/PlayerSlot.h"
#include "enums/TextureOffset.h"
#include "geometry/ObjRectangle.h"
#include "utils/Log.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <optional>
#include <ranges>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace
{
//NOTE: glyphs are sized in output pixels, so the logical scale is cancelled around the drawing and
//folded into the position - once around a run of lines, because every change of it breaks the batch
class ScopedRenderScale final
{
public:
	ScopedRenderScale(SDL_Renderer* const renderer, const float scale)
		: _renderer{renderer}
	{
		SDL_SetRenderScale(_renderer, 1.f / scale, 1.f / scale);
	}

	~ScopedRenderScale() { SDL_SetRenderScale(_renderer, 1.f, 1.f); }

	ScopedRenderScale(const ScopedRenderScale&) = delete;
	ScopedRenderScale& operator=(const ScopedRenderScale&) = delete;
	ScopedRenderScale(ScopedRenderScale&&) = delete;
	ScopedRenderScale& operator=(ScopedRenderScale&&) = delete;

private:
	SDL_Renderer* _renderer;
};
}

RenderManager::RenderManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig,
							 SDL_Config& sdlConfig)
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
	_subs.push_back(_events->AddListener(this, &RenderManager::OnPlayerSlotAssigned));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawPanelTables));

	_subs.push_back(_events->AddListener(this, &RenderManager::DrawMenuBackground));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawMenu));

	_subs.push_back(_events->AddListener(this, &RenderManager::DrawPlate));

	_subs.push_back(_events->AddListener(this, &RenderManager::DrawColorTexture));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawTexture));

	_subs.push_back(_events->AddListener(this, &RenderManager::RenderFPS));

	_subs.push_back(_events->AddListener(this, &RenderManager::DrawHealthBar));
	_subs.push_back(_events->AddListener(this, &RenderManager::DrawSideBar));

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
	//NOTE: text comes back from the font, images from their surfaces
	_textCache.Clear();
	_colorTextureCache.clear();

	CreateColorTexture(kGrayColor);

	if (const auto recreated{_sdlConfig.RecreateTexturesFromSurfaces()};
		!recreated)
	{
		Log::Error(recreated.error().stage + ": " + recreated.error().detail);
	}

	//NOTE: a replaced renderer has no logical size; idempotent on one that survived
	ApplyLogicalSize();
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

SDL_Rect RenderManager::CenteredInField(const int width, const int height) const
{
	const UPoint field{_gameConfig.battlefieldSize};

	return SDL_Rect{.x = (static_cast<int>(field.x) - width) / 2,
					.y = (static_cast<int>(field.y) - height) / 2,
					.w = width,
					.h = height};
}

Point RenderManager::PlateSize(const ObjRectangle& sprite, const double widthShare) const
{
	const auto width{static_cast<int>(static_cast<double>(_gameConfig.battlefieldSize.x) * widthShare)};

	return Point{.x = width, .y = static_cast<int>(static_cast<double>(width) * sprite.h / sprite.w)};
}

void RenderManager::DrawPlate(const RenderPlateEvent& event) const
{
	const auto [width, height]{Icon(event.plate).size};
	DrawIcon(event.plate, CenteredInField(width, height));
}

void RenderManager::DrawSideBar(const RenderSideBarEvent& event) const
{
	const float scale{CurrentRenderScale()};
	const UiLayout::Measure measure{CellMeasurer(SDL_Config::kFontSizePtMedium, scale)};

	const SDL_Rect frame{
			.x = SideBarColumnX(), .y = _fpsBox.y + _fpsBox.h, .w = kSideBarItemWidth, .h = kReserveFrameHeight};
	RenderCopyWithClipping(_sdlConfig.atlasTexture.get(), RectToSdlRect(TextureOffset::kEnemyIconBackground), frame);

	const Point reserveAt{.x = frame.x + kReserveInset.x, .y = frame.y + kReserveInset.y};
	const UiLayout::Placement enemies{
			UiLayout::Place(event.enemies, reserveAt, kReserveRowHeight, measure, kReserveColumnGap)};
	const Point countersAt{.x = frame.x, .y = frame.y + frame.h + kReserveGapBelow};
	const UiLayout::Placement counters{UiLayout::Place(event.counters, countersAt, 0, measure)};

	DrawTablePictures(event.enemies, enemies);
	DrawTablePictures(event.counters, counters);

	const ScopedRenderScale scaled{_sdlConfig.renderer.get(), scale};
	DrawTableText(event.counters, counters, SDL_Config::kFontSizePtMedium, scale);
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
	//NOTE: Uint8 promotes to int and 255 << 24 lands on the sign bit - the widening keeps it unsigned
	return (Uint32{a} << 24u) | (Uint32{r} << 16u) | (Uint32{g} << 8u) | Uint32{b};
}

void RenderManager::DrawMenuBackground(const RenderMenuBackgroundEvent& event) const
{
	const SDL_Rect backgroundRect{MenuPanelRect(event.slide)};
	constexpr unsigned int color{0x91808080u};
	constexpr Uint8 a{(color >> 24u) & 0xFFu};
	constexpr Uint8 r{(color >> 16u) & 0xFFu};
	constexpr Uint8 g{(color >> 8u) & 0xFFu};
	constexpr Uint8 b{(color >> 0u) & 0xFFu};
	SDL_SetRenderDrawColor(_sdlConfig.renderer.get(), r, g, b, a);
	FillRect(backgroundRect);
}

RenderManager::IconSource RenderManager::Icon(const UiIcon icon) const
{
	constexpr Point menuSquare{.x = kMenuIconSize, .y = kMenuIconSize};
	SDL_Texture* const atlas{_sdlConfig.atlasTexture.get()};
	switch (icon)
	{
		case UiIcon::None:
			break;
		case UiIcon::MenuLogo:
			return IconSource{.texture = _sdlConfig.logoTexture.get(), .size = kMenuLogoSize};
		case UiIcon::MenuSelector:
			return IconSource{.texture = _sdlConfig.selectorIconTexture.get(), .size = menuSquare};
		case UiIcon::MenuXBoxHome:
			return IconSource{.texture = _sdlConfig.xboxTextures[1].get(), .size = menuSquare};
		case UiIcon::MenuXBoxView:
			return IconSource{.texture = _sdlConfig.xboxTextures[3].get(), .size = menuSquare};
		case UiIcon::MenuXBoxMenu:
			return IconSource{.texture = _sdlConfig.xboxTextures[2].get(), .size = menuSquare};
		case UiIcon::MenuXBoxY:
			return IconSource{.texture = _sdlConfig.xboxTextures[5].get(), .size = menuSquare};
		case UiIcon::MenuXBoxDpad:
			return IconSource{.texture = _sdlConfig.xboxTextures[0].get(), .size = menuSquare};
		case UiIcon::MenuXBoxA:
			return IconSource{.texture = _sdlConfig.xboxTextures[4].get(), .size = menuSquare};
		case UiIcon::MenuPS5Home:
			return IconSource{.texture = _sdlConfig.ps5Textures[3].get(), .size = menuSquare};
		case UiIcon::MenuPS5Create:
			return IconSource{.texture = _sdlConfig.ps5Textures[0].get(), .size = menuSquare};
		case UiIcon::MenuPS5Options:
			return IconSource{.texture = _sdlConfig.ps5Textures[4].get(), .size = menuSquare};
		case UiIcon::MenuPS5Triangle:
			return IconSource{.texture = _sdlConfig.ps5Textures[5].get(), .size = menuSquare};
		case UiIcon::MenuPS5Dpad:
			return IconSource{.texture = _sdlConfig.ps5Textures[2].get(), .size = menuSquare};
		case UiIcon::MenuPS5Cross:
			return IconSource{.texture = _sdlConfig.ps5Textures[1].get(), .size = menuSquare};
		case UiIcon::SideBarEnemyTank:
			return IconSource{.texture = atlas,
							  .sprite = RectToSdlRect(TextureOffset::kEnemyIcon),
							  .size = kSideBarEnemyTankSize};
		case UiIcon::SideBarPlayerOne:
			return IconSource{.texture = atlas,
							  .sprite = RectToSdlRect(TextureOffset::kPlayer1Icon),
							  .size = kSideBarLivesSize};
		case UiIcon::SideBarPlayerTwo:
			return IconSource{.texture = atlas,
							  .sprite = RectToSdlRect(TextureOffset::kPlayer2Icon),
							  .size = kSideBarLivesSize};
		case UiIcon::SideBarStageFlag:
			return IconSource{.texture = atlas,
							  .sprite = RectToSdlRect(TextureOffset::kStageNumberFlag),
							  .size = kSideBarStageSize};
		case UiIcon::PlatePause:
			return IconSource{.texture = atlas,
							  .sprite = RectToSdlRect(TextureOffset::kPauseText),
							  .size = PlateSize(TextureOffset::kPauseText, kPausePlateShare)};
		case UiIcon::PlateGameOver:
			return IconSource{.texture = atlas,
							  .sprite = RectToSdlRect(TextureOffset::kGameOverText),
							  .size = PlateSize(TextureOffset::kGameOverText, kGameOverPlateShare)};
		case UiIcon::PlateGameWon:
			return IconSource{.texture = atlas,
							  .sprite = RectToSdlRect(TextureOffset::kGameWonText),
							  .size = PlateSize(TextureOffset::kGameWonText, kGameWonPlateShare)};
	}

	return IconSource{};
}

void RenderManager::DrawIcon(const UiIcon icon, const SDL_Rect dstRect) const
{
	const IconSource source{Icon(icon)};
	if (source.sprite)
	{
		RenderCopyWithClipping(source.texture, *source.sprite, dstRect);
		return;
	}

	RenderCopy(source.texture, dstRect);
}

UiLayout::Measure RenderManager::CellMeasurer(const int pointSize, const float scale) const
{
	return [this, pointSize, scale](const UiCell& cell)
	{
		if (cell.icon != UiIcon::None)
		{
			return Icon(cell.icon).size;
		}

		return _textCache.MeasureString(cell.text, pointSize, scale);
	};
}

//NOTE: what goes in keeps going in as it shrinks, so the smallest refused size bounds the answer
int RenderManager::FitPointSize(const std::function<bool(int)>& goesIn)
{
	const auto sizes{std::views::iota(kFitMinPointSize, FitStartPointSize() + 1)};
	const auto firstRefused{std::ranges::partition_point(sizes, goesIn)};

	return firstRefused == sizes.begin() ? kFitMinPointSize : *std::ranges::prev(firstRefused);
}

std::vector<UiLayout::Placement> RenderManager::MeasureAll(const std::span<const UiTable> tables, const int rowHeight,
														   const UiLayout::Measure& measure)
{
	const auto measured = [rowHeight, &measure](const UiTable& table)
	{
		return UiLayout::Place(table, Point{}, rowHeight, measure);
	};

	return tables | std::views::transform(measured) | std::ranges::to<std::vector>();
}

bool RenderManager::FitsAcross(const std::span<const UiTable> tables, const std::span<const UiLayout::Placement> placed,
							   const int width, const int rowHeight)
{
	const auto isWord = [](const UiCell& cell) { return cell.icon == UiIcon::None && !cell.text.empty(); };
	const auto goesIn = [isWord, width, rowHeight](const UiTable& table, const UiLayout::Placement& placement)
	{
		const auto isLowEnough = [&table, isWord, rowHeight](const UiLayout::PlacedCell& cell)
		{
			return !isWord(table.rows[cell.row].cells[cell.column]) || cell.size.y <= rowHeight;
		};
		const bool hasWords{std::ranges::any_of(table.rows, [isWord](const UiRow& row)
		{
			return std::ranges::any_of(row.cells, isWord);
		})};

		return (!hasWords || placement.size.x <= width) && std::ranges::all_of(placement.cells, isLowEnough);
	};

	return std::ranges::all_of(std::views::zip(tables, placed),
							   [&goesIn](const auto& pair) { return std::apply(goesIn, pair); });
}

//NOTE: measured once, then moved to where its width sits in the middle - no second layout
UiLayout::Placement RenderManager::CenteredAcross(UiLayout::Placement placement, const SDL_Rect& panel, const int top)
{
	placement.ShiftBy(Point{.x = panel.x + (panel.w - placement.size.x) / 2, .y = panel.y + top});

	return placement;
}

int RenderManager::PanelRowHeight(const int pointSize) { return pointSize * kPanelRowHeight / FitStartPointSize(); }

int RenderManager::StackHeight(const std::span<const UiLayout::Placement> placed, const int gap)
{
	const auto heightOf = [](const UiLayout::Placement& placement) { return placement.size.y; };

	return std::ranges::fold_left(placed | std::views::transform(heightOf), 0, std::plus{})
		   + gap * (static_cast<int>(placed.size()) - 1);
}

//NOTE: as large as the stack lets it be - the plate does not shrink, so the words make the room it needs
int RenderManager::PanelPointSize(const std::vector<UiTable>& tables, const SDL_Rect& panel, const float scale) const
{
	const Point panelSize{.x = panel.w, .y = panel.h};
	if (_panelFit.tables == tables && _panelFit.panelSize == panelSize
		&& MathUtils::AreEqualAbsolute(static_cast<double>(_panelFit.scale), static_cast<double>(scale)))
	{
		return _panelFit.pointSize;
	}

	const int width{panel.w - kPanelSideMargin * 2};
	const int pointSize{FitPointSize([this, &tables, &panel, width, scale](const int size)
	{
		const int rowHeight{PanelRowHeight(size)};
		const auto placed{MeasureAll(tables, rowHeight, CellMeasurer(size, scale))};

		return FitsAcross(tables, placed, width, rowHeight) && StackHeight(placed, rowHeight) <= panel.h;
	})};

	_panelFit = PanelFit{.tables = tables, .scale = scale, .panelSize = panelSize, .pointSize = pointSize};

	return pointSize;
}

void RenderManager::DrawTablePictures(const UiTable& table, const UiLayout::Placement& placement) const
{
	for (const UiLayout::PlacedCell& placed: placement.cells)
	{
		const UiCell& cell{table.rows[placed.row].cells[placed.column]};
		if (cell.background != UiIcon::None)
		{
			DrawIcon(cell.background, RectOf(placed.boxPos, placed.boxSize));
		}

		if (cell.icon != UiIcon::None)
		{
			DrawIcon(cell.icon, RectOf(placed.pos, placed.size));
		}
	}
}

void RenderManager::DrawTableText(const UiTable& table, const UiLayout::Placement& placement, const int pointSize,
								  const float scale) const
{
	for (const UiLayout::PlacedCell& placed: placement.cells)
	{
		const UiCell& cell{table.rows[placed.row].cells[placed.column]};
		if (cell.icon != UiIcon::None || cell.text.empty())
		{
			continue;
		}

		DrawTextAt(placed.pos, IntToColor(cell.color), cell.text, pointSize, scale);
	}
}

//NOTE: sent on, not kept - only this side knows where the rows ended up once the size was fitted
void RenderManager::AnnounceMenuTiles(const UiLayout::Placement& modes) const
{
	std::vector<Point> tiles{};
	tiles.reserve(modes.rows.size());
	for (const Point& row: modes.rows)
	{
		tiles.push_back(Point{.x = row.x - kMenuRowPadding, .y = row.y});
	}

	if (tiles == _menuTilePlaces)
	{
		return;
	}

	_menuTilePlaces = tiles;
	_events->EmitEvent(MenuTilesPlacedEvent{.tiles = std::move(tiles),
										 .tileSize = {.x = modes.size.x + kMenuRowPadding * 2,
													 .y = kMenuRowHeight}});
}

void RenderManager::DrawMenu(const RenderMenuEvent& event) const
{
	const SDL_Rect panel{MenuPanelRect(event.slide)};
	const float scale{CurrentRenderScale()};
	if (!MathUtils::AreEqualAbsolute(static_cast<double>(scale), static_cast<double>(_menuScale)))
	{
		_menuScale = scale;
		const std::array tables{event.modes, event.controls};
		const int width{panel.w - kPanelSideMargin * 2};
		_menuPointSize = FitPointSize([this, &tables, width, scale](const int size)
		{
			return FitsAcross(tables, MeasureAll(tables, kMenuRowHeight, CellMeasurer(size, scale)), width,
							  kMenuRowHeight);
		});
	}

	const UiLayout::Measure measure{CellMeasurer(_menuPointSize, scale)};
	const auto measured = [&measure](const UiTable& table)
	{
		return UiLayout::Place(table, Point{}, kMenuRowHeight, measure);
	};
	const UiLayout::Placement title{CenteredAcross(measured(event.title), panel, kMenuTitleTop)};
	const UiLayout::Placement modes{
			CenteredAcross(measured(event.modes), panel, kMenuTitleTop + title.size.y + kMenuTitleGap)};
	UiLayout::Placement controls{measured(event.controls)};
	controls = CenteredAcross(std::move(controls), panel, panel.h - kMenuControlsBottomGap - controls.size.y);

	const auto selected{static_cast<std::size_t>(event.selectedRow)};
	if (selected < modes.rows.size())
	{
		DrawIcon(UiIcon::MenuSelector,
				 {.x = modes.rows[selected].x - kMenuSelectorGap,
				  .y = modes.rows[selected].y + (kMenuRowHeight - kMenuIconSize) / 2,
				  .w = kMenuIconSize,
				  .h = kMenuIconSize});
	}

	DrawTablePictures(event.title, title);
	DrawTablePictures(event.controls, controls);

	//NOTE: one scale for all the words at once - the pictures are drawn in logical pixels above
	const ScopedRenderScale scaled{_sdlConfig.renderer.get(), scale};
	DrawTableText(event.modes, modes, _menuPointSize, scale);
	DrawTableText(event.controls, controls, _menuPointSize, scale);

	AnnounceMenuTiles(modes);
}

void RenderManager::DrawPanelTables(const RenderPanelTablesEvent& event) const
{
	if (event.tables.empty())
	{
		return;
	}

	const SDL_Rect panel{MenuPanelRect(0)};
	const float scale{CurrentRenderScale()};
	const int pointSize{PanelPointSize(event.tables, panel, scale)};
	const int rowHeight{PanelRowHeight(pointSize)};

	auto placements{MeasureAll(event.tables, rowHeight, CellMeasurer(pointSize, scale))};
	int top{(panel.h - StackHeight(placements, rowHeight)) / 2};
	for (UiLayout::Placement& placement: placements)
	{
		placement = CenteredAcross(std::move(placement), panel, top);
		top += placement.size.y + rowHeight;
	}

	for (const auto [table, placement]: std::views::zip(event.tables, placements))
	{
		DrawTablePictures(table, placement);
	}

	const ScopedRenderScale scaled{_sdlConfig.renderer.get(), scale};
	for (const auto [table, placement]: std::views::zip(event.tables, placements))
	{
		DrawTableText(table, placement, pointSize, scale);
	}
}

void RenderManager::RenderCopyWithClipping(SDL_Texture* texture, const SDL_Rect srcRect, const SDL_Rect dstRect) const
{
	const SDL_FRect src{ToFRect(srcRect)};
	const SDL_FRect dst{ToFRect(dstRect)};
	SDL_RenderTexture(_sdlConfig.renderer.get(), texture, &src, &dst);
}

void RenderManager::RenderCopy(SDL_Texture* texture, const SDL_Rect dstRect) const
{
	const SDL_FRect dst{ToFRect(dstRect)};
	SDL_RenderTexture(_sdlConfig.renderer.get(), texture, nullptr, &dst);
}


int RenderManager::FitStartPointSize() { return SDL_Config::kFontSizePtSmall; }

Point RenderManager::CenteredIn(const SDL_Rect& box, const TextCache::CachedText& cached)
{
	return Point{.x = box.x + (box.w - cached.width) / 2, .y = box.y + (box.h - cached.height) / 2};
}

float RenderManager::CurrentRenderScale() const
{
	//NOTE: SDL_GetRenderScale misses the logical presentation - the letterbox rect maps a logical pixel to the window
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

	const float scale{presentation.w / static_cast<float>(logicalWidth)};

	return scale > 0.f ? scale : 1.f;
}

void RenderManager::DrawTextAt(const Point pos, const SDL_Color color, const std::string_view text,
							   const int basePointSize, const float scale) const
{
	const TextCache::CachedText* cached{_textCache.Acquire(text, color, basePointSize, scale)};
	if (cached == nullptr)
	{
		return;
	}

	DrawText(*cached, pos.x, pos.y, scale);
}

void RenderManager::DrawTextCentered(const SDL_Rect& box, const SDL_Color color, const std::string_view text,
									 const int basePointSize, const float scale) const
{
	const TextCache::CachedText* cached{_textCache.Acquire(text, color, basePointSize, scale)};
	if (cached == nullptr)
	{
		return;
	}

	const Point pos{CenteredIn(box, *cached)};

	DrawText(*cached, pos.x, pos.y, scale);
}

//NOTE: the panel belongs to the field, not to whoever is showing it - all it takes from them is the slide-in
SDL_Rect RenderManager::MenuPanelRect(const int slide) const
{
	return SDL_Rect{.x = static_cast<int>(_menuParams.sideInset),
					.y = static_cast<int>(MenuParams::kTopInset) + slide,
					.w = static_cast<int>(_menuParams.panelSize.x),
					.h = static_cast<int>(_menuParams.panelSize.y)};
}

void RenderManager::DrawText(const TextCache::CachedText& cached, const int x, const int y, const float scale) const
{
	TTF_DrawRendererText(cached.text.get(), static_cast<float>(x) * scale, static_cast<float>(y) * scale);
}

inline SDL_Rect RenderManager::RectToSdlRect(const ObjRectangle& rect)
{
	return SDL_Rect{.x = static_cast<int>(rect.x),
					.y = static_cast<int>(rect.y),
					.w = static_cast<int>(rect.w),
					.h = static_cast<int>(rect.h)};
}

SDL_Rect RenderManager::RectOf(const Point pos, const Point size)
{
	return SDL_Rect{.x = pos.x, .y = pos.y, .w = size.x, .h = size.y};
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
	const SDL_FRect target{ToFRect(rect)};
	SDL_RenderFillRect(_sdlConfig.renderer.get(), &target);
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
	SDL_SetRenderDrawColor(_sdlConfig.renderer.get(), 0u, 0u, 0u, 255u);
	FillRect(SDL_Rect{.x = 0,
					  .y = 0,
					  .w = static_cast<int>(battlefieldSize.x),
					  .h = static_cast<int>(battlefieldSize.y)});
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
	const SDL_Rect dstRect{RectToSdlRect(rect)};
	if (const auto it{_colorTextureCache.find(kGrayColor)}; it != _colorTextureCache.end())
	{
		RenderCopy(it->second.get(), dstRect);
	}
}

void RenderManager::DrawTexture(const RenderTextureEvent& event) const
{
	auto [angle, flip] = GetRotateAndAngleAndFlip(event.dir);
	const SDL_FRect src{ToFRect(RectToSdlRect(event.textureRect))};
	const SDL_FRect dst{ToFRect(RectToSdlRect(event.destRect))};
	SDL_Texture* atlas{_sdlConfig.atlasTexture.get()};

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
	const unsigned int fps{event.fps};
	if (fps == 0u)
	{
		return;
	}

	constexpr SDL_Color textColor{.r = 140u, .g = 0u, .b = 255u, .a = 255u};
	const float scale{CurrentRenderScale()};
	const ScopedRenderScale scaled{_sdlConfig.renderer.get(), scale};
	//NOTE: three digits are 72 px in a 71 px column - the pixel over buys reusing the one font opened at startup
	DrawTextCentered(_fpsBox, textColor, std::to_string(fps), SDL_Config::kFontSizePtMedium, scale);
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
	FillRect(healthBarRect);
}

void RenderManager::InitMenu(const GameConfig& gameConfig)
{
	_menuParams.Init(gameConfig.LogicalSize(), gameConfig.sideBarWidth);
	_menuScale = {};

	CreateColorTexture(kGrayColor);
}

SDL_Rect RenderManager::CalcFpsBox(const UPoint& battlefieldSize)
{
	return SDL_Rect{.x = static_cast<int>(battlefieldSize.x) + kSideBarColumnPadding,
					.y = 0,
					.w = kSideBarItemWidth,
					.h = kFpsBoxHeight};
}

//NOTE: CalcFpsBox stays static - it runs before the geometry event; everything else asks here
int RenderManager::SideBarColumnX() const
{
	return static_cast<int>(_gameConfig.battlefieldSize.x) + kSideBarColumnPadding;
}
