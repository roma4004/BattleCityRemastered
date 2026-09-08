#pragma once

#include "geometry/Point.h"
#include "components/EventSystem.h"
#include "components/events/RenderUIEvents.h"
#include "components/managers/TextCache.h"
#include <SDL3/SDL_render.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
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
struct RenderTextEvent;
struct RenderMenuBackgroundEvent;
struct RenderMenuLogoEvent;
struct RenderMenuSelectorIconEvent;
struct RenderMenuXBoxHintEvent;
struct RenderMenuPS5HintEvent;
struct RenderPauseTextEvent;
struct RenderGameOverTextEvent;
struct RenderGameWonTextEvent;
struct RenderColorTextureEvent;
struct RenderTextureEvent;
struct RenderFPSEvent;
struct RenderHealthBarEvent;
struct RenderEnemyIconBackgroundEvent;
struct RenderEnemyIconsEvent;
struct RenderPlayerOneIconEvent;
struct RenderPlayerTwoIconEvent;
struct RenderStageNumberEvent;
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
		UPoint panelSize{};
		size_t padding{};

		void Init(const UPoint windowSize, const size_t sideBarWidth)
		{
			padding = 50;
			panelSize = UPoint{.x = windowSize.x - sideBarWidth - padding * 2,
							   .y = windowSize.y - padding * 2};
		}
	};

	MenuParams _menuParams{};

	//NOTE: kept between frames - the block itself is the key, so there is nothing to hash and
	//nothing to collide
	struct FittedBlock
	{
		RenderMenuTextBlockEvent block{};
		float scale{};
		int pointSize{};
	};

	mutable FittedBlock _menuBlockFit{};

	//NOTE: fitting only ever shrinks - every panel screen starts from the same size
	static constexpr int kBlockMinPointSize{8};

	SDL_Rect _fpsBox{};
	std::unordered_map<unsigned int, std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>> _colorTextureCache;

	static constexpr unsigned int kGrayColor{0x808080u};
	//NOTE: one column - fps box, enemy grid, counters and the flag share x and width
	static constexpr int kSideBarColumnPadding{55};
	static constexpr int kSideBarItemWidth{71};
	static constexpr int kSideBarColumnTop{60};
	static constexpr int kSideBarCounterTextPadding{38};
	//NOTE: opaque on purpose - the engine does not promote a transparent alpha
	static constexpr SDL_Color kSideBarCounterColor{.r = 0, .g = 0, .b = 2, .a = 255};

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
	void DrawEnemyIconBackground(const RenderEnemyIconBackgroundEvent&) const;
	void DrawEnemyIcons(const RenderEnemyIconsEvent& event) const;
	void DrawPlayerOneIcons(const RenderPlayerOneIconEvent& event) const;
	void DrawPlayerTwoIcons(const RenderPlayerTwoIconEvent& event) const;
	void DrawStageNumber(const RenderStageNumberEvent& event) const;

	[[nodiscard]] static unsigned int ColorToInt(const SDL_Color& color);
	[[nodiscard]] static SDL_Color IntToColor(unsigned int color);
	[[nodiscard]] static unsigned int ComponentsToColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	[[nodiscard]] static SDL_Rect RectToSdlRect(const ObjRectangle& rect);
	//NOTE: everything here is laid out in whole logical pixels; SDL3 wants floats only at the call
	[[nodiscard]] static SDL_FRect ToFRect(const SDL_Rect& rect);
	void FillRect(const SDL_Rect& rect) const;
	void SetRenderDrawColor(unsigned int color, Uint8 transparency = 255) const;

	void DrawMenuBackground(const RenderMenuBackgroundEvent& event) const;
	void DrawMenuLogo(const RenderMenuLogoEvent& event) const;
	void DrawSelectorIcon(const RenderMenuSelectorIconEvent& event) const;
	void RenderCopyWithClipping(SDL_Texture* texture, SDL_Rect srcRect, SDL_Rect dstRect) const;
	void RenderCopy(SDL_Texture* texture, SDL_Rect dstRect) const;
	void DrawXBoxHint(const RenderMenuXBoxHintEvent& event) const;
	void DrawPS5Hint(const RenderMenuPS5HintEvent& event) const;
	void OnRenderText(const RenderTextEvent& event) const;
	void DrawMenuTextBlock(const RenderMenuTextBlockEvent& event) const;
	[[nodiscard]] SDL_Rect MenuPanelRect(Point menuPos) const;
	[[nodiscard]] int FitBlockPointSize(const RenderMenuTextBlockEvent& event, float scale) const;
	[[nodiscard]] float CurrentRenderScale() const;
	[[nodiscard]] static int BasePointSize(bool isMediumFontSize);
	void TextToRender(const Point& pos, const SDL_Color& color, int value, bool isMediumFontSize) const;
	void TextToRender(Point pos, SDL_Color color, const std::string& text, bool isMediumFontSize = false) const;
	void TextToRenderSized(Point pos, SDL_Color color, const std::string& text, int basePointSize) const;
	//NOTE: keeps the proportions and the given size - a line wider than the box is not shrunk, it runs over
	void TextToRenderCentered(const SDL_Rect& box, SDL_Color color, const std::string& text,
							  int basePointSize) const;
	void DrawText(const TextCache::CachedText& cached, int x, int y, float scale) const;

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

	//NOTE: the band above the enemy icon background - the counter is centred in it
	[[nodiscard]] static SDL_Rect CalcFpsBox(const UPoint& battlefieldSize);
	[[nodiscard]] int SideBarColumnX() const;

public:
	RenderManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig, SDL_Config& sdlConfig);
};
