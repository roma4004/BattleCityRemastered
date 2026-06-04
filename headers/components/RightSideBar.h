#pragma once
#include "Point.h"

#include <SDL_render.h>
struct ObjRectangle;
class EventSystem;
class RenderManager;

class RightSideBar
{
	Point _pos;
	UPoint _windowSize;
	bool _isRightSideBarEnabled{false};
	bool _isEnemyIconEnabled{false};
	int _enemiesRespawnCount{};
	int _playerOneRespawnCount{};
	int _playerTwoRespawnCount{};

	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<SDL_Renderer> _renderer{nullptr};

	void SendEnemyIconsData() const;
	void Subscribe();
	void Unsubscribe();
	ObjRectangle GetRect() const;

public:
	RightSideBar(UPoint windowSize, const std::shared_ptr<EventSystem>& events);
	~RightSideBar();
};
