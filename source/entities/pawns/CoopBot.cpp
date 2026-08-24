#include "entities/pawns/CoopBot.h"
#include "entities/obstacles/IFortress.h"
#include "entities/pawns/PawnProperty.h"

CoopBot::CoopBot(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, const GameConfig& gameConfig)
	: Bot{std::move(pawnProperty), bulletPool, gameConfig}
{
	m_shouldShootToObstacleStrategy = [this](const std::shared_ptr<BaseObj>& obj)
	{
		if (obj == nullptr)
		{
			return false;
		}

		if (IsAlly(obj) || IsBonus(obj))
		{
			return false;
		}

		if ((obj->GetIsDestructible() || _tier > 2u)
			&& !obj->GetIsPenetrable()// skip water, ice, bush
			&& dynamic_cast<IFortress*>(obj.get()) == nullptr)//the eagle and the walls around it
		{
			return true;
		}

		return false;
	};

	_shootTimer.cooldown = std::chrono::milliseconds{500};
}

CoopBot::~CoopBot() = default;
