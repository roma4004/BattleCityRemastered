#pragma once

struct ObjRectangle;
struct SideBarEnemies;
class SideBarCanvas;

//NOTE: the enemy frame - reserve tanks, and the enemies' effects in the empty cells
class SideBarEnemyPainter final
{
	const SideBarCanvas& _canvas;

	[[nodiscard]] static ObjRectangle FrameCell(int cell);
	void PaintEffects(const SideBarEnemies& enemies, int firstEmpty) const;

public:
	explicit SideBarEnemyPainter(const SideBarCanvas& canvas);

	void Paint(const SideBarEnemies& enemies) const;
};
