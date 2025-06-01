#pragma once

#include <SDL_rect.h>

// tank size = 13 px
// BonusSize = 15 px
// eagle = 15 px
// explosion = 16 px

struct TextureOffset : SDL_Rect//TODO make atlas.png
{
	SDL_Rect enemy{0, 0, 13, 13};
	SDL_Rect playerOne{0, 13, 13, 13};
	SDL_Rect playerTwo{0, 26, 13, 13};
	SDL_Rect bullet{39, 13, 13, 13};
	SDL_Rect eagle{39, 101, 15, 15};
	SDL_Rect brick{40, 1, 9, 9};

	SDL_Rect steel{50, 0, 11, 11};//ALMOST DONE add sprites to atlas.png
	SDL_Rect water{61, 0, 11, 11};

	//SDL_Rect fortress{39,33,11,11}; redundant
	//SDL_Rect star{39,44,11,11};
	//SDL_Rect shovel{39,55,11,11};
	//SDL_Rect grenade{39,88,15,15};

	SDL_Rect bonusTimer{39, 39, 15, 15};
	SDL_Rect bonusTank{39, 54, 15, 15};
	SDL_Rect bonusHelmet{39, 69, 15, 15};
};
