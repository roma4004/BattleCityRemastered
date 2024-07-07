#include "../headers/PlayerTwo.h"

#include "../headers/EventSystem.h"
#include "../headers/MoveLikeTankBeh.h"

#include <chrono>

PlayerTwo::PlayerTwo(const Rectangle& rect, const int color, const int health, int* windowBuffer,
                     const UPoint windowSize, Direction direction, float speed,
                     std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
                     std::string name, std::string fraction, std::unique_ptr<IInputProvider>& inputProvider,
                     std::shared_ptr<BulletPool> bulletPool)
	: Tank{rect,
	       color,
	       health,
	       windowBuffer,
	       windowSize,
	       direction,
	       speed,
	       allObjects,
	       std::move(events),
	       std::make_shared<MoveLikeTankBeh>(this, allObjects),
	       std::move(bulletPool),
	       std::move(name),
	       std::move(fraction)},
	  _inputProvider{std::move(inputProvider)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();

	_events->EmitEvent(_name + "_Spawn");
}


PlayerTwo::~PlayerTwo()
{
	Unsubscribe();

	_events->EmitEvent(_name + "_Died");
}

void PlayerTwo::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		if (!_isActiveTeamFreeze)
		{
			this->TickUpdate(deltaTime);
		}
	});

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });

	_events->AddListener("DrawHealthBar", _name, [this]() { this->DrawHealthBar(); });

	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusTeamFreezeEnable",
			_name,
			[this](const std::string& author, const std::string& fraction, int bonusDurationTime)
			{
				if (fraction != _fraction)
				{
					this->_isActiveTeamFreeze = true;
				}
			});
	_events->AddListener<const std::string&, const std::string&>(
			"BonusTeamFreezeDisable",
			_name,
			[this](const std::string& author, const std::string& fraction)
			{
				if (fraction == _fraction)
				{
					this->_isActiveTeamFreeze = false;
				}
			});

	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusHelmetEnable",
			_name,
			[this](const std::string& author, const std::string& fraction, int bonusDurationTime)
			{
				if (fraction == _fraction && author == _name)
				{
					this->_isActiveHelmet = true;
				}
			});
	_events->AddListener<const std::string&, const std::string&>(
			"BonusHelmetDisable",
			_name,
			[this](const std::string& author, const std::string& fraction)
			{
				if (fraction == _fraction && author == _name)
				{
					this->_isActiveHelmet = false;
				}
			});
}

void PlayerTwo::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);

	_events->RemoveListener("DrawHealthBar", _name);

	_events->RemoveListener<const std::string&, const std::string&, int>("BonusTeamFreezeEnable", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusTeamFreezeDisable", _name);

	_events->RemoveListener<const std::string&, const std::string&>("BonusHelmetEnable", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusHelmetDisable", _name);
}

void PlayerTwo::TickUpdate(const float deltaTime)
{
	const auto playerKeys = _inputProvider->GetKeysStats();

	// move
	if (playerKeys.left)
	{
		SetDirection(LEFT);
		_moveBeh->Move(deltaTime);
	}
	else if (playerKeys.right)
	{
		SetDirection(RIGHT);
		_moveBeh->Move(deltaTime);
	}
	else if (playerKeys.up)
	{
		SetDirection(UP);
		_moveBeh->Move(deltaTime);
	}
	else if (playerKeys.down)
	{
		SetDirection(DOWN);
		_moveBeh->Move(deltaTime);
	}

	// shot
	if (playerKeys.shot && IsReloadFinish())
	{
		this->Shot();
		_lastTimeFire = std::chrono::system_clock::now();
	}
}

void PlayerTwo::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("PlayerTwoHit", author, fraction);

	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>("PlayerTwoDied", author, fraction);
	}
}
