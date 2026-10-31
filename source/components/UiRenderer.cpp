#include "components/UiRenderer.h"
#include "components/SideBarCanvas.h"
#include "components/SideBarPlayerPainter.h"
#include "components/UiLayout.h"
#include "components/UiTable.h"
#include "components/WorldGeometry.h"
#include "enums/UiIcon.h"
#include "utils/MathUtils.h"
#include "utils/TextUtils.h"
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
#include <optional>
#include <ranges>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
//NOTE: fitting only ever shrinks - every panel screen starts from the same size
constexpr int kFitStartPointSize{SDL_Config::kFontSizePt};
//NOTE: kept free on every side of the panel
constexpr int kPanelSideMargin{20};

//NOTE: tighter than the controls' rows, which hold pad pictures
constexpr int kMenuModeRowHeight{24};
constexpr Point kMenuIconSize{.x = 30, .y = 30};
constexpr int kMenuRowPadding{5};

int PanelRowHeight(const int pointSize)
{
	//NOTE: at the full point size - rows and the gaps between tables shrink with the font
	constexpr int fullHeight{20};

	return pointSize * fullHeight / kFitStartPointSize;
}

int ScrollBarWidth(const int rowHeight)
{
	constexpr int rowShare{4};

	return std::max(rowHeight / rowShare, 2);
}

void FillColored(SDL_Renderer* const renderer, const SDL_Color& color, const SDL_Rect& rect)
{
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SdlRenderUtils::FillRect(renderer, rect);
}

//NOTE: drops the render scale around a run of glyphs sized in output pixels; each scale change breaks the batch
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
	, _sideBarCanvas{sdlConfig, gameConfig}
	, _enemyPainter{_sideBarCanvas}
	, _playerPainter{_sideBarCanvas}
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
	constexpr int minPointSize{8};
	const Point size{.x = panel.w, .y = panel.h};
	if (std::ranges::equal(tables, words) && panelSize == size
		&& MathUtils::AreEqualAbsolute(static_cast<double>(scale), static_cast<double>(renderScale)))
	{
		return pointSize;
	}

	*this = PanelFit{.tables = words | std::ranges::to<std::vector>(),
					 .scale = renderScale,
					 .panelSize = size,
					 .pointSize = UiLayout::FitPointSize(minPointSize, kFitStartPointSize, goesIn)};

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
	constexpr int titleTop{17};
	//NOTE: the modes start this far under the logo
	constexpr int titleGap{28};
	//NOTE: the controls are placed from the panel's bottom, so the room under them never changes
	constexpr int controlsBottomGap{10};
	constexpr int menuRowHeight{30};
	//NOTE: room for the arrow left of the mode it points at, and the slack a click still lands in
	constexpr int selectorGap{35};

	const SDL_Rect panel{MenuPanelRect(event.slide)};
	const float scale{CurrentRenderScale()};
	const int width{panel.w - kPanelSideMargin * 2};
	const auto goesIn = [this, &event, width, scale](const int size)
	{
		const std::array tables{event.modes, event.controls};

		//NOTE: one size for both tables - it has to fit the modes' tighter row
		return UiLayout::FitsAcross(tables, UiLayout::MeasureAll(tables, kMenuModeRowHeight, CellMeasurer(size, scale)),
									width, kMenuModeRowHeight);
	};
	//NOTE: the menu's words never change - only another scale or panel refits them
	const int pointSize{_menuFit.PointSize({}, panel, scale, goesIn)};

	const UiLayout::Measure measure{CellMeasurer(pointSize, scale)};
	const auto measured = [&measure](const UiTable& table, const int rowHeight)
	{
		return UiLayout::Place(table, Point{}, rowHeight, measure);
	};
	const auto across = [&panel](UiLayout::Placement placement, const int top)
	{
		return UiLayout::CenteredAcross(std::move(placement), Point{.x = panel.x, .y = panel.y + top}, panel.w);
	};
	const UiLayout::Placement title{across(measured(event.title, menuRowHeight), titleTop)};
	const UiLayout::Placement modes{across(measured(event.modes, kMenuModeRowHeight),
										   titleTop + title.size.y + titleGap)};
	UiLayout::Placement controls{measured(event.controls, menuRowHeight)};
	const int controlsTop{panel.h - controlsBottomGap - controls.size.y};
	controls = across(std::move(controls), controlsTop);

	const auto selected{static_cast<std::size_t>(event.selectedRow)};
	if (selected < modes.rows.size())
	{
		DrawIcon(UiIcon::MenuSelector,
				 {.x = modes.rows[selected].x - selectorGap,
				  .y = modes.rows[selected].y + (kMenuModeRowHeight - kMenuIconSize.y) / 2,
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
	//NOTE: a pick past the tables is no pick
	const bool isPickInTables{event.pick && event.pick->table < event.tables.size()};
	const std::optional<PanelPick> pick{isPickInTables ? event.pick : std::nullopt};
	//NOTE: the selector left of the picked table and the scroll bar right of it are centered with it as one block
	const bool hasScrollBar{pick && pick->scroll.has_value()};
	const auto flanks = [hasScrollBar](const int rowHeight)
	{
		return std::pair{rowHeight + kMenuRowPadding, hasScrollBar ? kMenuRowPadding + ScrollBarWidth(rowHeight) : 0};
	};
	//NOTE: as large as the stack lets it be - the plate does not shrink, so the words make the room it needs
	const auto goesIn = [this, &event, &panel, width, scale, &pick, &flanks](const int size)
	{
		const int rowHeight{PanelRowHeight(size)};
		const auto placed{UiLayout::MeasureAll(event.tables, rowHeight, CellMeasurer(size, scale))};
		const auto [left, right]{flanks(rowHeight)};
		const bool isPickedIn{!pick || placed[pick->table].size.x + left + right <= width};

		return isPickedIn && UiLayout::FitsAcross(event.tables, placed, width, rowHeight)
			   && UiLayout::StackHeight(placed, rowHeight) <= panel.h - kPanelSideMargin * 2;
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

	if (pick)
	{
		UiLayout::Placement& picked{placements[pick->table]};
		const auto [left, right]{flanks(rowHeight)};
		picked.ShiftBy(Point{.x = (left - right) / 2});

		//NOTE: the arrow is a row tall here - the menu's own is sized for the menu's taller rows
		if (pick->selectedRow < picked.rows.size())
		{
			const Point row{picked.rows[pick->selectedRow]};
			DrawIcon(UiIcon::MenuSelector,
					 {.x = row.x - rowHeight - kMenuRowPadding, .y = row.y, .w = rowHeight, .h = rowHeight});
		}

		if (pick->scroll)
		{
			DrawScrollBar(picked, *pick->scroll, rowHeight);
		}

		if (!pick->dropDown)
		{
			AnnouncePanelRows(picked.rows, Point{.x = picked.size.x, .y = rowHeight},
							  _textCache.MeasureString("0", pointSize, scale).x);
		}
	}

	DrawTables(std::views::zip(event.tables, placements), pointSize, scale);

	if (pick && pick->caret)
	{
		DrawCaret(event.tables[pick->table], placements[pick->table], *pick->caret, pointSize, scale);
	}

	if (pick && pick->dropDown)
	{
		DrawDropDown(placements[pick->table], *pick->dropDown, pointSize, scale);
	}
}

void UiRenderer::DrawCaret(const UiTable& table, const UiLayout::Placement& placement, const PanelCaret& caret,
						   const int pointSize, const float scale) const
{
	const auto isCaretCell = [&caret](const UiLayout::PlacedCell& cell)
	{
		return cell.row == caret.row && cell.column == 0;
	};
	const auto placed{std::ranges::find_if(placement.cells, isCaretCell)};
	if (placed == placement.cells.end())
	{
		return;
	}

	const UiCell& cell{table.rows[caret.row].cells.front()};
	const std::string_view text{cell.text};
	const int height{_textCache.MeasureString(text, pointSize, scale).y};
	const std::size_t start{TextUtils::ByteOf(text, caret.symbol)};
	const int before{_textCache.MeasureString(text.substr(0, start), pointSize, scale).x};
	const int top{placed->pos.y + (placed->size.y - height) / 2};
	//NOTE: one dot of the pixel font - a glyph is eight dots tall
	constexpr int dotsTall{8};
	const int thickness{std::max(height / dotsTall, 1)};
	const std::string_view under{text.substr(start, TextUtils::ByteOf(text, caret.symbol + caret.symbols) - start)};
	const SDL_Rect bar{caret.symbols == 0
						   ? SDL_Rect{.x = placed->pos.x + before, .y = top, .w = thickness, .h = height}
						   : SDL_Rect{.x = placed->pos.x + before,
									  .y = top + height - thickness,
									  .w = _textCache.MeasureString(under, pointSize, scale).x,
									  .h = thickness}};

	SDL_Renderer* const renderer{_sdlConfig.renderer.get()};
	SDL_SetRenderDrawColor(renderer, 0xffu, 0xffu, 0xffu, caret.alpha);
	SdlRenderUtils::FillRect(renderer, bar);
}

//NOTE: right of the table along the window's rows; the thumb is the window's share and place in the list
void UiRenderer::DrawScrollBar(const UiLayout::Placement& picked, const PanelScroll& scroll, const int rowHeight) const
{
	if (scroll.total <= scroll.shownCount || scroll.rowCount == 0
		|| scroll.firstRow + scroll.rowCount > picked.rows.size())
	{
		return;
	}

	const Point top{picked.rows[scroll.firstRow]};
	const int height{picked.rows[scroll.firstRow + scroll.rowCount - 1].y + rowHeight - top.y};
	const int total{static_cast<int>(scroll.total)};
	const SDL_Rect track{.x = top.x + picked.size.x + kMenuRowPadding,
						 .y = top.y,
						 .w = ScrollBarWidth(rowHeight),
						 .h = height};
	const SDL_Rect thumb{.x = track.x,
						 .y = track.y + height * static_cast<int>(scroll.firstShown) / total,
						 .w = track.w,
						 .h = height * static_cast<int>(scroll.shownCount) / total};

	constexpr SDL_Color trackColor{.r = 0xffu, .g = 0xffu, .b = 0xffu, .a = 0x40u};
	constexpr SDL_Color thumbColor{.r = 0xffu, .g = 0xffu, .b = 0xffu, .a = 0xffu};
	FillColored(_sdlConfig.renderer.get(), trackColor, track);
	FillColored(_sdlConfig.renderer.get(), thumbColor, thumb);
}

//NOTE: as wide as its table, the picked item on a lighter bar
void UiRenderer::DrawDropDown(const UiLayout::Placement& picked, const PanelDropDown& dropDown, const int pointSize,
							  const float scale) const
{
	if (dropDown.row >= picked.rows.size())
	{
		return;
	}

	const int rowHeight{PanelRowHeight(pointSize)};
	const Point anchor{picked.rows[dropDown.row]};
	const UiLayout::Placement items{UiLayout::Place(dropDown.items, Point{.x = anchor.x, .y = anchor.y + rowHeight},
													rowHeight, CellMeasurer(pointSize, scale))};
	const SDL_Rect plate{.x = anchor.x - kMenuRowPadding,
						 .y = anchor.y + rowHeight,
						 .w = picked.size.x + kMenuRowPadding * 2,
						 .h = items.size.y};

	//NOTE: nearly opaque - the list hides the rows under it
	constexpr SDL_Color plateColor{.r = 0x10u, .g = 0x10u, .b = 0x10u, .a = 0xf0u};
	constexpr SDL_Color pickColor{.r = 0xffu, .g = 0xffu, .b = 0xffu, .a = 0x40u};
	FillColored(_sdlConfig.renderer.get(), plateColor, plate);
	if (dropDown.picked < items.rows.size())
	{
		FillColored(_sdlConfig.renderer.get(), pickColor,
					SDL_Rect{.x = plate.x, .y = items.rows[dropDown.picked].y, .w = plate.w, .h = rowHeight});
	}

	DrawTables(std::array<PlacedTable, 1>{{{dropDown.items, items}}}, pointSize, scale);

	std::vector<Point> rows{items.rows};
	rows.push_back(anchor);
	AnnouncePanelRows(rows, Point{.x = picked.size.x, .y = rowHeight},
					  _textCache.MeasureString("0", pointSize, scale).x);
}

//NOTE: the painters draw pictures and place words; the words are written here
void UiRenderer::DrawSideBar(const RenderSideBarEvent& event) const
{
	_enemyPainter.Paint(event.enemies);

	const float scale{CurrentRenderScale()};
	const SideBarWords words{_playerPainter.Paint(event.players, CellMeasurer(SideBarCanvas::kPointSize, scale),
												  CellMeasurer(SideBarCanvas::kLabelPointSize, scale))};
	DrawTables(std::views::zip(words.tables, words.places), SideBarCanvas::kPointSize, scale);
	DrawTables(std::views::zip(words.labels, words.labelPlaces), SideBarCanvas::kLabelPointSize, scale);
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
	const SDL_Rect box{_sideBarCanvas.FpsBox()};
	//NOTE: one row as tall as the box centers the digits vertically
	const UiLayout::Placement placement{UiLayout::CenteredAcross(
			UiLayout::Place(fps, Point{}, box.h, CellMeasurer(SideBarCanvas::kPointSize, scale)),
			Point{.x = box.x, .y = box.y}, box.w)};

	const ScopedRenderScale scaled{_sdlConfig.renderer.get(), scale};
	DrawTableText(fps, placement, SideBarCanvas::kPointSize, scale);
}

void UiRenderer::DrawPlate(const RenderPlateEvent& event) const
{
	const auto [width, height]{Icon(event.plate).size};
	//NOTE: plate centers as a share of the field height - the match-end plate stands above the pause one
	constexpr double pauseMiddle{0.5};
	constexpr double matchEndMiddle{0.25};
	const double middle{event.plate == UiIcon::PlatePause ? pauseMiddle : matchEndMiddle};
	DrawIcon(event.plate, CenteredInField(width, height, middle));
}

UiRenderer::IconSource UiRenderer::Icon(const UiIcon icon) const
{
	constexpr Point logoSize{.x = 300, .y = 75};
	//NOTE: plate widths as a share of the field width
	constexpr double pauseShare{0.40};
	constexpr double gameOverShare{0.27};
	constexpr double gameWonShare{0.16};
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
			return IconSource{.texture = _sdlConfig.logoTexture.get(), .size = logoSize};
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
		case UiIcon::PlatePause:
			return sprite(TextureOffset::kPauseText, PlateSize(TextureOffset::kPauseText, pauseShare));
		case UiIcon::PlateGameOver:
			return sprite(TextureOffset::kGameOverText, PlateSize(TextureOffset::kGameOverText, gameOverShare));
		case UiIcon::PlateGameWon:
			return sprite(TextureOffset::kGameWonText, PlateSize(TextureOffset::kGameWonText, gameWonShare));
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

		if (cell.symbols > 0)
		{
			return _textCache.MeasureString(std::string(cell.symbols, '0'), pointSize, scale);
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
														 .y = kMenuModeRowHeight}});
}

void UiRenderer::AnnouncePanelRows(const std::vector<Point>& rows, const Point rowSize, const int symbolWidth) const
{
	if (rows == _panelRowPlaces && rowSize == _panelRowSize && symbolWidth == _panelSymbolWidth)
	{
		return;
	}

	_panelRowPlaces = rows;
	_panelRowSize = rowSize;
	_panelSymbolWidth = symbolWidth;
	_events->EmitEvent(PanelRowsPlacedEvent{.rows = rows, .rowSize = rowSize, .symbolWidth = symbolWidth});
}

//NOTE: the panel belongs to the field, not to whoever is showing it - all it takes from them is the slide-in
SDL_Rect UiRenderer::MenuPanelRect(const int slide) const
{
	//NOTE: whole cells, so the panel's sides land on the outer brick blocks - must match the .map files' border
	constexpr int sideInset{static_cast<int>(WorldGeometry::kCellSize) * 5};
	constexpr int topInset{50};
	const UPoint window{_gameConfig.LogicalSize()};
	const int width{static_cast<int>(window.x) - static_cast<int>(_gameConfig.sideBarWidth)};

	//NOTE: as far off the bottom edge as off the sides - the panel reads as one frame
	return SDL_Rect{.x = sideInset,
					.y = topInset + slide,
					.w = width - sideInset * 2,
					.h = static_cast<int>(window.y) - topInset - sideInset};
}

float UiRenderer::CurrentRenderScale() const { return SdlRenderUtils::LogicalScale(_sdlConfig.renderer.get()).x; }

SDL_Rect UiRenderer::CenteredInField(const int width, const int height, const double middleShare) const
{
	const UPoint field{_gameConfig.battlefieldSize};

	return SDL_Rect{.x = (static_cast<int>(field.x) - width) / 2,
					.y = static_cast<int>(static_cast<double>(field.y) * middleShare) - height / 2,
					.w = width,
					.h = height};
}

Point UiRenderer::PlateSize(const ObjRectangle& sprite, const double widthShare) const
{
	const auto width{static_cast<int>(static_cast<double>(_gameConfig.battlefieldSize.x) * widthShare)};

	return Point{.x = width, .y = static_cast<int>(static_cast<double>(width) * sprite.h / sprite.w)};
}
