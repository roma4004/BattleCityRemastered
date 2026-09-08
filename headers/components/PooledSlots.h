#pragma once

#include <cstddef>
#include <memory>
#include <queue>
#include <vector>

//NOTE: the free-list half of a pool, shared by BulletPool and TankPool. The rule it carries: an object
//on the free list must not listen, or the next spawn subscribes it twice
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
	//NOTE: nullptr means the free list is empty and the caller builds one - the pool cannot, it does
	//not know how any particular object is constructed
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

	//NOTE: returns what it reclaimed so the caller can log it or announce it - the slots are already
	//free by then, so a listener is free to spawn again
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

	//NOTE: a reset ends the match, so the field goes back on the shelf alive or not - what comes off
	//it next is armed by Reset anyway
	void ReclaimAll()
	{
		for (const std::shared_ptr<T>& obj: _inPlay)
		{
			Shelve(obj);
		}

		_inPlay.clear();
	}

	[[nodiscard]] std::size_t FreeCount() const { return _free.size(); }
};
