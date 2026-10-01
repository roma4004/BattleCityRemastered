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
#include <memory>
#include <optional>
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
struct RenderPauseTextEvent;
struct RenderGameOverTextEvent;
struct RenderGameWonTextEvent;
struct RenderColorTextureEvent;
struct RenderTextureEvent;
struct RenderFPSEvent;
struct RenderHealthBarEvent;
struct RenderSideBarEvent;
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

		UPoint panelSize{};
		size_t padding{};
		size_t sideInset{};

		void Init(const UPoint windowSize, const size_t sideBarWidth)
		{
			padding = 50;
			sideInset = static_cast<size_t>(WorldGeometry::kCellSize) * kSideInsetCells;
			//NOTE: as far off the bottom edge as off the sides - the panel reads as one frame
			panelSize = UPoint{.x = windowSize.x - sideBarWidth - sideInset * 2,
							   .y = windowSize.y - padding - sideInset};
		}
	};

	MenuParams _menuParams{};

	//NOTE: what a block needs across at one point size - where its leftmost line starts, how far the
	//line reaching furthest gets from there, and the height of the tallest of them
	struct BlockSpan
	{
		int left{};
		int width{};
		int tallestLine{};
	};

	//NOTE: kept between frames - everything the fitted size rests on, and menuPos is not part of it:
	//the slide reaches the panel only through its y, which neither the width nor the line step reads
	struct FittedBlock
	{
		std::vector<TextBlockLine> lines{};
		int lineHeight{};
		TextBlockAlign align{};
		float scale{};
		int pointSize{};
		int shiftX{};

		[[nodiscard]] bool Matches(const RenderMenuTextBlockEvent& event, float renderScale) const;
	};

	mutable FittedBlock _menuBlockFit{};

	//NOTE: fitting only ever shrinks - every panel screen starts from the same size
	static constexpr int kBlockMinPointSize{8};

	//NOTE: the menu's own places inside the panel - the tables stand where the hand-placed lines used to
	static constexpr int kMenuLogoTop{17};
	static constexpr int kMenuLogoWidth{300};
	static constexpr int kMenuLogoHeight{75};
	static constexpr int kMenuModesTop{120};
	//NOTE: the controls hang off the bottom of the panel, so the room under them stays the same
	static constexpr int kMenuControlsBottomGap{25};
	static constexpr int kMenuRowHeight{30};
	static constexpr int kMenuIconSize{30};
	//NOTE: room for the arrow to the left of the mode it points at, and the slack a click still lands in
	static constexpr int kMenuSelectorGap{35};
	static constexpr int kMenuRowPadding{5};

	//NOTE: kept between frames - the fit rests on the scale alone, the menu's words never change
	mutable int _menuPointSize{};
	mutable float _menuScale{};
	mutable std::vector<Point> _menuTilePlaces{};

	SDL_Rect _fpsBox{};
	std::unordered_map<unsigned int, std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>> _colorTextureCache;

	static constexpr unsigned int kGrayColor{0x808080u};
	//NOTE: one column - fps box, enemy grid, counters and the flag share x and width
	static constexpr int kSideBarColumnPadding{55};
	static constexpr int kSideBarItemWidth{71};
	static constexpr int kSideBarColumnTop{60};
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

	void DrawPauseText(const RenderPauseTextEvent&) const;
	void DrawGameOverText(const RenderGameOverTextEvent&) const;
	void DrawGameWonText(const RenderGameWonTextEvent&) const;
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
	[[nodiscard]] int FitMenuPointSize(const RenderMenuEvent& event, const SDL_Rect& panel, float scale) const;
	[[nodiscard]] UiLayout::Placement PlaceCentered(const UiTable& table, const SDL_Rect& panel, int top,
													const UiLayout::Measure& measure) const;
	void DrawTablePictures(const UiTable& table, const UiLayout::Placement& placement) const;
	void DrawTableText(const UiTable& table, const UiLayout::Placement& placement, int pointSize, float scale) const;
	void AnnounceMenuTiles(const UiLayout::Placement& modes) const;
	void DrawMenuTextBlock(const RenderMenuTextBlockEvent& event) const;
	[[nodiscard]] SDL_Rect MenuPanelRect(Point menuPos) const;
	//NOTE: the block is never empty here - the draw returns before it reaches this
	[[nodiscard]] BlockSpan MeasureBlock(const RenderMenuTextBlockEvent& event, int pointSize, float scale) const;
	[[nodiscard]] int FitBlockPointSize(const RenderMenuTextBlockEvent& event, float scale) const;
	[[nodiscard]] int MenuBlockShiftX(const RenderMenuTextBlockEvent& event, int pointSize, float scale) const;
	//NOTE: the menu lays its content out around the text, so the icons and the logo take the shift the
	//block was last drawn with - the menu emits the block ahead of them for exactly that
	[[nodiscard]] Point MenuContentPos(Point pos) const;
	[[nodiscard]] float CurrentRenderScale() const;
	//NOTE: the largest a menu block ever starts from - fitting only ever shrinks from here
	[[nodiscard]] static int BlockStartPointSize();
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
	[[nodiscard]] SDL_Rect PlateRect(const ObjRectangle& sprite, double widthShare) const;

	static constexpr double kPausePlateShare{0.40};
	static constexpr double kGameOverPlateShare{0.27};
	static constexpr double kGameWonPlateShare{0.16};

	[[nodiscard]] static SDL_Rect CalcFpsBox(const UPoint& battlefieldSize);
	[[nodiscard]] int SideBarColumnX() const;

public:
	RenderManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig, SDL_Config& sdlConfig);
};
