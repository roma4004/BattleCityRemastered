#pragma once

#include "geometry/Point.h"
#include "components/EventSystem.h"
#include "components/WorldGeometry.h"
#include "components/events/RenderUIEvents.h"
#include "components/UiLayout.h"
#include "components/managers/TextCache.h"
#include <SDL3/SDL_render.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

enum class Direction : char8_t;
enum class GameMode : char8_t;
enum class PlayerSlot : std::uint8_t;
struct ObjRectangle;
struct SDL_Config;
struct PreTickUpdateEvent;
struct PresentFrameEvent;
struct GameModeChangedToEvent;
struct PlayerSlotAssignedEvent;
struct RenderMenuBackgroundEvent;
struct RenderMenuEvent;
struct RenderPlateEvent;
struct RenderColorTextureEvent;
struct RenderTextureEvent;
struct RenderFPSEvent;
struct RenderHealthBarEvent;
struct RenderSideBarEvent;
struct RenderPanelTablesEvent;
struct WorldGeometryChangedEvent;
struct WindowSizeChangedToEvent;
struct RenderTargetsResetEvent;
struct RenderDeviceResetEvent;
class EventSystem;
class GameConfig;

class RenderManager final
{

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	const GameConfig& _gameConfig;
	SDL_Config& _sdlConfig;

	struct MenuParams
	{
		//NOTE: whole cells, so the panel's sides land on the outermost brick blocks - the number has to
		//match the border the .map files leave around the pattern
		static constexpr size_t kSideInsetCells{5u};
		static constexpr size_t kTopInset{50u};

		UPoint panelSize{};
		size_t sideInset{};

		void Init(const UPoint windowSize, const size_t sideBarWidth)
		{
			sideInset = static_cast<size_t>(WorldGeometry::kCellSize) * kSideInsetCells;
			//NOTE: as far off the bottom edge as off the sides - the panel reads as one frame
			panelSize = UPoint{.x = windowSize.x - sideBarWidth - sideInset * 2,
							   .y = windowSize.y - kTopInset - sideInset};
		}
	};

	MenuParams _menuParams{};

	//NOTE: fitting only ever shrinks - every panel screen starts from the same size
	static constexpr int kFitMinPointSize{8};
	//NOTE: the scoreboard and the lobby at the full point size - a row, and the gap between tables, shrink with it
	static constexpr int kPanelRowHeight{20};
	//NOTE: the widest table still leaves this much of the panel on each side
	static constexpr int kPanelSideMargin{20};

	//NOTE: the menu's own places inside the panel
	static constexpr int kMenuTitleTop{17};
	static constexpr Point kMenuLogoSize{.x = 300, .y = 75};
	//NOTE: the modes start this far under the logo
	static constexpr int kMenuTitleGap{28};
	//NOTE: the controls hang off the bottom of the panel, so the room under them stays the same
	static constexpr int kMenuControlsBottomGap{25};
	static constexpr int kMenuRowHeight{30};
	static constexpr int kMenuIconSize{30};
	//NOTE: room for the arrow to the left of the mode it points at, and the slack a click still lands in
	static constexpr int kMenuSelectorGap{35};
	static constexpr int kMenuRowPadding{5};

	//NOTE: kept between frames - the menu's words never change, a new scale refits them and InitMenu forgets it
	mutable int _menuPointSize{};
	mutable float _menuScale{};

	//NOTE: kept between frames - a scoreboard number changes a few times a match, not every frame
	struct PanelFit final
	{
		std::vector<UiTable> tables{};
		float scale{};
		Point panelSize{};
		int pointSize{};
	};

	mutable PanelFit _panelFit{};
	mutable std::vector<Point> _menuTilePlaces{};

	SDL_Rect _fpsBox{};
	std::unordered_map<unsigned int, std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>> _colorTextureCache;

	static constexpr unsigned int kGrayColor{0x808080u};
	//NOTE: one column - fps box, enemy grid, counters and the flag share x and width
	static constexpr int kSideBarColumnPadding{55};
	static constexpr int kSideBarItemWidth{71};
	static constexpr int kFpsBoxHeight{60};
	static constexpr int kReserveFrameHeight{277};
	static constexpr int kReserveGapBelow{13};
	//NOTE: 4, not 5 - the 25 px tank is centered in its 27 px row
	static constexpr Point kReserveInset{.x = 5, .y = 4};
	static constexpr int kReserveRowHeight{27};
	static constexpr int kReserveColumnGap{1};
	static constexpr Point kSideBarEnemyTankSize{.x = 30, .y = 25};
	static constexpr Point kSideBarLivesSize{.x = kSideBarItemWidth, .y = 70};
	static constexpr Point kSideBarStageSize{.x = kSideBarItemWidth, .y = 95};

	//NOTE: no sprite means the whole texture
	struct IconSource final
	{
		SDL_Texture* texture{nullptr};
		std::optional<SDL_Rect> sprite{};
		Point size{};
	};

	mutable TextCache _textCache;

	//NOTE: both halves of the caption - the mode arrives on entering a match, the seat only once
	//the server has answered, so the title is rebuilt rather than written in one go
	GameMode _titleMode{};
	std::optional<PlayerSlot> _titleSlot{};

	void Subscribe();
	void OnWorldGeometryChanged(const WorldGeometryChangedEvent&);
	void OnWindowSizeChangedTo(const WindowSizeChangedToEvent&);
	//NOTE: a letterbox only ever fills what the window has and the field does not - give the window the
	//field's own proportions and there is nothing left to fill
	void SnapWindowToLogicalAspect() const;
	void OnRenderTargetsReset(const RenderTargetsResetEvent&);
	void OnRenderDeviceReset(const RenderDeviceResetEvent&);
	void ApplyLogicalSize();

	void DrawPlate(const RenderPlateEvent& event) const;
	void DrawSideBar(const RenderSideBarEvent& event) const;

	[[nodiscard]] static unsigned int ColorToInt(const SDL_Color& color);
	[[nodiscard]] static SDL_Color IntToColor(unsigned int color);
	[[nodiscard]] static unsigned int ComponentsToColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	[[nodiscard]] static SDL_Rect RectToSdlRect(const ObjRectangle& rect);
	[[nodiscard]] static SDL_Rect RectOf(Point pos, Point size);
	//NOTE: everything here is laid out in whole logical pixels; SDL3 wants floats only at the call
	[[nodiscard]] static SDL_FRect ToFRect(const SDL_Rect& rect);
	void FillRect(const SDL_Rect& rect) const;
	void SetRenderDrawColor(unsigned int color, Uint8 transparency = 255) const;

	void DrawMenuBackground(const RenderMenuBackgroundEvent& event) const;
	void RenderCopyWithClipping(SDL_Texture* texture, SDL_Rect srcRect, SDL_Rect dstRect) const;
	void RenderCopy(SDL_Texture* texture, SDL_Rect dstRect) const;
	void DrawMenu(const RenderMenuEvent& event) const;
	//NOTE: one switch with no default - an icon left out is a compiler warning, not an empty square
	[[nodiscard]] IconSource Icon(UiIcon icon) const;
	void DrawIcon(UiIcon icon, SDL_Rect dstRect) const;
	//NOTE: what a table needs to become places: a word is as wide as the font makes it, a picture as Icon says
	[[nodiscard]] UiLayout::Measure CellMeasurer(int pointSize, float scale) const;
	//NOTE: one size for every table of a screen - tables shrunk apart would read as different screens
	[[nodiscard]] static int FitPointSize(const std::function<bool(int)>& goesIn);
	[[nodiscard]] static std::vector<UiLayout::Placement> MeasureAll(std::span<const UiTable> tables, int rowHeight,
																	 const UiLayout::Measure& measure);
	//NOTE: only the words answer to the font - a picture is not asked, nor a table of nothing but pictures
	[[nodiscard]] static bool FitsAcross(std::span<const UiTable> tables, std::span<const UiLayout::Placement> placed,
										 int width, int rowHeight);
	[[nodiscard]] static UiLayout::Placement CenteredAcross(UiLayout::Placement placement, const SDL_Rect& panel,
															int top);
	[[nodiscard]] int PanelPointSize(const std::vector<UiTable>& tables, const SDL_Rect& panel, float scale) const;
	[[nodiscard]] static int PanelRowHeight(int pointSize);
	[[nodiscard]] static int StackHeight(std::span<const UiLayout::Placement> placed, int gap);
	void DrawTablePictures(const UiTable& table, const UiLayout::Placement& placement) const;
	void DrawTableText(const UiTable& table, const UiLayout::Placement& placement, int pointSize, float scale) const;
	void AnnounceMenuTiles(const UiLayout::Placement& modes) const;
	void DrawPanelTables(const RenderPanelTablesEvent& event) const;
	[[nodiscard]] SDL_Rect MenuPanelRect(int slide) const;
	[[nodiscard]] float CurrentRenderScale() const;
	//NOTE: the largest a panel screen ever starts from - fitting only ever shrinks from here
	[[nodiscard]] static int FitStartPointSize();
	void DrawTextAt(Point pos, SDL_Color color, std::string_view text, int basePointSize, float scale) const;
	//NOTE: keeps the proportions and the given size - a line wider than the box is not shrunk, it runs over
	void DrawTextCentered(const SDL_Rect& box, SDL_Color color, std::string_view text, int basePointSize,
						  float scale) const;
	//NOTE: the render scale is the caller's - a run of lines sets it once, see ScopedRenderScale
	void DrawText(const TextCache::CachedText& cached, int x, int y, float scale) const;
	[[nodiscard]] static Point CenteredIn(const SDL_Rect& box, const TextCache::CachedText& cached);

	void ClearFrame(const PreTickUpdateEvent&) const;
	void PresentFrame(const PresentFrameEvent&) const;
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);
	void OnPlayerSlotAssigned(const PlayerSlotAssignedEvent& event);
	void UpdateWindowTitle() const;

	void CreateColorTexture(unsigned int color);
	[[nodiscard]] static std::pair<double, SDL_FlipMode> GetRotateAndAngleAndFlip(Direction dir);
	void DrawColorTexture(const RenderColorTextureEvent& event);
	void DrawTexture(const RenderTextureEvent& event) const;

	void RenderFPS(const RenderFPSEvent& event) const;

	void DrawHealthBar(const RenderHealthBarEvent& event) const;
	void InitMenu(const GameConfig& gameConfig);

	//NOTE: the map decides the field, so a plate is placed against it - fixed numbers were the middle
	//of the first map and stayed where they were when it grew
	[[nodiscard]] SDL_Rect CenteredInField(int width, int height) const;
	//NOTE: the field says how wide the plate is, the sprite says what shape - so a bigger map moves it
	//and grows it without stretching the picture
	[[nodiscard]] Point PlateSize(const ObjRectangle& sprite, double widthShare) const;

	static constexpr double kPausePlateShare{0.40};
	static constexpr double kGameOverPlateShare{0.27};
	static constexpr double kGameWonPlateShare{0.16};

	[[nodiscard]] static SDL_Rect CalcFpsBox(const UPoint& battlefieldSize);
	[[nodiscard]] int SideBarColumnX() const;

public:
	RenderManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig, SDL_Config& sdlConfig);
};
