#include "components/SideBarCanvas.h"
#include "application/GameConfig.h"
#include "application/SDL_Config.h"
#include "components/WorldGeometry.h"
#include "enums/TextureOffset.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include "utils/SdlRenderUtils.h"
#include <algorithm>
#include <cmath>
#include <optional>
#include <SDL3/SDL_rect.h>

namespace
{
constexpr int kFpsBoxHeight{60};
}//namespace

SideBarCanvas::SideBarCanvas(const SDL_Config& sdlConfig, const GameConfig& gameConfig)
	: _sdlConfig{sdlConfig}
	, _gameConfig{gameConfig}
{
}

int SideBarCanvas::Left() const { return static_cast<int>(_gameConfig.battlefieldSize.x); }

int SideBarCanvas::ColumnX() const
{
	const int frameWidth{Pixels(TextureOffset::kEnemyIconBackground.w)};

	return Left() + (static_cast<int>(WorldGeometry::kSideBarWidth) - frameWidth) / 2;
}

SDL_Rect SideBarCanvas::FpsBox() const
{
	return SDL_Rect{.x = ColumnX(), .y = 0, .w = Pixels(TextureOffset::kEnemyIconBackground.w), .h = kFpsBoxHeight};
}

//NOTE: edges rounded apart - adjacent cells meet with no gap
SDL_Rect SideBarCanvas::Rect(const ObjRectangle& atlas) const
{
	const ObjRectangle& frame{TextureOffset::kEnemyIconBackground};
	const int left{ColumnX() + Pixels(atlas.x - frame.x)};
	const int top{kFpsBoxHeight + Pixels(atlas.y - frame.y)};

	return SDL_Rect{.x = left,
					.y = top,
					.w = ColumnX() + Pixels(atlas.x + atlas.w - frame.x) - left,
					.h = kFpsBoxHeight + Pixels(atlas.y + atlas.h - frame.y) - top};
}

void SideBarCanvas::Draw(const ObjRectangle& atlas, const SDL_Rect& place) const
{
	SdlRenderUtils::RenderCopyWithClipping(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(),
										   SdlRenderUtils::RectToSdlRect(atlas), place);
}

void SideBarCanvas::DrawBadge(const ObjRectangle& picture, const std::optional<float> spent,
							  const SDL_Rect& place) const
{
	if (!spent)
	{
		return;
	}

	const SDL_FRect box{.x = static_cast<float>(picture.x),
						.y = static_cast<float>(picture.y),
						.w = static_cast<float>(TextureOffset::kBonusBoxWidth),
						.h = static_cast<float>(TextureOffset::kBonusBoxHeight)};
	const SDL_FRect room{SdlRenderUtils::ToFRect(place)};
	const float aspect{box.w / box.h};
	const float width{std::min(room.w, room.h * aspect)};
	const float height{width / aspect};
	const SDL_FRect fitted{.x = room.x + (room.w - width) / 2.f,
						   .y = room.y + (room.h - height) / 2.f,
						   .w = width,
						   .h = height};
	SdlRenderUtils::DrawBadge(_sdlConfig.renderer.get(), _sdlConfig.atlasTexture.get(), box, fitted, *spent);
}

int SideBarCanvas::Pixels(const double atlasPixels) { return static_cast<int>(std::lround(atlasPixels * kScale)); }

ObjRectangle SideBarCanvas::CellOff(const ObjRectangle& picture, const Point offset)
{
	return ObjRectangle{.x = picture.x + offset.x, .y = picture.y + offset.y, .w = kCell, .h = kCell};
}

//NOTE: one whole-pixel inset for both sides keeps the picture centered
SDL_Rect SideBarCanvas::Inset(const SDL_Rect& place, const double atlasAcross, const double atlasDown)
{
	const int across{Pixels(atlasAcross)};
	const int down{Pixels(atlasDown)};

	return SDL_Rect{.x = place.x + across, .y = place.y + down, .w = place.w - across * 2, .h = place.h - down * 2};
}

//NOTE: a glyph inks 7 of its 8 dots - half a dot more centers it in the cell
ObjRectangle SideBarCanvas::GlyphCells(const ObjRectangle& cells)
{
	constexpr double halfDot{0.5};

	return ObjRectangle{.x = cells.x + halfDot, .y = cells.y + halfDot, .w = cells.w, .h = cells.h};
}
