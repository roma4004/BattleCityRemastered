#include "components/SideBarEnemyPainter.h"
#include "components/SideBarCanvas.h"
#include "components/events/RenderUIEvents.h"
#include "enums/BonusType.h"
#include "enums/TextureOffset.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include <algorithm>
#include <ranges>
#include <SDL3/SDL_rect.h>

namespace
{
constexpr int kReservePerRow{2};
constexpr int kReserveRows{kEnemyFrameCells / kReservePerRow};
constexpr Point kReserveFirstCell{.x = 1, .y = 1};
}//namespace

SideBarEnemyPainter::SideBarEnemyPainter(const SideBarCanvas& canvas)
	: _canvas{canvas}
{
}

//NOTE: a reserve past the frame's cells is not shown
void SideBarEnemyPainter::Paint(const SideBarEnemies& enemies) const
{
	const ObjRectangle& frame{TextureOffset::kEnemyIconBackground};
	_canvas.Draw(frame, _canvas.Rect(frame));

	const int reserveShown{std::min(static_cast<int>(enemies.reserve), kEnemyFrameCells)};
	std::ranges::for_each(std::views::iota(0, reserveShown), [this](const int cell)
	{
		const SDL_Rect place{_canvas.Rect(FrameCell(cell))};
		_canvas.Draw(TextureOffset::kEnemyIcon,
					 SideBarCanvas::Inset(place, SideBarCanvas::kCellMargin, SideBarCanvas::kCellMargin));
	});

	PaintEffects(enemies, reserveShown);
}

ObjRectangle SideBarEnemyPainter::FrameCell(const int cell)
{
	const int column{cell % kReservePerRow};
	const int row{cell / kReservePerRow};
	const Point offset{.x = kReserveFirstCell.x + column * static_cast<int>(SideBarCanvas::kCell),
					   .y = kReserveFirstCell.y + row * static_cast<int>(SideBarCanvas::kCell)};

	return SideBarCanvas::CellOff(TextureOffset::kEnemyIconBackground, offset);
}

//NOTE: seat badges keep the cells they took; the team timer centers on the first two free rows
void SideBarEnemyPainter::PaintEffects(const SideBarEnemies& enemies, const int firstEmpty) const
{
	const auto inCell = [this](const ObjRectangle& cell)
	{
		return SideBarCanvas::Inset(_canvas.Rect(cell), SideBarCanvas::kCellMargin, SideBarCanvas::kBadgeMarginDown);
	};

	std::ranges::for_each(enemies.badges, [this, &inCell](const SideBarEnemyBadge& badge)
	{
		const ObjRectangle& picture{badge.type == BonusType::Helmet ? TextureOffset::kBonusHelmet
																	: TextureOffset::kBonusTimer};
		_canvas.DrawBadge(picture, badge.spent, inCell(FrameCell(badge.cell)));
	});

	constexpr int timerRows{2};
	const int firstEmptyRow{(firstEmpty + kReservePerRow - 1) / kReservePerRow};
	const int timerRow{std::min(firstEmptyRow, kReserveRows - timerRows)};
	const auto halfCell{static_cast<int>(SideBarCanvas::kCell) / 2};
	const Point middle{.x = kReserveFirstCell.x + halfCell,
					   .y = kReserveFirstCell.y + timerRow * static_cast<int>(SideBarCanvas::kCell) + halfCell};
	_canvas.DrawBadge(TextureOffset::kBonusTimer, enemies.teamFrozen,
					  inCell(SideBarCanvas::CellOff(TextureOffset::kEnemyIconBackground, middle)));
}
