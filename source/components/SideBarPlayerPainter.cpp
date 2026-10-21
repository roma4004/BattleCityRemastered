#include "components/SideBarPlayerPainter.h"
#include "components/SideBarCanvas.h"
#include "components/UiLayout.h"
#include "components/UiTable.h"
#include "components/WorldGeometry.h"
#include "components/events/RenderUIEvents.h"
#include "enums/TextureOffset.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <SDL3/SDL_rect.h>
#include <string>
#include <utility>
#include <vector>

namespace
{
//NOTE: a seat row's lives cell; the row ends after the lives digits
constexpr int kRowLivesCell{4};
}//namespace

void SideBarWords::Add(std::string text, const SDL_Rect& place, const UiLayout::Measure& measure)
{
	UiTable table{.rows = {UiRow{.cells = {TextCell(std::move(text), kSideBarTextColor)}}}};
	places.push_back(UiLayout::Place(table, Point{.x = place.x, .y = place.y}, 0, measure));
	tables.push_back(std::move(table));
}

SideBarPlayerPainter::SideBarPlayerPainter(const SideBarCanvas& canvas)
	: _canvas{canvas}
{
}

//NOTE: the atlas has pictures for two seats; three or four go as rows
SideBarWords SideBarPlayerPainter::Paint(const SideBarPlayers& players, const UiLayout::Measure& barFont,
										 const UiLayout::Measure& menuFont) const
{
	constexpr double seatRowsTop{1.0};
	//NOTE: how far the gray block reaches past the outermost picture
	constexpr double border{1.0};
	//NOTE: the stage number's two cells under the flag, right-aligned
	constexpr ObjRectangle stageCells{.x = 1, .y = 16, .w = SideBarCanvas::kCell * 2, .h = SideBarCanvas::kCell};
	//NOTE: the shovel's spot between the flag's cloth and the number, two screen pixels clear of both
	constexpr ObjRectangle fortifiedSpace{.x = stageCells.x + SideBarCanvas::kCell / 2,
										  .y = 9,
										  .w = SideBarCanvas::kCell,
										  .h = 7};
	constexpr double fortifiedMargin{2.0 / SideBarCanvas::kScale};

	const ObjRectangle& frame{TextureOffset::kEnemyIconBackground};
	const ObjRectangle& firstSeat{TextureOffset::kPlayer1Icon};
	const ObjRectangle& flag{TextureOffset::kStageNumberFlag};
	const bool isRows{players.seats.size() > 2u};
	const unsigned short mostLives{std::ranges::fold_left(players.seats | std::views::transform(&SideBarSeat::lives),
														  decltype(SideBarSeat::lives){}, std::ranges::max)};
	const std::size_t livesDigits{std::to_string(mostLives).size()};
	const double rowWidth{SideBarCanvas::kCell * static_cast<double>(kRowLivesCell + livesDigits)};
	const double rowLeft{frame.x + (frame.w - rowWidth) / 2.0};
	const double rowsTop{firstSeat.y + seatRowsTop};

	const double rowsHeight{SideBarCanvas::kCell * static_cast<double>(players.seats.size())};
	const double seatsBottom{isRows ? rowsTop + rowsHeight + border : flag.y};
	const double blockBottom{players.hint ? seatsBottom : flag.y + flag.h + border};
	const ObjRectangle block{.x = isRows ? rowLeft - border : frame.x,
							 .y = firstSeat.y,
							 .w = isRows ? rowWidth + border * 2.0 : frame.w,
							 .h = blockBottom - firstSeat.y};
	_canvas.Draw(TextureOffset::kSideBarGray, _canvas.Rect(block));
	if (!players.hint)
	{
		_canvas.Draw(flag, _canvas.Rect(flag));
		const ObjRectangle fortified{.x = flag.x + fortifiedSpace.x,
									 .y = flag.y + fortifiedSpace.y,
									 .w = fortifiedSpace.w,
									 .h = fortifiedSpace.h};
		_canvas.DrawBadge(TextureOffset::kBonusShovel, players.fortified,
						  SideBarCanvas::Inset(_canvas.Rect(fortified), SideBarCanvas::kCellMargin, fortifiedMargin));
	}

	SideBarWords words{};
	if (isRows)
	{
		PaintSeatRows(players.seats, rowLeft, rowsTop, words, barFont);
	}
	else
	{
		PaintSeatPictures(players.seats, words, barFont);
	}

	if (players.hint)
	{
		const Point hintAt{.x = _canvas.Left(), .y = _canvas.Rect(flag).y};
		words.hint = UiLayout::CenteredAcross(UiLayout::Place(*players.hint, hintAt, 0, menuFont), hintAt,
											  static_cast<int>(WorldGeometry::kSideBarWidth));

		return words;
	}

	const std::string number{std::to_string(players.stage)};
	const SDL_Rect cells{_canvas.Rect(SideBarCanvas::GlyphCells(ObjRectangle{.x = flag.x + stageCells.x,
																			 .y = flag.y + stageCells.y,
																			 .w = stageCells.w,
																			 .h = stageCells.h}))};
	const int width{barFont(TextCell(number, kSideBarTextColor)).x};
	words.Add(number, SDL_Rect{.x = cells.x + cells.w - width, .y = cells.y, .w = width, .h = cells.h}, barFont);

	return words;
}

void SideBarPlayerPainter::PaintSeatPictures(const std::vector<SideBarSeat>& seats, SideBarWords& words,
											 const UiLayout::Measure& barFont) const
{
	//NOTE: off the seat's picture; badges take the empty row under it
	constexpr Point livesCell{.x = 9, .y = 9};
	constexpr Point timerCell{.x = 1, .y = 17};
	constexpr Point helmetCell{.x = 9, .y = 17};
	constexpr std::array pictures{TextureOffset::kPlayer1Icon, TextureOffset::kPlayer2Icon};

	const auto seated{std::views::zip(seats, pictures)};
	for (const auto [seat, picture]: seated)
	{
		_canvas.Draw(picture, _canvas.Rect(picture));
		words.Add(std::to_string(seat.lives),
				  _canvas.Rect(SideBarCanvas::GlyphCells(SideBarCanvas::CellOff(picture, livesCell))), barFont);
	}

	//NOTE: badges after all pictures - the next picture's gray margin overlaps the badge row
	for (const auto [seat, picture]: seated)
	{
		const auto badgeIn = [this, &picture](const Point cell)
		{
			return SideBarCanvas::Inset(_canvas.Rect(SideBarCanvas::CellOff(picture, cell)), SideBarCanvas::kCellMargin,
										SideBarCanvas::kBadgeMarginDown);
		};
		_canvas.DrawBadge(TextureOffset::kBonusTimer, seat.frozen, badgeIn(timerCell));
		_canvas.DrawBadge(TextureOffset::kBonusHelmet, seat.helmet, badgeIn(helmetCell));
	}
}

void SideBarPlayerPainter::PaintSeatRows(const std::vector<SideBarSeat>& seats, const double left, const double top,
										 SideBarWords& words, const UiLayout::Measure& barFont) const
{
	constexpr int timerCell{0};
	constexpr int numberCell{1};
	constexpr int helmetCell{3};

	for (const auto [index, seat]: std::views::zip(std::views::iota(0), seats))
	{
		const auto cell = [left, rowTop = top + SideBarCanvas::kCell * index](const int column, const int span)
		{
			return ObjRectangle{.x = left + SideBarCanvas::kCell * column,
								.y = rowTop,
								.w = SideBarCanvas::kCell * span,
								.h = SideBarCanvas::kCell};
		};
		const auto badgeIn = [this, &cell](const int column)
		{
			return SideBarCanvas::Inset(_canvas.Rect(cell(column, 1)), SideBarCanvas::kCellMargin,
										SideBarCanvas::kBadgeMarginDown);
		};
		const ObjRectangle number{cell(numberCell, helmetCell - numberCell)};
		_canvas.DrawBadge(TextureOffset::kBonusTimer, seat.frozen, badgeIn(timerCell));
		words.Add(std::to_string(index + 1) + "P", _canvas.Rect(SideBarCanvas::GlyphCells(number)), barFont);
		_canvas.DrawBadge(TextureOffset::kBonusHelmet, seat.helmet, badgeIn(helmetCell));
		words.Add(std::to_string(seat.lives), _canvas.Rect(SideBarCanvas::GlyphCells(cell(kRowLivesCell, 1))),
				  barFont);
	}
}
