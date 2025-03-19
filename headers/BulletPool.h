#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

enum GameMode : char8_t;
enum Direction : char8_t;

struct ObjRectangle;

class Bullet;
class BaseObj;
class EventSystem;
struct Window;

class BulletPool final
{
	std::queue<std::shared_ptr<BaseObj>> _bullets;
	std::mutex _bulletsMutex;
	std::atomic<int> _lastId{0};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::string _name;
	GameMode _gameMode;
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;
	std::shared_ptr<Window> _window;
	bool _isClearing{false};

public:
	BulletPool(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	           std::shared_ptr<Window> window, GameMode gameMode);

	~BulletPool();

	void Subscribe();
	void Unsubscribe() const;

	void SpawnBullet(const ObjRectangle& rect, int damage, double aoeRadius, int color, int health, Direction direction,
	                 float speed, std::string author, std::string fraction, int tier);

	void ReturnBullet(BaseObj* bullet);

	void Clear();
};
