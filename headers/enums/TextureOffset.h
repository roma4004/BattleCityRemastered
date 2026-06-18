#pragma once

#include "entities/ObjRectangle.h"

// struct ObjRectangle;
// tank size = 13 px
// BonusSize = 15 px
// eagle = 15 px
// explosion = 16 px

struct TextureOffset
{
	/*ObjRectangle enemy{0, 0, 13, 13};
	ObjRectangle playerOne{0, 13, 13, 13};
	ObjRectangle playerTwo{0, 26, 13, 13};
	ObjRectangle bullet{39, 13, 13, 13};
	ObjRectangle eagle{39, 101, 15, 15};
	ObjRectangle brick{40, 1, 9, 9};

	ObjRectangle steel{50, 0, 11, 11};//ALMOST DONE add sprites to atlas.png
	ObjRectangle water{61, 0, 11, 11};

	// ObjRectangle fortress{39,33,11,11}; redundant
	// ObjRectangle star{39,44,11,11};
	// ObjRectangle shovel{39,55,11,11};
	// ObjRectangle grenade{39,88,15,15};

	ObjRectangle bonusTimer{39, 39, 15, 15};
	ObjRectangle bonusTank{39, 54, 15, 15};
	ObjRectangle bonusHelmet{39, 69, 15, 15};*/


	// Battle City SpriteSheet.png Grid size 16x16
	// tanks are 13x13 so we need to make 1px offset cuz they're not aligned properly and start flickering while we rotate them

	//Tanks
	ObjRectangle enemy{.x = 129, .y = 1, .w = 13, .h = 13};
	ObjRectangle playerOne{.x = 1, .y = 1, .w = 13, .h = 13};
	ObjRectangle playerTwo{.x = 1, .y = 129, .w = 13, .h = 13};

	//Map
	ObjRectangle bullet{.x = 320, .y = 80, .w = 16, .h = 16};
	ObjRectangle eagle{.x = 304, .y = 32, .w = 16, .h = 16};
	ObjRectangle brick{.x = 256, .y = 64, .w = 8, .h = 8};
	ObjRectangle steel{.x = 256, .y = 16, .w = 8, .h = 8};
	ObjRectangle grass{.x = 272, .y = 32, .w = 8, .h = 8};
	ObjRectangle ice{.x = 288, .y = 32, .w = 8, .h = 8};
	//ObjRectangle water{264, 80, 8, 8}; // waterflow right - > left
	ObjRectangle water{.x = 272, .y = 80, .w = 8, .h = 8};// waterflow left  - > right

	//Explosions and spawn , left to right,
	ObjRectangle spawnAnim{.x = 256, .y = 96, .w = 16, .h = 16};// 4 step animation 16x16
	ObjRectangle smallExplosion{.x = 256, .y = 128, .w = 16, .h = 16};// 3 step animation 16x16
	ObjRectangle bigExplosion{.x = 304, .y = 128, .w = 32, .h = 32};// 2 steps 32x32

	//Bonuses
	ObjRectangle bonusHelmet{.x = 256, .y = 112, .w = 16, .h = 16};
	ObjRectangle helmetAnimationFrame{.x = 352, .y = 112, .w = 16, .h = 16};// 2 step animation 16x16, left -> right

	ObjRectangle bonusTimer{.x = 272, .y = 112, .w = 16, .h = 16};
	ObjRectangle bonusShovel{.x = 288, .y = 112, .w = 16, .h = 16};
	ObjRectangle bonusStar{.x = 304, .y = 112, .w = 16, .h = 16};
	ObjRectangle bonusGrenade{.x = 320, .y = 112, .w = 16, .h = 16};
	ObjRectangle bonusTank{.x = 336, .y = 112, .w = 16, .h = 16};
	ObjRectangle bonusCaliber{.x = 352, .y = 112, .w = 16, .h = 16};
	ObjRectangle bonusShip{.x = 352, .y = 80, .w = 16, .h = 16};//TODO implement new bonus (make logic\code)

	//Text
	ObjRectangle pauseText{.x = 288, .y = 175, .w = 40, .h = 8};
	ObjRectangle gameOverText{.x = 288, .y = 183, .w = 32, .h = 18};
	ObjRectangle gameWonText{.x = 375, .y = 183, .w = 18, .h = 17};

	//SideBar
	ObjRectangle rightSideBar{.x = 625, .y = 0, .w = 220, .h = 600};
	ObjRectangle rightSideBarCloth{.x = 375, .y = 23, .w = 18, .h = 82};
	ObjRectangle enemyIcon{.x = 321, .y = 193, .w = 7, .h = 7};
	ObjRectangle playerOneIcon{.x = 375, .y = 135, .w = 19, .h = 18};
	ObjRectangle playerTwoIcon{.x = 375, .y = 159, .w = 19, .h = 18};
	ObjRectangle stageNumberFlag{.x = 375, .y = 182, .w = 18, .h = 28};
};
