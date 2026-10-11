#include "components/ScenePainter.h"
#include "components/AnimatedObjects.h"
#include "components/EventSystem.h"
#include "components/Sprite.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/managers/AnimationManager.h"
#include "components/managers/TextureManager.h"
#include "entities/BaseObj.h"
#include "enums/DrawLayer.h"
#include "utils/ObjectUtils.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <ranges>
#include <vector>

ScenePainter::ScenePainter(const std::shared_ptr<EventSystem>& events,
						   const std::vector<std::shared_ptr<BaseObj>>& allObjects, const AnimationManager& animations,
						   const TextureManager& textures)
	: _events{events}
	, _allObjects{allObjects}
	, _animations{animations}
	, _textures{textures} {}

//NOTE: the health bars last - over the bush too, as the tank they belong to is not in one
void ScenePainter::Paint()
{
	Read();
	std::ranges::for_each(std::views::iota(std::size_t{}, kDrawLayerCount), [this](const std::size_t layer)
	{
		Paint(static_cast<DrawLayer>(layer));
	});
	std::ranges::for_each(_healthBars, [this](const RenderHealthBarEvent& bar) { _events->EmitEvent(bar); });
}

//NOTE: one walk for all layers, not one per layer
void ScenePainter::Read()
{
	std::ranges::for_each(_layers, [](std::vector<Sprite>& layer) { layer.clear(); });
	_healthBars.clear();
	for (const std::shared_ptr<BaseObj>& object: _allObjects)
	{
		if (!ObjectUtils::IsAlive(object.get()))
		{
			continue;
		}

		const std::optional<Sprite> look{object->Look()};
		if (!look)
		{
			continue;
		}

		_layers[static_cast<std::size_t>(look->layer)].push_back(*look);
		if (const std::optional<Sprite> cover{object->Cover()})
		{
			_layers[static_cast<std::size_t>(cover->layer)].push_back(*cover);
		}

		if (const std::optional<int> health{object->ShownHealth()})
		{
			_healthBars.push_back(RenderHealthBarEvent{.rect = look->rect, .health = *health});
		}
	}
}

//TODO: bake the level's bricks into one image, blacking out a brick's spot as it dies - one draw, not one per quarter
//NOTE: a layer's animations under its sprites - the water beside the ice, the bursts beside the helmets
void ScenePainter::Paint(const DrawLayer layer) const
{
	const auto isPlaying = [](const AnimatedObject& animation) { return !animation.markToDispose; };
	std::ranges::for_each(_animations.OnLayer(layer) | std::views::filter(isPlaying),
						  [this](const AnimatedObject& animation) { _textures.Draw(animation); });
	std::ranges::for_each(_layers[static_cast<std::size_t>(layer)],
						  [this](const Sprite& sprite) { _textures.Draw(sprite); });
}
