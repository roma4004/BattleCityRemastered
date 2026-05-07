#pragma once
#include "Point.h"
#include <SDL_rect.h>
#include <SDL_render.h>
class EventSystem;
class RenderManager;

class RightSideBar
{
	Point _pos;
	UPoint windowSize;
	bool _isRightSideBarEnabled;
	bool _isEnemyIconEnabled;
	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<SDL_Texture> _atlasTexture{nullptr};

	const int numPictures = 20;
	const int columns = 2;
	const int rows = numPictures / columns;
	const int distanceBetweenColumns = 50;
	const int verticalDistanceBetweenDecals = 200;
	const SDL_Rect rect{.x = 650, .y = 60, .w = 20, .h = 40}; //positioning inside window
	int imageWidth{rect.w}, imageHeight{rect.h};

	int spacingX = (distanceBetweenColumns - (columns * imageWidth)) / (columns + 1);
	int spacingY = (verticalDistanceBetweenDecals - (rows * imageHeight)) / (rows + 1);
	const int leftUpCornerX = 685;
	const int leftUpCornerY = 95;
	int x{};
	int y{};

	void Subscribe();
	void Unsubscribe() const;

public:
	RightSideBar(UPoint windowSize, const std::shared_ptr<EventSystem>& events,
	const std::shared_ptr<SDL_Renderer>& renderer, const std::shared_ptr<SDL_Texture>& atlasTexture);
	~RightSideBar();
};
