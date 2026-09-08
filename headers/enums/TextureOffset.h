#pragma once

#include "geometry/ObjRectangle.h"

//NOTE: static - one copy of the atlas layout, no instance to hold
struct TextureOffset
{
	// NOTE: the sheet is a 16x16 grid holding 13x13 tanks, off-centre by a pixel - without the offset
	// they flicker when rotated

	// Tanks
	static constexpr ObjRectangle kEnemy{.x = 129, .y = 1, .w = 13, .h = 13};
	static constexpr ObjRectangle kPlayer1{.x = 1, .y = 1, .w = 13, .h = 13};
	static constexpr ObjRectangle kPlayer2{.x = 1, .y = 129, .w = 13, .h = 13};

	// Map
	static constexpr ObjRectangle kBullet{.x = 320, .y = 80, .w = 16, .h = 16};
	static constexpr ObjRectangle kEagle{.x = 304, .y = 32, .w = 16, .h = 16};
	static constexpr ObjRectangle kBrick{.x = 256, .y = 64, .w = 8, .h = 8};
	static constexpr ObjRectangle kSteel{.x = 256, .y = 16, .w = 8, .h = 8};
	static constexpr ObjRectangle kBush{.x = 272, .y = 32, .w = 8, .h = 8};
	static constexpr ObjRectangle kIce{.x = 288, .y = 32, .w = 8, .h = 8};
	static constexpr ObjRectangle kWater{.x = 272, .y = 80, .w = 8, .h = 8};//left -> right; played back-to-front frame

	// Effects
	static constexpr ObjRectangle kTankSpawn{.x = 256, .y = 96, .w = 16, .h = 16};// 4 frames, animation 16x16
	static constexpr ObjRectangle kBulletExplosion{.x = 256, .y = 128, .w = 16, .h = 16};// 3 frames, animation 16x16
	static constexpr ObjRectangle kTankExplosion{.x = 304, .y = 128, .w = 32, .h = 32};// 2 frames, 32x32
	// 2 frames, animation 16x16, left -> right
	static constexpr ObjRectangle kHelmetEffect{.x = 256, .y = 144, .w = 16, .h = 16};

	// Bonuses
	static constexpr ObjRectangle kBonusHelmet{.x = 256, .y = 112, .w = 16, .h = 16};
	static constexpr ObjRectangle kBonusTimer{.x = 272, .y = 112, .w = 16, .h = 16};
	static constexpr ObjRectangle kBonusShovel{.x = 288, .y = 112, .w = 16, .h = 16};
	static constexpr ObjRectangle kBonusStar{.x = 304, .y = 112, .w = 16, .h = 16};
	static constexpr ObjRectangle kBonusGrenade{.x = 320, .y = 112, .w = 16, .h = 16};
	static constexpr ObjRectangle kBonusTank{.x = 336, .y = 112, .w = 16, .h = 16};
	static constexpr ObjRectangle kBonusCaliber{.x = 352, .y = 112, .w = 16, .h = 16};
	static constexpr ObjRectangle kBonusShip{.x = 352, .y = 96, .w = 16, .h = 16};

	// Text
	static constexpr ObjRectangle kPauseText{.x = 288, .y = 175, .w = 40, .h = 8};
	static constexpr ObjRectangle kGameOverText{.x = 288, .y = 183, .w = 32, .h = 18};
	static constexpr ObjRectangle kGameWonText{.x = 375, .y = 183, .w = 18, .h = 17};

	// SideBar
	static constexpr ObjRectangle kEnemyIconBackground{.x = 375, .y = 23, .w = 18, .h = 82};
	static constexpr ObjRectangle kEnemyIcon{.x = 321, .y = 193, .w = 7, .h = 7};
	static constexpr ObjRectangle kPlayer1Icon{.x = 375, .y = 135, .w = 19, .h = 18};
	static constexpr ObjRectangle kPlayer2Icon{.x = 375, .y = 159, .w = 19, .h = 18};
	static constexpr ObjRectangle kStageNumberFlag{.x = 375, .y = 182, .w = 18, .h = 28};
};
