#pragma once

#include "geometry/ObjRectangle.h"

struct TextureOffset
{
	// NOTE: Battle City SpriteSheet.png Grid size 16x16
	// tanks are 13x13px, so we need to make 1px offset,
	// cuz they're not aligned properly and start flickering while we rotate them

	// Tanks
	ObjRectangle enemy{.x = 129, .y = 1, .w = 13, .h = 13};
	ObjRectangle playerOne{.x = 1, .y = 1, .w = 13, .h = 13};
	ObjRectangle playerTwo{.x = 1, .y = 129, .w = 13, .h = 13};

	// Map
	ObjRectangle bullet{.x = 320, .y = 80, .w = 16, .h = 16};
	ObjRectangle eagle{.x = 304, .y = 32, .w = 16, .h = 16};
	ObjRectangle brick{.x = 256, .y = 64, .w = 8, .h = 8};
	ObjRectangle steel{.x = 256, .y = 16, .w = 8, .h = 8};
	ObjRectangle bush{.x = 272, .y = 32, .w = 8, .h = 8};
	ObjRectangle ice{.x = 288, .y = 32, .w = 8, .h = 8};
	ObjRectangle water{.x = 272, .y = 80, .w = 8, .h = 8};// waterflow left - > right; played back-to-front frame

	// Effects
	ObjRectangle tankSpawn{.x = 256, .y = 96, .w = 16, .h = 16};// 4 frames, animation 16x16
	ObjRectangle bulletExplosion{.x = 256, .y = 128, .w = 16, .h = 16};// 3 frames, animation 16x16
	ObjRectangle tankExplosion{.x = 304, .y = 128, .w = 32, .h = 32};// 2 frames, 32x32
	ObjRectangle helmetEffect{.x = 256, .y = 144, .w = 16, .h = 16};// 2 frames, animation 16x16, left -> right

	// Bonuses
	ObjRectangle bonusHelmet{.x = 256, .y = 112, .w = 16, .h = 16};
	ObjRectangle bonusTimer{.x = 272, .y = 112, .w = 16, .h = 16};
	ObjRectangle bonusShovel{.x = 288, .y = 112, .w = 16, .h = 16};
	ObjRectangle bonusStar{.x = 304, .y = 112, .w = 16, .h = 16};
	ObjRectangle bonusGrenade{.x = 320, .y = 112, .w = 16, .h = 16};
	ObjRectangle bonusTank{.x = 336, .y = 112, .w = 16, .h = 16};
	ObjRectangle bonusCaliber{.x = 352, .y = 112, .w = 16, .h = 16};
	ObjRectangle bonusShip{.x = 352, .y = 80, .w = 16, .h = 16};//TODO implement new bonus (make logic\code)

	// Text
	ObjRectangle pauseText{.x = 288, .y = 175, .w = 40, .h = 8};
	ObjRectangle gameOverText{.x = 288, .y = 183, .w = 32, .h = 18};
	ObjRectangle gameWonText{.x = 375, .y = 183, .w = 18, .h = 17};

	// SideBar
	ObjRectangle rightSideBar{.x = 625, .y = 0, .w = 175, .h = 600};
	ObjRectangle enemyIconBackground{.x = 375, .y = 23, .w = 18, .h = 82};
	ObjRectangle enemyIcon{.x = 321, .y = 193, .w = 7, .h = 7};
	ObjRectangle playerOneIcon{.x = 375, .y = 135, .w = 19, .h = 18};
	ObjRectangle playerTwoIcon{.x = 375, .y = 159, .w = 19, .h = 18};
	ObjRectangle stageNumberFlag{.x = 375, .y = 182, .w = 18, .h = 28};
};
