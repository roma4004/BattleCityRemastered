#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <string>

enum GameMode : char8_t;
enum Direction : char8_t;
struct ObjRectangle;
struct Window;
struct SDL_Renderer;
class Bullet;
class BaseObj;
class EventSystem;
class IDrawable;
class TextureManager;

class BulletPool final
{
	using milliseconds = std::chrono::milliseconds;

	std::queue<std::shared_ptr<BaseObj>> _bullets;
	std::mutex _bulletsMutex;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::string _name;
	GameMode _gameMode;
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;
	std::shared_ptr<Window> _window;
	bool _isClearing{false};
	std::shared_ptr<IDrawable> _textureManager;
	std::shared_ptr<SDL_Renderer> _renderer;

public:
	BulletPool(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	           std::shared_ptr<Window> window, GameMode gameMode, std::shared_ptr<IDrawable> textureManager,
	           std::shared_ptr<SDL_Renderer> renderer);

	~BulletPool();

	void Subscribe();
	void Unsubscribe() const;

	std::shared_ptr<Bullet> CreateNewBullet();

	std::shared_ptr<BaseObj> SpawnBullet();

	void ReturnBullet(BaseObj* bullet);

	void Clear();

	static std::string GetCurrentTimeString();

};
