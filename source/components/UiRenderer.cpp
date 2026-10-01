#include "components/UiRenderer.h"
#include "components/UiLayout.h"
#include "components/UiTable.h"
#include "components/WorldGeometry.h"
#include "enums/UiIcon.h"
#include "utils/MathUtils.h"
#include "utils/SdlRenderUtils.h"
#include "geometry/Point.h"
#include "application/GameConfig.h"
#include "application/SDL_Config.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/RenderUIEvents.h"
#include "enums/TextureOffset.h"
#include "geometry/ObjRectangle.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <ranges>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{
//NOTE: fitting only ever shrinks - every panel screen starts from the same size
constexpr int kFitStartPointSize{SDL_Config::kFontSizePtSmall};
constexpr int kFitMinPointSize{8};
//NOTE: the scoreboard and the lobby at the full point size - a row, and the gap between tables, shrink with it
constexpr int kPanelRowHeight{20};
//NOTE: the widest table still leaves this much of the panel on each side
constexpr int kPanelSideMargin{20};
//NOTE: whole cells, so the panel's sides land on the outermost brick blocks - the number has to
//match the border the .map files leave around the pattern
constexpr int kPanelSideInset{static_cast<int>(WorldGeometry::kCellSize) * 5};
constexpr int kPanelTopInset{50};

//NOTE: the menu's own places inside the panel
constexpr int kMenuTitleTop{17};
constexpr Point kMenuLogoSize{.x = 300, .y = 75};
//NOTE: the modes start this far under the logo
constexpr int kMenuTitleGap{28};
//NOTE: the controls hang off the bottom of the panel, so the room under them stays the same
constexpr int kMenuControlsBottomGap{25};
constexpr int kMenuRowHeight{30};
constexpr Point kMenuIconSize{.x = 30, .y = 30};
//NOTE: room for the arrow to the left of the mode it points at, and the slack a click still lands in
constexpr int kMenuSelectorGap{35};
constexpr int kMenuRowPadding{5};

//NOTE: one column - fps box, enemy grid, counters and the flag share x and width
constexpr int kSideBarColumnPadding{55};
constexpr int kSideBarItemWidth{71};
constexpr int kFpsBoxHeight{60};
constexpr int kReserveFrameHeight{277};
constexpr int kReserveGapBelow{13};
//NOTE: 4, not 5 - the 25 px tank is centered in its 27 px row
constexpr Point kReserveInset{.x = 5, .y = 4};
constexpr int kReserveRowHeight{27};
constexpr int kReserveColumnGap{1};
constexpr Point kSideBarEnemyTankSize{.x = 30, .y = 25};
constexpr Point kSideBarLivesSize{.x = kSideBarItemWidth, .y = 70};
constexpr Point kSideBarStageSize{.x = kSideBarItemWidth, .y = 95};

constexpr double kPausePlateShare{0.40};
constexpr double kGameOverPlateShare{0.27};
constexpr double kGameWonPlateShare{0.16};

int PanelRowHeight(const int pointSize) { return pointSize * kPanelRowHeight / kFitStartPointSize; }

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
}//namespace

UiRenderer::UiRenderer(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig,
					   const SDL_Config& sdlConfig)
	: _events{events}
	, _gameConfig{gameConfig}
	, _sdlConfig{sdlConfig}
	, _textCache{sdlConfig}
{
	Subscribe();
}

void UiRenderer::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &UiRenderer::DrawMenuBackground));
	_subs.push_back(_events->AddListener(this, &UiRenderer::DrawMenu));
	_subs.push_back(_events->AddListener(this, &UiRenderer::DrawPanelTables));
	_subs.push_back(_events->AddListener(this, &UiRenderer::DrawSideBar));
	_subs.push_back(_events->AddListener(this, &UiRenderer::RenderFPS));
	_subs.push_back(_events->AddListener(this, &UiRenderer::DrawPlate));

	_subs.push_back(_events->AddListener(this, &UiRenderer::OnRenderDeviceReset));
}

//NOTE: text comes back from the font
void UiRenderer::OnRenderDeviceReset(const RenderDeviceResetEvent&) { _textCache.Clear(); }

int UiRenderer::PanelFit::PointSize(const std::span<const UiTable> words, const SDL_Rect& panel,
									const float renderScale, const std::function<bool(int)>& goesIn)
{
	const Point size{.x = panel.w, .y = panel.h};
	if (std::ranges::equal(tables, words) && panelSize == size
		&& MathUtils::AreEqualAbsolute(static_cast<double>(scale), static_cast<double>(renderScale)))
	{
		return pointSize;
	}

	*this = PanelFit{.tables = words | std::ranges::to<std::vector>(),
					 .scale = renderScale,
					 .panelSize = size,
					 .pointSize = UiLayout::FitPointSize(kFitMinPointSize, kFitStartPointSize, goesIn)};

	return pointSize;
}

void UiRenderer::DrawTables(const auto& placedTables, const int pointSize, const float scale) const
{
	for (auto&& [table, placement]: placedTables)
	{
		DrawTablePictures(table, placement);
	}

	const ScopedRenderScale scaled{_sdlConfig.renderer.get(), scale};
	for (auto&& [table, placement]: placedTables)
	{
		DrawTableText(table, placement, pointSize, scale);
	}
}

void UiRenderer::DrawMenuBackground(const RenderMenuBackgroundEvent& event) const
{
	constexpr SDL_Color color{.r = 0x80u, .g = 0x80u, .b = 0x80u, .a = 0x91u};
	SDL_SetRenderDrawColor(_sdlConfig.renderer.get(), color.r, color.g, color.b, color.a);
	SdlRenderUtils::FillRect(_sdlConfig.renderer.get(), MenuPanelRect(event.slide));
}

void UiRenderer::DrawMenu(const RenderMenuEvent& event) const
{
	const SDL_Rect panel{MenuPanelRect(event.slide)};
	const float scale{CurrentRenderScale()};
	const int width{panel.w - kPanelSideMargin * 2};
	const auto goesIn = [this, &event, width, scale](const int size)
	{
		const std::array tables{event.modes, event.controls};

		return UiLayout::FitsAcross(tables, UiLayout::MeasureAll(tables, kMenuRowHeight, CellMeasurer(size, scale)),
									width, kMenuRowHeight);
	};
	//NOTE: the menu's words never change - only another scale or panel refits them
	const int pointSize{_menuFit.PointSize({}, panel, scale, goesIn)};

	const UiLayout::Measure measure{CellMeasurer(pointSize, scale)};
	const auto measured = [&measure](const UiTable& table)
	{
		return UiLayout::Place(table, Point{}, kMenuRowHeight, measure);
	};
	const auto across = [&panel](UiLayout::Placement placement, const int top)
	{
		return UiLayout::CenteredAcross(std::move(placement), Point{.x = panel.x, .y = panel.y + top}, panel.w);
	};
	const UiLayout::Placement title{across(measured(event.title), kMenuTitleTop)};
	const UiLayout::Placement modes{across(measured(event.modes), kMenuTitleTop + title.size.y + kMenuTitleGap)};
	UiLayout::Placement controls{measured(event.controls)};
	controls = across(std::move(controls), panel.h - kMenuControlsBottomGap - controls.size.y);

	const auto selected{static_cast<std::size_t>(event.selectedRow)};
	if (selected < modes.rows.size())
	{
		DrawIcon(UiIcon::MenuSelector,
				 {.x = modes.rows[selected].x - kMenuSelectorGap,
				  .y = modes.rows[selected].y + (kMenuRowHeight - kMenuIconSize.y) / 2,
				  .w = kMenuIconSize.x,
				  .h = kMenuIconSize.y});
	}

	DrawTables(std::array<PlacedTable, 3>{{{event.title, title}, {event.modes, modes}, {event.controls, controls}}},
			   pointSize, scale);

	AnnounceMenuTiles(modes);
}

void UiRenderer::DrawPanelTables(const RenderPanelTablesEvent& event) const
{
	if (event.tables.empty())
	{
		return;
	}

	const SDL_Rect panel{MenuPanelRect(0)};
	const float scale{CurrentRenderScale()};
	const int width{panel.w - kPanelSideMargin * 2};
	//NOTE: as large as the stack lets it be - the plate does not shrink, so the words make the room it needs
	const auto goesIn = [this, &event, &panel, width, scale](const int size)
	{
		const int rowHeight{PanelRowHeight(size)};
		const auto placed{UiLayout::MeasureAll(event.tables, rowHeight, CellMeasurer(size, scale))};

		return UiLayout::FitsAcross(event.tables, placed, width, rowHeight)
			   && UiLayout::StackHeight(placed, rowHeight) <= panel.h;
	};
	const int pointSize{_panelFit.PointSize(event.tables, panel, scale, goesIn)};
	const int rowHeight{PanelRowHeight(pointSize)};

	auto placements{UiLayout::MeasureAll(event.tables, rowHeight, CellMeasurer(pointSize, scale))};
	int top{panel.y + (panel.h - UiLayout::StackHeight(placements, rowHeight)) / 2};
	for (UiLayout::Placement& placement: placements)
	{
		placement = UiLayout::CenteredAcross(std::move(placement), Point{.x = panel.x, .y = top}, panel.w);
		top += placement.size.y + rowHeight;
	}

	DrawTables(std::views::zip(event.tables, placements), pointSize, scale);
}

void UiRenderer::DrawSideBar(const RenderSideBarEvent& event) const
{
	const float scale{CurrentRenderScale()};
	const UiLayout::Measure measure{CellMeasurer(SDL_Config::kFontSizePtMedium, scale)};

	//NOTE: the fps box tops the column, the reserve hangs right under it
	const SDL_Rect frame{.x = SideBarColumnX(), .y = kFpsBoxHeight, .w = kSideBarItemWidth, .h = kReserveFrameHeight};
	SdlRenderUtils::RenderCopyWithClipping(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(),
										   SdlRenderUtils::RectToSdlRect(TextureOffset::kEnemyIconBackground), frame);

	const Point reserveAt{.x = frame.x + kReserveInset.x, .y = frame.y + kReserveInset.y};
	const UiLayout::Placement enemies{
			UiLayout::Place(event.enemies, reserveAt, kReserveRowHeight, measure, kReserveColumnGap)};
	const Point countersAt{.x = frame.x, .y = frame.y + frame.h + kReserveGapBelow};
	const UiLayout::Placement counters{UiLayout::Place(event.counters, countersAt, 0, measure)};

	DrawTables(std::array<PlacedTable, 2>{{{event.enemies, enemies}, {event.counters, counters}}},
			   SDL_Config::kFontSizePtMedium, scale);
}

void UiRenderer::RenderFPS(const RenderFPSEvent& event) const
{
	if (event.fps == 0u)
	{
		return;
	}

	constexpr unsigned int textColor{0xFF8C00FFu};
	const UiTable fps{.rows = {UiRow{.cells = {TextCell(std::to_string(event.fps), textColor)}}}};
	const float scale{CurrentRenderScale()};
	//NOTE: a row as tall as the box centers the digits in it; three digits are 72 px in a 71 px column -
	//the pixel over buys reusing the one font opened at startup
	const UiLayout::Placement placement{UiLayout::CenteredAcross(
			UiLayout::Place(fps, Point{}, kFpsBoxHeight, CellMeasurer(SDL_Config::kFontSizePtMedium, scale)),
			Point{.x = SideBarColumnX(), .y = 0}, kSideBarItemWidth)};

	const ScopedRenderScale scaled{_sdlConfig.renderer.get(), scale};
	DrawTableText(fps, placement, SDL_Config::kFontSizePtMedium, scale);
}

void UiRenderer::DrawPlate(const RenderPlateEvent& event) const
{
	const auto [width, height]{Icon(event.plate).size};
	DrawIcon(event.plate, CenteredInField(width, height));
}

UiRenderer::IconSource UiRenderer::Icon(const UiIcon icon) const
{
	const auto sprite = [this](const ObjRectangle& offset, const Point size)
	{
		return IconSource{.texture = _sdlConfig.atlasTexture.get(),
						  .sprite = SdlRenderUtils::RectToSdlRect(offset),
						  .size = size};
	};

	switch (icon)
	{
		case UiIcon::None:
			break;
		case UiIcon::MenuLogo:
			return IconSource{.texture = _sdlConfig.logoTexture.get(), .size = kMenuLogoSize};
		case UiIcon::MenuSelector:
			return IconSource{.texture = _sdlConfig.selectorIconTexture.get(), .size = kMenuIconSize};
		case UiIcon::MenuXBoxHome:
			return IconSource{.texture = _sdlConfig.xboxTextures[1].get(), .size = kMenuIconSize};
		case UiIcon::MenuXBoxView:
			return IconSource{.texture = _sdlConfig.xboxTextures[3].get(), .size = kMenuIconSize};
		case UiIcon::MenuXBoxMenu:
			return IconSource{.texture = _sdlConfig.xboxTextures[2].get(), .size = kMenuIconSize};
		case UiIcon::MenuXBoxY:
			return IconSource{.texture = _sdlConfig.xboxTextures[5].get(), .size = kMenuIconSize};
		case UiIcon::MenuXBoxDpad:
			return IconSource{.texture = _sdlConfig.xboxTextures[0].get(), .size = kMenuIconSize};
		case UiIcon::MenuXBoxA:
			return IconSource{.texture = _sdlConfig.xboxTextures[4].get(), .size = kMenuIconSize};
		case UiIcon::MenuPS5Home:
			return IconSource{.texture = _sdlConfig.ps5Textures[3].get(), .size = kMenuIconSize};
		case UiIcon::MenuPS5Create:
			return IconSource{.texture = _sdlConfig.ps5Textures[0].get(), .size = kMenuIconSize};
		case UiIcon::MenuPS5Options:
			return IconSource{.texture = _sdlConfig.ps5Textures[4].get(), .size = kMenuIconSize};
		case UiIcon::MenuPS5Triangle:
			return IconSource{.texture = _sdlConfig.ps5Textures[5].get(), .size = kMenuIconSize};
		case UiIcon::MenuPS5Dpad:
			return IconSource{.texture = _sdlConfig.ps5Textures[2].get(), .size = kMenuIconSize};
		case UiIcon::MenuPS5Cross:
			return IconSource{.texture = _sdlConfig.ps5Textures[1].get(), .size = kMenuIconSize};
		case UiIcon::SideBarEnemyTank:
			return sprite(TextureOffset::kEnemyIcon, kSideBarEnemyTankSize);
		case UiIcon::SideBarPlayerOne:
			return sprite(TextureOffset::kPlayer1Icon, kSideBarLivesSize);
		case UiIcon::SideBarPlayerTwo:
			return sprite(TextureOffset::kPlayer2Icon, kSideBarLivesSize);
		case UiIcon::SideBarStageFlag:
			return sprite(TextureOffset::kStageNumberFlag, kSideBarStageSize);
		case UiIcon::PlatePause:
			return sprite(TextureOffset::kPauseText, PlateSize(TextureOffset::kPauseText, kPausePlateShare));
		case UiIcon::PlateGameOver:
			return sprite(TextureOffset::kGameOverText, PlateSize(TextureOffset::kGameOverText, kGameOverPlateShare));
		case UiIcon::PlateGameWon:
			return sprite(TextureOffset::kGameWonText, PlateSize(TextureOffset::kGameWonText, kGameWonPlateShare));
	}

	return IconSource{};
}

void UiRenderer::DrawIcon(const UiIcon icon, const SDL_Rect dstRect) const
{
	const IconSource source{Icon(icon)};
	if (source.sprite)
	{
		SdlRenderUtils::RenderCopyWithClipping(_sdlConfig.renderer.get(), source.texture, *source.sprite, dstRect);
		return;
	}

	SdlRenderUtils::RenderCopy(_sdlConfig.renderer.get(), source.texture, dstRect);
}

UiLayout::Measure UiRenderer::CellMeasurer(const int pointSize, const float scale) const
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

void UiRenderer::DrawTablePictures(const UiTable& table, const UiLayout::Placement& placement) const
{
	for (const UiLayout::PlacedCell& placed: placement.cells)
	{
		const UiCell& cell{table.rows[placed.row].cells[placed.column]};
		if (cell.background != UiIcon::None)
		{
			DrawIcon(cell.background, SdlRenderUtils::RectOf(placed.boxPos, placed.boxSize));
		}

		if (cell.icon != UiIcon::None)
		{
			DrawIcon(cell.icon, SdlRenderUtils::RectOf(placed.pos, placed.size));
		}
	}
}

void UiRenderer::DrawTableText(const UiTable& table, const UiLayout::Placement& placement, const int pointSize,
							   const float scale) const
{
	for (const UiLayout::PlacedCell& placed: placement.cells)
	{
		const UiCell& cell{table.rows[placed.row].cells[placed.column]};
		if (cell.icon != UiIcon::None || cell.text.empty())
		{
			continue;
		}

		const TextCache::CachedText* cached{
				_textCache.Acquire(cell.text, SdlRenderUtils::IntToColor(cell.color), pointSize, scale)};
		if (cached == nullptr)
		{
			continue;
		}

		TTF_DrawRendererText(cached->text.get(), static_cast<float>(placed.pos.x) * scale,
							 static_cast<float>(placed.pos.y) * scale);
	}
}

//NOTE: sent on, not kept - only this side knows where the rows ended up once the size was fitted
void UiRenderer::AnnounceMenuTiles(const UiLayout::Placement& modes) const
{
	const auto tileAt = [](const Point& row) { return Point{.x = row.x - kMenuRowPadding, .y = row.y}; };
	auto tiles{modes.rows | std::views::transform(tileAt) | std::ranges::to<std::vector>()};
	if (tiles == _menuTilePlaces)
	{
		return;
	}

	_menuTilePlaces = tiles;
	_events->EmitEvent(MenuTilesPlacedEvent{.tiles = std::move(tiles),
										 .tileSize = {.x = modes.size.x + kMenuRowPadding * 2,
													 .y = kMenuRowHeight}});
}

//NOTE: the panel belongs to the field, not to whoever is showing it - all it takes from them is the slide-in
SDL_Rect UiRenderer::MenuPanelRect(const int slide) const
{
	const UPoint window{_gameConfig.LogicalSize()};
	const int width{static_cast<int>(window.x) - static_cast<int>(_gameConfig.sideBarWidth)};

	//NOTE: as far off the bottom edge as off the sides - the panel reads as one frame
	return SDL_Rect{.x = kPanelSideInset,
					.y = kPanelTopInset + slide,
					.w = width - kPanelSideInset * 2,
					.h = static_cast<int>(window.y) - kPanelTopInset - kPanelSideInset};
}

float UiRenderer::CurrentRenderScale() const
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

SDL_Rect UiRenderer::CenteredInField(const int width, const int height) const
{
	const UPoint field{_gameConfig.battlefieldSize};

	return SDL_Rect{.x = (static_cast<int>(field.x) - width) / 2,
					.y = (static_cast<int>(field.y) - height) / 2,
					.w = width,
					.h = height};
}

Point UiRenderer::PlateSize(const ObjRectangle& sprite, const double widthShare) const
{
	const auto width{static_cast<int>(static_cast<double>(_gameConfig.battlefieldSize.x) * widthShare)};

	return Point{.x = width, .y = static_cast<int>(static_cast<double>(width) * sprite.h / sprite.w)};
}

int UiRenderer::SideBarColumnX() const
{
	return static_cast<int>(_gameConfig.battlefieldSize.x) + kSideBarColumnPadding;
}
