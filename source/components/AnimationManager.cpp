#include "components/AnimationManager.h"

AnimationManager::AnimationManager(std::shared_ptr<EventSystem> events): _events(std::move(events))
{
	Subscribe();
}

AnimationManager::~AnimationManager()
{
	Unsubscribe();
}

void AnimationManager::Subscribe()
{
	_events->AddListener<const std::string&>(
			std::string("AnimationUpdate"), _name,
			[this](const std::string& name)
			{
				for (auto& animPassport: _animatedObj)
				{
					if (name == animPassport.name)
					{
						if (++animPassport.currentFrameCounter > 12)
						{
							animPassport.currentFrameCounter = 0;
							if (++animPassport.animationFrame >= animPassport.animationIdLimit)
							{
								animPassport.animationFrame = 0;
							}
						}

						return;
					}
				}
				_animatedObj.emplace_back(AnimationStruct{name, 0, 0, 2});
			});
}

void AnimationManager::Unsubscribe() const
{
	_events->RemoveListener<const std::string&>("AnimationUpdate", _name);
}

int AnimationManager::GetAnimFrame(const std::string& name) const
{
	for (auto& animPassport: _animatedObj)
	{
		if (name == animPassport.name)
		{
			return animPassport.animationFrame;
		}
	}

	return 0;
}

int AnimationManager::GetAnimWater()
{
	if (++_animWater.currentFrameCounter % 48 == 0)
	{
		_animWater.currentFrameCounter = 0;
		if (++_animWater.animationFrame == _animWater.animationIdLimit)
		{
			_animWater.animationFrame = 0;
		}
	}

	return _animWater.animationFrame;
}

int AnimationManager::GetAnimExplosion()
{
	if (++_animExplosion.currentFrameCounter % 12 == 0)
	{
		_animExplosion.currentFrameCounter = 0;
		if (++_animExplosion.animationFrame == _animExplosion.animationIdLimit)
		{
			_animExplosion.animationFrame = 0;
		}
	}

	return _animExplosion.animationFrame;
}

