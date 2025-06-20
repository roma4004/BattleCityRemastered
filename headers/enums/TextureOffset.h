#pragma once

#include "../ObjRectangle.h"

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
	ObjRectangle enemy{129, 1, 13, 13};
	ObjRectangle playerOne{1, 1, 13, 13};
	ObjRectangle playerTwo{1, 129, 13, 13};

	//Map
	ObjRectangle bullet{320, 80, 16, 16};
	ObjRectangle eagle{304, 32, 16, 16};
	ObjRectangle brick{256, 64, 8, 8};
	ObjRectangle steel{256, 16, 8, 8};
	ObjRectangle grass{272, 32, 8, 8};
	ObjRectangle ice{288, 32, 8, 8};
	//ObjRectangle water{264, 80, 8, 8}; // waterflow right - > left
	ObjRectangle water{272, 80, 8, 8};// waterflow left  - > right

	//Explosions and spawn , left to right,
	ObjRectangle spawnAnim{256, 96, 16, 16};// 4 step animation 16x16
	ObjRectangle smallExplosion{256, 128, 16, 16};// 3 step animation 16x16
	ObjRectangle bigExplosion{304, 128, 32, 32};// 2 steps 32x32

	//Bonuses
	ObjRectangle bonusHelmet{256, 112, 16, 16};
	ObjRectangle helmetAnimationFrame{352, 112, 16, 16}; // 2 step animation 16x16 , left -> right
	
	ObjRectangle bonusTimer{272, 112, 16, 16};
	ObjRectangle bonusShovel{288, 112, 16, 16};
	ObjRectangle bonusStar{304, 112, 16, 16};
	ObjRectangle bonusGrenade{320, 112, 16, 16};
	ObjRectangle bonusTank{336, 112, 16, 16};
	ObjRectangle bonusCaliber{352, 112, 16, 16};
	
	
};
