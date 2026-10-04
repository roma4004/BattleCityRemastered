#pragma once

#include "geometry/Point.h"
#include "components/EventSystem.h"
#include "components/UiLayout.h"
#include "components/UiTable.h"
#include "components/managers/TextCache.h"
#include "enums/UiIcon.h"
#include <SDL3/SDL_render.h>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

struct ObjRectangle;
struct SDL_Config;
struct RenderMenuBackgroundEvent;
struct RenderMenuEvent;
struct RenderPlateEvent;
struct RenderFPSEvent;
struct RenderSideBarEvent;
struct RenderPanelTablesEvent;
struct PanelScroll;
struct PanelCaret;
struct PanelDropDown;
struct RenderDeviceResetEvent;
class GameConfig;

//NOTE: the screens over the world - menu, scoreboard, lobby, side bar, plates; the frame is RenderManager's
class UiRenderer final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	const GameConfig& _gameConfig;
	const SDL_Config& _sdlConfig;

	//NOTE: kept between frames - a fit measures every word at every size; other words, scale or panel refit
	struct PanelFit final
	{
		std::vector<UiTable> tables{};
		float scale{};
		Point panelSize{};
		int pointSize{};

		[[nodiscard]] int PointSize(std::span<const UiTable> words, const SDL_Rect& panel, float renderScale,
									const std::function<bool(int)>& goesIn);
	};

	mutable PanelFit _menuFit{};
	mutable PanelFit _panelFit{};
	mutable std::vector<Point> _menuTilePlaces{};
	mutable std::vector<Point> _panelRowPlaces{};
	mutable Point _panelRowSize{};
	mutable int _panelSymbolWidth{};

	//NOTE: no sprite means the whole texture
	struct IconSource final
	{
		SDL_Texture* texture{nullptr};
		std::optional<SDL_Rect> sprite{};
		Point size{};
	};

	//NOTE: a table and where it was placed - DrawTables walks these, or a zip of the two
	using PlacedTable = std::pair<const UiTable&, const UiLayout::Placement&>;

	mutable TextCache _textCache;

	void Subscribe();
	void OnRenderDeviceReset(const RenderDeviceResetEvent&);

	void DrawMenuBackground(const RenderMenuBackgroundEvent& event) const;
	void DrawMenu(const RenderMenuEvent& event) const;
	void DrawPanelTables(const RenderPanelTablesEvent& event) const;
	void DrawSideBar(const RenderSideBarEvent& event) const;
	void RenderFPS(const RenderFPSEvent& event) const;
	void DrawPlate(const RenderPlateEvent& event) const;

	//NOTE: one switch with no default - an icon left out is a compiler warning, not an empty square
	[[nodiscard]] IconSource Icon(UiIcon icon) const;
	void DrawIcon(UiIcon icon, SDL_Rect dstRect) const;
	//NOTE: what a table needs to become places: a word is as wide as the font makes it, a picture as Icon says
	[[nodiscard]] UiLayout::Measure CellMeasurer(int pointSize, float scale) const;
	//NOTE: every picture first, then every word under one render scale - each change of it breaks the batch
	void DrawTables(const auto& placedTables, int pointSize, float scale) const;
	void DrawTablePictures(const UiTable& table, const UiLayout::Placement& placement) const;
	//NOTE: the render scale is the caller's - a run of lines sets it once, see ScopedRenderScale
	void DrawTableText(const UiTable& table, const UiLayout::Placement& placement, int pointSize, float scale) const;
	void AnnounceMenuTiles(const UiLayout::Placement& modes) const;
	void AnnouncePanelRows(const std::vector<Point>& rows, Point rowSize, int symbolWidth) const;
	void DrawScrollBar(const UiLayout::Placement& picked, const PanelScroll& scroll, int rowHeight) const;
	void DrawCaret(const UiTable& table, const UiLayout::Placement& placement, const PanelCaret& caret, int pointSize,
				   float scale) const;
	void DrawDropDown(const UiLayout::Placement& picked, const PanelDropDown& dropDown, int pointSize,
					  float scale) const;
	[[nodiscard]] SDL_Rect MenuPanelRect(int slide) const;
	[[nodiscard]] float CurrentRenderScale() const;

	//NOTE: the map decides the field, so a plate is placed against it - fixed numbers were the middle
	//of the first map and stayed where they were when it grew
	[[nodiscard]] SDL_Rect CenteredInField(int width, int height, double middleShare) const;
	//NOTE: the field says how wide the plate is, the sprite says what shape - so a bigger map moves it
	//and grows it without stretching the picture
	[[nodiscard]] Point PlateSize(const ObjRectangle& sprite, double widthShare) const;
	[[nodiscard]] int SideBarColumnX() const;

public:
	UiRenderer(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig, const SDL_Config& sdlConfig);
};
