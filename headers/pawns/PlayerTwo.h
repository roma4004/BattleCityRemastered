#pragma once

#include "Tank.h"
#include "../interfaces/IInputProvider.h"

class PlayerTwo final : public Tank
{
	void Subscribe();
	void Unsubscribe() const;
	void MoveTo(float deltaTime, Direction direction);

	void TickUpdate(float deltaTime) override;

	std::unique_ptr<IInputProvider> _inputProvider;
	bool _isNetworkControlled;
	bool _isHost;

public:
	PlayerTwo(const ObjRectangle& rect, int color, int health, int* windowBuffer, UPoint windowSize,
	          Direction direction, float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	          const std::shared_ptr<EventSystem>& events, std::string name, std::string fraction,
	          std::unique_ptr<IInputProvider>& inputProvider, std::shared_ptr<BulletPool> bulletPool,
	          bool isNetworkControlled, bool isHost, int tankId);

	~PlayerTwo() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
	void TakeDamage(int damage) override;
};
