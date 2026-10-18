#include "utils/SdlRenderUtils.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include <algorithm>
#include <cmath>
#include <SDL3/SDL_rect.h>
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

SDL_FPoint SdlRenderUtils::LogicalScale(SDL_Renderer* const renderer)
{
	int logicalWidth{};
	int logicalHeight{};
	SDL_RendererLogicalPresentation mode{SDL_LOGICAL_PRESENTATION_DISABLED};
	SDL_FRect presentation{};
	if (!SDL_GetRenderLogicalPresentation(renderer, &logicalWidth, &logicalHeight, &mode)
		|| mode == SDL_LOGICAL_PRESENTATION_DISABLED
		|| logicalWidth <= 0
		|| logicalHeight <= 0
		|| !SDL_GetRenderLogicalPresentationRect(renderer, &presentation)
		|| presentation.w <= 0.f
		|| presentation.h <= 0.f)
	{
		return SDL_FPoint{.x = 1.f, .y = 1.f};
	}

	return SDL_FPoint{.x = presentation.w / static_cast<float>(logicalWidth),
					  .y = presentation.h / static_cast<float>(logicalHeight)};
}

SDL_FRect SdlRenderUtils::SnapToWindowPixels(SDL_Renderer* const renderer, const SDL_FRect& rect)
{
	const auto [scaleX, scaleY]{LogicalScale(renderer)};
	const auto snap = [](const float logical, const float scale) { return std::round(logical * scale) / scale; };
	const float left{snap(rect.x, scaleX)};
	const float top{snap(rect.y, scaleY)};

	return SDL_FRect{.x = left,
					 .y = top,
					 .w = snap(rect.x + rect.w, scaleX) - left,
					 .h = snap(rect.y + rect.h, scaleY) - top};
}

SDL_FRect SdlRenderUtils::SwapSidesAboutCenter(const SDL_FRect& rect)
{
	return SDL_FRect{.x = rect.x + (rect.w - rect.h) / 2.f,
					 .y = rect.y + (rect.h - rect.w) / 2.f,
					 .w = rect.h,
					 .h = rect.w};
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

//NOTE: the spent part is cut off the top in whole atlas rows, so the picture and its black backing stay aligned
void SdlRenderUtils::DrawBadge(SDL_Renderer* const renderer, SDL_Texture* const atlas, const SDL_FRect& src,
							   const SDL_FRect& dst, const float spent)
{
	const SDL_FRect box{SnapToWindowPixels(renderer, dst)};
	const float scaleX{box.w / src.w};
	const float scaleY{box.h / src.h};
	const float cut{std::floor(spent * src.h)};

	constexpr float halfLine{0.5f};
	const float blackTop{std::max(cut, halfLine)};
	const SDL_FRect black{.x = box.x + halfLine * scaleX,
						  .y = box.y + blackTop * scaleY,
						  .w = (src.w - 2.f * halfLine) * scaleX,
						  .h = (src.h - halfLine - blackTop) * scaleY};
	SDL_SetRenderDrawColor(renderer, 0u, 0u, 0u, 255u);
	SDL_RenderFillRect(renderer, &black);

	const SDL_FRect restSrc{.x = src.x, .y = src.y + cut, .w = src.w, .h = src.h - cut};
	const SDL_FRect restDst{.x = box.x, .y = box.y + cut * scaleY, .w = box.w, .h = box.h - cut * scaleY};
	SDL_RenderTexture(renderer, atlas, &restSrc, &restDst);
}
