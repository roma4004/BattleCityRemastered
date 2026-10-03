#pragma once

#include "components/EventSystem.h"
#include "components/Sprite.h"
#include "components/events/AnimationRenderEvents.h"
#include "enums/DrawLayer.h"
#include <array>
#include <memory>
#include <vector>

struct PreDrawEvent;
struct DrawEvent;
struct PostDrawEvent;
class BaseObj;
class EventSystem;
class TextureManager;

//NOTE: reads the world once a frame and paints it layer by layer - nothing is pushed here to go stale
class ScenePainter final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	const std::vector<std::shared_ptr<BaseObj>>& _world;
	const TextureManager& _textures;
	//NOTE: by DrawLayer, refilled every frame - kept only for the capacity
	std::array<std::vector<Sprite>, kDrawLayerCount> _layers{};
	std::vector<RenderHealthBarEvent> _healthBars{};

	void Subscribe();
	void OnPreDraw(const PreDrawEvent&);
	void OnDraw(const DrawEvent&) const;
	void OnPostDraw(const PostDrawEvent&) const;

	void Read();
	void Paint(DrawLayer layer) const;

public:
	ScenePainter(const std::shared_ptr<EventSystem>& events, const std::vector<std::shared_ptr<BaseObj>>& world,
				 const TextureManager& textures);
};
