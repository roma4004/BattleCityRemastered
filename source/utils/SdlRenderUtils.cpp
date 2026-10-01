#include "utils/SdlRenderUtils.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include <SDL3/SDL_render.h>

SDL_FRect SdlRenderUtils::ToFRect(const SDL_Rect& rect)
{
	return SDL_FRect{.x = static_cast<float>(rect.x),
					 .y = static_cast<float>(rect.y),
					 .w = static_cast<float>(rect.w),
					 .h = static_cast<float>(rect.h)};
}

SDL_Rect SdlRenderUtils::RectToSdlRect(const ObjRectangle& rect)
{
	return SDL_Rect{.x = static_cast<int>(rect.x),
					.y = static_cast<int>(rect.y),
					.w = static_cast<int>(rect.w),
					.h = static_cast<int>(rect.h)};
}

SDL_Rect SdlRenderUtils::RectOf(const Point pos, const Point size)
{
	return SDL_Rect{.x = pos.x, .y = pos.y, .w = size.x, .h = size.y};
}

SDL_Color SdlRenderUtils::IntToColor(const unsigned int color)
{
	return SDL_Color{.r = static_cast<Uint8>((color >> 16u) & 0xFFu),
					 .g = static_cast<Uint8>((color >> 8u) & 0xFFu),
					 .b = static_cast<Uint8>((color >> 0u) & 0xFFu),
					 .a = static_cast<Uint8>((color >> 24u) & 0xFFu)};
}

unsigned int SdlRenderUtils::ColorToInt(const SDL_Color& color)
{
	//NOTE: Uint8 promotes to int and 255 << 24 lands on the sign bit - the widening keeps it unsigned
	return (Uint32{color.a} << 24u) | (Uint32{color.r} << 16u) | (Uint32{color.g} << 8u) | Uint32{color.b};
}

void SdlRenderUtils::FillRect(SDL_Renderer* const renderer, const SDL_Rect& rect)
{
	const SDL_FRect target{ToFRect(rect)};
	SDL_RenderFillRect(renderer, &target);
}

void SdlRenderUtils::RenderCopy(SDL_Renderer* const renderer, SDL_Texture* const texture, const SDL_Rect dstRect)
{
	const SDL_FRect dst{ToFRect(dstRect)};
	SDL_RenderTexture(renderer, texture, nullptr, &dst);
}

void SdlRenderUtils::RenderCopyWithClipping(SDL_Renderer* const renderer, SDL_Texture* const texture,
											const SDL_Rect srcRect, const SDL_Rect dstRect)
{
	const SDL_FRect src{ToFRect(srcRect)};
	const SDL_FRect dst{ToFRect(dstRect)};
	SDL_RenderTexture(renderer, texture, &src, &dst);
}
