#pragma once

#include "geometry/Point.h"
#include <SDL3/SDL_rect.h>
#include <optional>

struct ObjRectangle;
struct SDL_Config;
class GameConfig;

//NOTE: side bar places are atlas pixels off the enemy frame's corner; this draws them at screen size
class SideBarCanvas final
{
	const SDL_Config& _sdlConfig;
	const GameConfig& _gameConfig;

	//NOTE: the enemy frame's left edge, centered in the side bar
	[[nodiscard]] int ColumnX() const;
	[[nodiscard]] static int Pixels(double atlasPixels);

public:
	static constexpr double kCell{8.0};
	//NOTE: one scale for the whole bar keeps the original gaps
	static constexpr double kScale{2.75};
	//NOTE: a digit fills one atlas cell
	static constexpr int kPointSize{static_cast<int>(kCell * kScale)};
	//NOTE: square symbols - "PRESS TO START" fits across the bar
	static constexpr int kLabelPointSize{11};
	static constexpr double kCellMargin{0.5};
	//NOTE: keeps a badge clear of the pictures above and below
	static constexpr double kBadgeMarginDown{1.0};

	SideBarCanvas(const SDL_Config& sdlConfig, const GameConfig& gameConfig);

	//NOTE: the side bar starts where the field ends
	[[nodiscard]] int Left() const;
	//NOTE: over the enemy frame, as wide as it
	[[nodiscard]] SDL_Rect FpsBox() const;
	[[nodiscard]] SDL_Rect Rect(const ObjRectangle& atlas) const;
	void Draw(const ObjRectangle& atlas, const SDL_Rect& place) const;
	void DrawBadge(const ObjRectangle& picture, std::optional<float> spent, const SDL_Rect& place) const;

	[[nodiscard]] static ObjRectangle CellOff(const ObjRectangle& picture, Point offset);
	[[nodiscard]] static SDL_Rect Inset(const SDL_Rect& place, double atlasAcross, double atlasDown);
	[[nodiscard]] static ObjRectangle GlyphCells(const ObjRectangle& cells);
};
