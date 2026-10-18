#include "components/UiRenderer.h"
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
#include "enums/BonusType.h"
#include "enums/TextureOffset.h"
#include "geometry/ObjRectangle.h"
#include <algorithm>
#include <array>
#include <cmath>
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

//NOTE: side bar places are atlas coordinates off the reserve frame's corner; one scale keeps the original gaps
constexpr double kAtlasCell{8.0};
constexpr double kSideBarScale{2.75};
//NOTE: a digit fills one atlas cell - a glyph's eight dots, an atlas pixel each
constexpr int kSideBarPointSize{static_cast<int>(kAtlasCell * kSideBarScale)};
constexpr int kFpsBoxHeight{60};
constexpr int kReservePerRow{2};
constexpr int kReserveRows{kEnemyFrameCells / kReservePerRow};
constexpr Point kReserveFirstCell{.x = 1, .y = 1};
//NOTE: a picture inside a cell is inset half an atlas pixel on each side
constexpr double kCellMargin{0.5};
//NOTE: badges keep an atlas pixel clear of the pictures above and below
constexpr double kBadgeMarginDown{1.0};

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

int SideBarPixels(const double atlasPixels) { return static_cast<int>(std::lround(atlasPixels * kSideBarScale)); }

Point SideBarSize(const ObjRectangle& atlas)
{
	return Point{.x = SideBarPixels(atlas.w), .y = SideBarPixels(atlas.h)};
}

ObjRectangle AtlasCellOff(const ObjRectangle& picture, const Point offset)
{
	return ObjRectangle{.x = picture.x + offset.x, .y = picture.y + offset.y, .w = kAtlasCell, .h = kAtlasCell};
}

ObjRectangle FrameCell(const ObjRectangle& frame, const std::size_t cell)
{
	const auto column{static_cast<int>(cell % kReservePerRow)};
	const auto row{static_cast<int>(cell / kReservePerRow)};
	const Point offset{.x = kReserveFirstCell.x + column * static_cast<int>(kAtlasCell),
					   .y = kReserveFirstCell.y + row * static_cast<int>(kAtlasCell)};

	return AtlasCellOff(frame, offset);
}

//NOTE: the same whole-pixel inset on both sides - rounding each edge apart would shift the picture off center
SDL_Rect Inset(const SDL_Rect& place, const double atlasAcross, const double atlasDown)
{
	const int across{SideBarPixels(atlasAcross)};
	const int down{SideBarPixels(atlasDown)};

	return SDL_Rect{.x = place.x + across, .y = place.y + down, .w = place.w - across * 2, .h = place.h - down * 2};
}

//NOTE: a glyph inks only the top-left 7 of its 8 dots, so it is shifted half a dot to center it in the cell
ObjRectangle GlyphCells(const ObjRectangle& cells)
{
	constexpr double halfDot{0.5};

	return ObjRectangle{.x = cells.x + halfDot, .y = cells.y + halfDot, .w = cells.w, .h = cells.h};
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

//NOTE: top to bottom - the fps box, the enemy reserve right under it, the seats and the flag at their atlas places
void UiRenderer::DrawSideBar(const RenderSideBarEvent& event) const
{
	//NOTE: the lives digit cell, relative to the seat's picture
	constexpr Point livesCell{.x = 9, .y = 9};
	//NOTE: badges in the empty row under a seat's picture - the timer under the tank, the helmet under the lives
	constexpr Point timerCell{.x = 1, .y = 17};
	constexpr Point helmetCell{.x = 9, .y = 17};
	//NOTE: with three or four seats each is a row; its cells count from where the first seat's label starts
	constexpr int rowTimerCell{0};
	constexpr int rowNumberCell{1};
	constexpr int rowHelmetCell{3};
	constexpr int rowLivesCell{4};
	constexpr double seatRowsTop{1.0};
	//NOTE: the two cells under the flag's picture holding the stage number, right-aligned
	constexpr ObjRectangle stageCells{.x = 1, .y = 16, .w = kAtlasCell * 2, .h = kAtlasCell};
	//NOTE: the shovel badge spot between the flag's cloth and the stage number, two screen pixels clear of both
	constexpr ObjRectangle fortifiedSpace{.x = stageCells.x + kAtlasCell / 2, .y = 9, .w = kAtlasCell, .h = 7};
	constexpr double fortifiedMargin{0.75};

	const float scale{CurrentRenderScale()};
	const UiLayout::Measure measure{CellMeasurer(kSideBarPointSize, scale)};
	std::vector<UiTable> words{};
	std::vector<UiLayout::Placement> places{};
	const auto write = [&words, &places, &measure](UiTable table, const Point at)
	{
		places.push_back(UiLayout::Place(table, at, 0, measure));
		words.push_back(std::move(table));
	};
	const auto line = [](std::string text)
	{
		return UiTable{.rows = {UiRow{.cells = {TextCell(std::move(text), kSideBarTextColor)}}}};
	};
	const auto topLeft = [](const SDL_Rect& rect) { return Point{.x = rect.x, .y = rect.y}; };

	const ObjRectangle& frame{TextureOffset::kEnemyIconBackground};
	SdlRenderUtils::RenderCopyWithClipping(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(),
										   SdlRenderUtils::RectToSdlRect(frame), SideBarRect(frame));
	const int reserveShown{std::min(static_cast<int>(event.enemyReserve), kEnemyFrameCells)};
	//NOTE: one cell per reserve tank in order; tanks past the frame's cells are not shown
	std::ranges::for_each(std::views::iota(0, reserveShown), [this, &frame](const int cell)
	{
		const SDL_Rect place{SideBarRect(FrameCell(frame, static_cast<std::size_t>(cell)))};
		DrawIcon(UiIcon::SideBarEnemyTank, Inset(place, kCellMargin, kCellMargin));
	});
	DrawEnemyEffects(event, reserveShown);

	//NOTE: the atlas lays out two seats; with more, each seat is a row as wide as its lives number needs
	const ObjRectangle& firstSeat{TextureOffset::kPlayer1Icon};
	const ObjRectangle& flag{TextureOffset::kStageNumberFlag};
	const bool isRows{event.seats.size() > 2u};
	const std::size_t livesDigits{std::ranges::fold_left(event.seats | std::views::transform([](const SideBarSeat& seat)
	{
		return std::to_string(seat.lives).size();
	}), std::size_t{1}, std::ranges::max)};
	const double rowWidth{kAtlasCell * static_cast<double>(rowLivesCell + livesDigits)};
	const double rowLeft{frame.x + (frame.w - rowWidth) / 2.0};
	const double rowsTop{firstSeat.y + seatRowsTop};

	//NOTE: one gray block under the seats and the flag, reaching an atlas pixel past the outermost picture
	const double border{kReserveFirstCell.x};
	const double seatsBottom{isRows ? rowsTop + kAtlasCell * static_cast<double>(event.seats.size()) + border : flag.y};
	const double blockBottom{event.stage ? flag.y + flag.h + border : seatsBottom};
	const ObjRectangle block{.x = isRows ? rowLeft - border : frame.x,
							 .y = firstSeat.y,
							 .w = isRows ? rowWidth + border * 2.0 : frame.w,
							 .h = blockBottom - firstSeat.y};
	SdlRenderUtils::RenderCopyWithClipping(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(),
										   SdlRenderUtils::RectToSdlRect(TextureOffset::kSideBarGray),
										   SideBarRect(block));
	if (event.stage)
	{
		DrawIcon(UiIcon::SideBarStageFlag, SideBarRect(flag));
		DrawSideBarBadge(TextureOffset::kBonusShovel, event.fortified,
						 Inset(SideBarRect(ObjRectangle{.x = flag.x + fortifiedSpace.x,
													  .y = flag.y + fortifiedSpace.y,
													  .w = fortifiedSpace.w,
													  .h = fortifiedSpace.h}),
							   kCellMargin, fortifiedMargin));
	}

	if (isRows)
	{
		for (const auto& [index, seat]: std::views::zip(std::views::iota(0), event.seats))
		{
			const auto cell = [rowLeft, rowTop = rowsTop + kAtlasCell * index](const int column, const int span)
			{
				return ObjRectangle{.x = rowLeft + kAtlasCell * column,
									.y = rowTop,
									.w = kAtlasCell * span,
									.h = kAtlasCell};
			};
			const auto badgeIn = [this, &cell](const int column)
			{
				return Inset(SideBarRect(cell(column, 1)), kCellMargin, kBadgeMarginDown);
			};
			const ObjRectangle number{cell(rowNumberCell, rowHelmetCell - rowNumberCell)};
			DrawSideBarBadge(TextureOffset::kBonusTimer, seat.frozen, badgeIn(rowTimerCell));
			write(line(std::to_string(index + 1) + "P"), topLeft(SideBarRect(GlyphCells(number))));
			DrawSideBarBadge(TextureOffset::kBonusHelmet, seat.helmet, badgeIn(rowHelmetCell));
			write(line(std::to_string(seat.lives)), topLeft(SideBarRect(GlyphCells(cell(rowLivesCell, 1)))));
		}
	}
	else
	{
		constexpr std::array pictures{std::pair{UiIcon::SideBarPlayerOne, TextureOffset::kPlayer1Icon},
									  std::pair{UiIcon::SideBarPlayerTwo, TextureOffset::kPlayer2Icon}};
		const auto seats{std::views::zip(event.seats, pictures)};
		for (const auto& [seat, picture]: seats)
		{
			const auto& [icon, atlas] = picture;
			DrawIcon(icon, SideBarRect(atlas));
			write(line(std::to_string(seat.lives)),
				  topLeft(SideBarRect(GlyphCells(AtlasCellOff(atlas, livesCell)))));
		}

		//NOTE: badges go after all pictures - the gray margins of the next seat and of the flag overlap the badge row
		for (const auto& [seat, picture]: seats)
		{
			const ObjRectangle& atlas{picture.second};
			const auto badgeIn = [this, &atlas](const Point cell)
			{
				return Inset(SideBarRect(AtlasCellOff(atlas, cell)), kCellMargin, kBadgeMarginDown);
			};
			DrawSideBarBadge(TextureOffset::kBonusTimer, seat.frozen, badgeIn(timerCell));
			DrawSideBarBadge(TextureOffset::kBonusHelmet, seat.helmet, badgeIn(helmetCell));
		}
	}

	if (event.stage)
	{
		const std::string number{std::to_string(*event.stage)};
		const SDL_Rect cells{SideBarRect(GlyphCells(ObjRectangle{.x = flag.x + stageCells.x,
																 .y = flag.y + stageCells.y,
																 .w = stageCells.w,
																 .h = stageCells.h}))};
		const int width{measure(TextCell(number, kSideBarTextColor)).x};
		write(line(number), Point{.x = cells.x + cells.w - width, .y = cells.y});
	}
	else
	{
		//NOTE: in the menu font - twelve letters in the side bar's font would not fit its width
		const Point hintAt{.x = static_cast<int>(_gameConfig.battlefieldSize.x), .y = SideBarRect(flag).y};
		const UiLayout::Placement hint{UiLayout::CenteredAcross(
				UiLayout::Place(event.hint, hintAt, 0, CellMeasurer(SDL_Config::kFontSizePt, scale)), hintAt,
				static_cast<int>(WorldGeometry::kSideBarWidth))};
		DrawTables(std::array<PlacedTable, 1>{{{event.hint, hint}}}, SDL_Config::kFontSizePt, scale);
	}

	DrawTables(std::views::zip(words, places), kSideBarPointSize, scale);
}

//NOTE: helmets and free-for-all timers stand in the cells they took; the team timer centers on the first two free rows
void UiRenderer::DrawEnemyEffects(const RenderSideBarEvent& event, const int firstEmpty) const
{
	const ObjRectangle& frame{TextureOffset::kEnemyIconBackground};
	const auto inCell = [this](const ObjRectangle& cell)
	{
		return Inset(SideBarRect(cell), kCellMargin, kBadgeMarginDown);
	};

	std::ranges::for_each(event.enemyBadges, [this, &frame, &inCell](const SideBarEnemyBadge& badge)
	{
		const ObjRectangle& picture{badge.type == BonusType::Helmet ? TextureOffset::kBonusHelmet
																	: TextureOffset::kBonusTimer};
		DrawSideBarBadge(picture, badge.spent, inCell(FrameCell(frame, static_cast<std::size_t>(badge.cell))));
	});

	//NOTE: the team timer is centered across two reserve rows
	constexpr int timerRows{2};
	const int firstEmptyRow{(firstEmpty + kReservePerRow - 1) / kReservePerRow};
	const int timerRow{std::min(firstEmptyRow, kReserveRows - timerRows)};
	const auto halfCell{static_cast<int>(kAtlasCell) / 2};
	const Point middle{.x = kReserveFirstCell.x + halfCell,
					   .y = kReserveFirstCell.y + timerRow * static_cast<int>(kAtlasCell) + halfCell};
	DrawSideBarBadge(TextureOffset::kBonusTimer, event.enemyTeamFrozen, inCell(AtlasCellOff(frame, middle)));
}

void UiRenderer::DrawSideBarBadge(const ObjRectangle& picture, const std::optional<float> spent,
								  const SDL_Rect& place) const
{
	if (!spent)
	{
		return;
	}

	const SDL_FRect box{.x = static_cast<float>(picture.x),
						.y = static_cast<float>(picture.y),
						.w = static_cast<float>(TextureOffset::kBonusBoxWidth),
						.h = static_cast<float>(TextureOffset::kBonusBoxHeight)};
	const SDL_FRect room{SdlRenderUtils::ToFRect(place)};
	const float aspect{box.w / box.h};
	const float width{std::min(room.w, room.h * aspect)};
	const float height{width / aspect};
	const SDL_FRect fitted{.x = room.x + (room.w - width) / 2.f,
						   .y = room.y + (room.h - height) / 2.f,
						   .w = width,
						   .h = height};
	SdlRenderUtils::DrawBadge(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), box, fitted, *spent);
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
	//NOTE: one row as tall as the fps box centers the digits vertically, the column's middle centers them across
	const UiLayout::Placement placement{UiLayout::CenteredAcross(
			UiLayout::Place(fps, Point{}, kFpsBoxHeight, CellMeasurer(kSideBarPointSize, scale)),
			Point{.x = SideBarColumnX(), .y = 0}, SideBarSize(TextureOffset::kEnemyIconBackground).x)};

	const ScopedRenderScale scaled{_sdlConfig.renderer.get(), scale};
	DrawTableText(fps, placement, kSideBarPointSize, scale);
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
		case UiIcon::SideBarEnemyTank:
			return sprite(TextureOffset::kEnemyIcon, SideBarSize(TextureOffset::kEnemyIcon));
		case UiIcon::SideBarPlayerOne:
			return sprite(TextureOffset::kPlayer1Icon, SideBarSize(TextureOffset::kPlayer1Icon));
		case UiIcon::SideBarPlayerTwo:
			return sprite(TextureOffset::kPlayer2Icon, SideBarSize(TextureOffset::kPlayer2Icon));
		case UiIcon::SideBarStageFlag:
			return sprite(TextureOffset::kStageNumberFlag, SideBarSize(TextureOffset::kStageNumberFlag));
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

//NOTE: x of the reserve frame's left edge; the frame is centered in the side bar
int UiRenderer::SideBarColumnX() const
{
	const int frameWidth{SideBarSize(TextureOffset::kEnemyIconBackground).x};

	return static_cast<int>(_gameConfig.battlefieldSize.x)
		   + (static_cast<int>(WorldGeometry::kSideBarWidth) - frameWidth) / 2;
}

//NOTE: each edge rounded separately, so adjacent cells share an edge with no gap
SDL_Rect UiRenderer::SideBarRect(const ObjRectangle& atlas) const
{
	const ObjRectangle& frame{TextureOffset::kEnemyIconBackground};
	const int left{SideBarColumnX() + SideBarPixels(atlas.x - frame.x)};
	const int top{kFpsBoxHeight + SideBarPixels(atlas.y - frame.y)};

	return SDL_Rect{.x = left,
					.y = top,
					.w = SideBarColumnX() + SideBarPixels(atlas.x + atlas.w - frame.x) - left,
					.h = kFpsBoxHeight + SideBarPixels(atlas.y + atlas.h - frame.y) - top};
}
