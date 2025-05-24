#pragma once

#include <SDL_rect.h>

struct TextureOffset : SDL_Rect //TODO make atlas.png
{
	SDL_Rect enemy{0, 0, 15, 15};
	SDL_Rect playerOne{0, 30, 15, 15};
	SDL_Rect playerTwo{0, 15, 15, 15};
	SDL_Rect bullet{7, 66,3, 4};
	SDL_Rect eagle{0, 45, 15, 15};
};
