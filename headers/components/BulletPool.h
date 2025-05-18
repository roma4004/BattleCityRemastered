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

	std::shared_ptr<Bullet> CreateNewBullet(ObjRectangle rect, int damage, double aoeRadius, int color, int health,
	                                        Direction dir, float speed, std::string author, std::string fraction,
	                                        int tier);

	std::shared_ptr<BaseObj> SpawnBullet(ObjRectangle rect, int damage, double aoeRadius, int color, int health,
	                                     Direction dir, float speed, std::string author, std::string fraction,
	                                     int tier);

	void ReturnBullet(BaseObj* bullet);

	void Clear();

	static std::string GetCurrentTimeString();

};
