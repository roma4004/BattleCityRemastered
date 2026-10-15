#pragma once

#include "geometry/Point.h"
#include <SDL3/SDL_render.h>

struct ObjRectangle;

//NOTE: what the world and the interface both draw with
class SdlRenderUtils final
{
public:
	//NOTE: everything here is laid out in whole logical pixels; SDL3 wants floats only at the call
	[[nodiscard]] static SDL_FRect ToFRect(const SDL_Rect& rect);
	[[nodiscard]] static SDL_Rect RectToSdlRect(const ObjRectangle& rect);
	[[nodiscard]] static SDL_Rect RectOf(Point pos, Point size);
	[[nodiscard]] static SDL_Color IntToColor(unsigned int color);
	[[nodiscard]] static unsigned int ColorToInt(const SDL_Color& color);
	//NOTE: SDL_GetRenderScale misses the logical presentation - the letterbox rect maps a logical pixel to the window
	[[nodiscard]] static SDL_FPoint LogicalScale(SDL_Renderer* renderer);
	[[nodiscard]] static SDL_FRect SnapToWindowPixels(SDL_Renderer* renderer, const SDL_FRect& rect);
	//NOTE: turned a quarter about the middle, covers rect
	[[nodiscard]] static SDL_FRect SwapSidesAboutCenter(const SDL_FRect& rect);
	static void FillRect(SDL_Renderer* renderer, const SDL_Rect& rect);
	static void RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, SDL_Rect dstRect);
	static void RenderCopyWithClipping(SDL_Renderer* renderer, SDL_Texture* texture, SDL_Rect srcRect,
									   SDL_Rect dstRect);
};
