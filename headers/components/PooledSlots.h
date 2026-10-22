#pragma once

#include <cstddef>
#include <memory>
#include <queue>
#include <vector>

//NOTE: the free list of BulletPool and TankPool; an object on it must not listen, or a spawn subscribes it twice
template<class T>
class PooledSlots final
{
	std::queue<std::shared_ptr<T>> _free{};
	std::vector<std::shared_ptr<T>> _inPlay{};

	void Shelve(const std::shared_ptr<T>& obj)
	{
		obj->Deactivate();
		_free.push(obj);
	}

public:
	//NOTE: nullptr means the free list is empty - the pool cannot build one, it knows no type's constructor
	[[nodiscard]] std::shared_ptr<T> TakeFree()
	{
		std::shared_ptr<T> obj{nullptr};
		if (!_free.empty())
		{
			obj = _free.front();
			_free.pop();
		}

		return obj;
	}

	void AddFree(std::shared_ptr<T> obj) { _free.push(std::move(obj)); }

	void Track(std::shared_ptr<T> obj) { _inPlay.push_back(std::move(obj)); }

	//NOTE: the slots are free before the caller announces the result, so a listener may spawn again
	[[nodiscard]] std::vector<std::shared_ptr<T>> ReclaimDead()
	{
		auto isDead = [](const std::shared_ptr<T>& obj) { return !obj->GetIsAlive(); };

		std::vector<std::shared_ptr<T>> reclaimed{};
		for (const std::shared_ptr<T>& obj: _inPlay)
		{
			if (isDead(obj))
			{
				reclaimed.push_back(obj);
			}
		}
		std::erase_if(_inPlay, isDead);

		for (const std::shared_ptr<T>& obj: reclaimed)
		{
			Shelve(obj);
		}

		return reclaimed;
	}

	//NOTE: a reset ends the match, so every object goes back to the free list; Reset arms it on its next spawn
	void ReclaimAll()
	{
		for (const std::shared_ptr<T>& obj: _inPlay)
		{
			Shelve(obj);
		}

		_inPlay.clear();
	}

	[[nodiscard]] std::size_t FreeCount() const noexcept { return _free.size(); }
};
