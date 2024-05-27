#pragma once
#include "../headers/BaseObj.h"
#include "../headers/Direction.h"
#include "../headers/EventSystem.h"
#include "../headers/IMovable.h"
#include "../headers/PlayerKeys.h"
#include "../headers/Point.h"

#include <list>
#include <memory>

class Pawn : public BaseObj, public IMovable
{
public:
	Pawn(const Rectangle& rect, int color, float speed, int health, int* windowBuffer, size_t windowWidth,
		 size_t windowHeight, std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events);

	~Pawn() override;

	void SetPixel(size_t x, size_t y, int color) const;

	float FindNearestDistance(const std::list<std::weak_ptr<BaseObj>>& pawns,
							  const std::function<float(const std::shared_ptr<BaseObj>&)>& getNearestSide) const;

	void Move(float deltaTime) override;

	void Draw() const override;

	[[nodiscard]] static bool IsCollideWith(const Rectangle& r1, const Rectangle& r2);

	[[nodiscard]] virtual std::list<std::weak_ptr<BaseObj>> IsCanMove(float deltaTime);

	void TickUpdate(float deltaTime) override;

	virtual void Shot();

	[[nodiscard]] Direction GetDirection() const;

	void SetDirection(Direction direction);

	[[nodiscard]] float GetBulletWidth() const;
	void SetBulletWidth(float bulletWidth);

	[[nodiscard]] float GetBulletHeight() const;
	void SetBulletHeight(float bulletHeight);

	[[nodiscard]] float GetBulletSpeed() const;
	void SetBulletSpeed(float bulletSpeed);

	PlayerKeys keyboardButtons;

private:
	Direction _direction = UP;

	int _damage{1};

	float _bulletWidth{6.f};

	float _bulletHeight{6.f};

	float _bulletSpeed{300.f};

protected:
	size_t _windowWidth{};
	size_t _windowHeight{};

	int* _windowBuffer;

	std::shared_ptr<EventSystem> _events;

	std::vector<std::shared_ptr<BaseObj>>* _allPawns{nullptr};
};
