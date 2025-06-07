#pragma once

struct ObjRectangle;
// tank size = 13 px
// BonusSize = 15 px
// eagle = 15 px
// explosion = 16 px

struct TextureOffset//TODO make atlas.png
{
	ObjRectangle enemy{0, 0, 13, 13};
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
	ObjRectangle bonusHelmet{39, 69, 15, 15};
};
