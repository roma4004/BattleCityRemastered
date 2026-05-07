#include "Components/RightSideBar.h"
#include "Components/EventSystem.h"
#include "enums/TextureOffset.h"

#include <SDL_render.h>

RightSideBar::RightSideBar(UPoint windowSize, const std::shared_ptr<EventSystem>& events,
	const std::shared_ptr<SDL_Renderer>& renderer, const std::shared_ptr<SDL_Texture>& atlasTexture)
	: windowSize{windowSize}
	, _events{events}
	, _renderer{renderer}
	, _atlasTexture{atlasTexture}
	, _name{std::string("RightSideBar")}
{
	Subscribe();
	_events->EmitEvent("EnableRightSideBar");
	_events->EmitEvent("EnableEnemyIcon");
	_events->EmitEvent("RenderRightSideBar");
	_events->EmitEvent("DrawEnemyIcons");
}

void RightSideBar::Subscribe()
{
	_events->AddListener(
		"EnableEnemyIcon", _name,
		[this]()
		{
			_isEnemyIconEnabled = true;
		});

	_events->AddListener(
			"DisableEnemyIcon", _name,
			[this]()
			{
				_isEnemyIconEnabled = false;
			});

	_events->AddListener("DrawEnemyIcons", _name, [this]()
	{
		constexpr TextureOffset offset{};
		SDL_Rect srcrect{
				static_cast<int>(offset.enemyDecal.x),
				static_cast<int>(offset.enemyDecal.y),
				static_cast<int>(offset.enemyDecal.w),
				static_cast<int>(offset.enemyDecal.h)};

		for (int i = 0; i < numPictures; ++i)
		{
			int row = i / columns;
			int col = i % columns;
			x = leftUpCornerX + spacingX + col * (imageWidth + spacingX);
			y = leftUpCornerY + spacingY + row * (imageHeight + spacingY);

			SDL_Rect destRect = {x, y, imageWidth, imageHeight};
			SDL_RenderCopy(_renderer.get(), _atlasTexture.get(), &srcrect, &destRect);
		}
		
	});
}

void RightSideBar::Unsubscribe() const
{
	_events->RemoveListener("EnableEnemyIcon", _name);
	_events->RemoveListener("DisableEnemyIcon", _name);
	_events->RemoveListener("DrawEnemyIcons", _name);
}

RightSideBar::~RightSideBar()
{
	_events->EmitEvent("DisableRightSideBar");
	Unsubscribe();
}


//TODO: Глянуть как в менюхе реализован покадровый апдэйт и запилить тут его аналог