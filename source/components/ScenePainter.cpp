#include "components/ScenePainter.h"
#include "components/EventSystem.h"
#include "components/Sprite.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/managers/TextureManager.h"
#include "entities/BaseObj.h"
#include "enums/DrawLayer.h"
#include "utils/ObjectUtils.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

ScenePainter::ScenePainter(const std::shared_ptr<EventSystem>& events,
						   const std::vector<std::shared_ptr<BaseObj>>& allObjects, const TextureManager& textures)
	: _events{events}
	, _allObjects{allObjects}
	, _textures{textures}
{
	Subscribe();
}

//NOTE: after AnimationManager in each phase - keeps the water under the walls and the blasts under the bush
void ScenePainter::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &ScenePainter::OnPreDraw));
	_subs.push_back(_events->AddListener(this, &ScenePainter::OnDraw));
	_subs.push_back(_events->AddListener(this, &ScenePainter::OnPostDraw));
}

void ScenePainter::OnPreDraw(const PreDrawEvent&)
{
	Read();
	Paint(DrawLayer::Ground);
}

void ScenePainter::OnDraw(const DrawEvent&) const { Paint(DrawLayer::World); }

void ScenePainter::OnPostDraw(const PostDrawEvent&) const
{
	Paint(DrawLayer::Overlay);
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
		if (const std::optional<int> health{object->ShownHealth()})
		{
			_healthBars.push_back(RenderHealthBarEvent{.rect = look->rect, .health = *health});
		}
	}
}

void ScenePainter::Paint(const DrawLayer layer) const
{
	std::ranges::for_each(_layers[static_cast<std::size_t>(layer)],
						  [this](const Sprite& sprite) { _textures.Draw(sprite); });
}
