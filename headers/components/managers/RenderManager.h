#pragma once

#include "components/EventSystem.h"
#include <SDL3/SDL_render.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

enum class GameMode : char8_t;
enum class PlayerSlot : std::uint8_t;
struct SDL_Config;
struct PreTickUpdateEvent;
struct PresentFrameEvent;
struct GameModeChangedToEvent;
struct PlayerSlotAssignedEvent;
struct RenderColorTextureEvent;
struct RenderTextureEvent;
struct RenderHealthBarEvent;
struct WorldGeometryChangedEvent;
struct WindowSizeChangedToEvent;
struct RenderTargetsResetEvent;
struct RenderDeviceResetEvent;
class GameConfig;

class RenderManager final
{

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	const GameConfig& _gameConfig;
	SDL_Config& _sdlConfig;

	std::unordered_map<unsigned int, std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>> _colorTextureCache;

	static constexpr unsigned int kGrayColor{0x808080u};

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

	void SetRenderDrawColor(unsigned int color, Uint8 transparency = 255) const;

	void ClearFrame(const PreTickUpdateEvent&) const;
	void PresentFrame(const PresentFrameEvent&) const;
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);
	void OnPlayerSlotAssigned(const PlayerSlotAssignedEvent& event);
	void UpdateWindowTitle() const;

	void CreateColorTexture(unsigned int color);
	void DrawColorTexture(const RenderColorTextureEvent& event);
	void DrawTexture(const RenderTextureEvent& event) const;

	void DrawHealthBar(const RenderHealthBarEvent& event) const;

public:
	RenderManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig, SDL_Config& sdlConfig);
};
