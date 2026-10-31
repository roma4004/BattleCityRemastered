#pragma once

#include "components/UiLayout.h"
#include "components/UiTable.h"
#include <SDL3/SDL_rect.h>
#include <string>
#include <vector>

struct SideBarPlayers;
struct SideBarSeat;
class SideBarCanvas;

//NOTE: placed, not drawn - the caller writes all of them in one batch
struct SideBarWords
{
	std::vector<UiTable> tables{};
	std::vector<UiLayout::Placement> places{};
	std::vector<UiTable> labels{};
	std::vector<UiLayout::Placement> labelPlaces{};

	//NOTE: one line from the place's top-left corner
	void Add(std::string text, const SDL_Rect& place, const UiLayout::Measure& measure);
};

//NOTE: the gray block under the enemy frame - the seats, then the stage flag
class SideBarPlayerPainter final
{
	const SideBarCanvas& _canvas;

	void PaintSeatPictures(const std::vector<SideBarSeat>& seats, SideBarWords& words, const UiLayout::Measure& barFont,
						   const UiLayout::Measure& labelFont) const;
	void PaintSeatRows(const std::vector<SideBarSeat>& seats, double left, double top, SideBarWords& words,
					   const UiLayout::Measure& barFont, const UiLayout::Measure& labelFont) const;
	void PaintLabel(const std::string& label, const SDL_Rect& seat, SideBarWords& words,
					const UiLayout::Measure& labelFont) const;

public:
	explicit SideBarPlayerPainter(const SideBarCanvas& canvas);

	[[nodiscard]] SideBarWords Paint(const SideBarPlayers& players, const UiLayout::Measure& barFont,
									 const UiLayout::Measure& labelFont) const;
};
