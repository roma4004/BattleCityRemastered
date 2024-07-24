#pragma once

#include <functional>
#include <string>
#include <variant>

enum Direction : int;
enum GameMode : int;
struct FPoint;

template<typename... Args>
struct Event
{
	using ListenerCallback = std::function<void(Args...)>;

	void AddListener(const std::string& listenerName, ListenerCallback callback)
	{
		_listeners[listenerName] = std::move(callback);
	}

	void Emit(Args&&... args)
	{
		for (auto& [_, callback]: _listeners)
		{
			callback(std::forward<Args>(args)...);
		}
	}

	void RemoveListener(const std::string& listenerName) { _listeners.erase(listenerName); }

private:
	std::unordered_map<std::string, ListenerCallback> _listeners;
};

class EventSystem
{
	using EventVariant = std::variant<Event<>, // regular events eg method call
	                                  Event<const float>, // update(deltaTime)
	                                  Event<const FPoint>, // bullets tank new pos,
	                                  Event<const FPoint, const int>, // bullets send new pos, bulletId
	                                  Event<const FPoint, const int, const Direction>, // bullets send new pos, bulletId, direction
	                                  Event<const FPoint, const Direction>, // send newPos and direction
	                                  Event<const Direction>, // send shot
	                                  Event<const int>, // respawn resource changed
	                                  Event<const int, const int>, // send health changed
	                                  Event<const std::string, const int>, // send health changed
	                                  Event<const std::string&, const std::string&>, // statistics, bonus
	                                  Event<const std::string&, const std::string&, int>, // bonus with duration
	                                  Event<const GameMode>, // gameMode switch
	                                  Event<const bool>>; // pause status

	std::unordered_map<std::string, EventVariant> _events;

public:
	template<typename... Args>
	void AddListener(const std::string& eventName, const std::string& listenerName, auto callback)
	{
		if (std::holds_alternative<Event<Args...>>(_events[eventName]))
		{
			std::get<Event<Args...>>(_events[eventName]).AddListener(listenerName, std::move(callback));
		}
		else
		{
			_events[eventName] = Event<Args...>{};
			std::get<Event<Args...>>(_events[eventName]).AddListener(listenerName, std::move(callback));
		}
	}

	template<typename... Args>
	void EmitEvent(const std::string& eventName, Args&... args)
	{
		if (std::holds_alternative<Event<Args...>>(_events[eventName]))
		{
			std::get<Event<Args...>>(_events[eventName]).Emit(std::forward<Args>(args)...);
		}
	}

	template<typename... Args>
	void RemoveListener(const std::string& eventName, const std::string& listenerName)
	{
		if (std::holds_alternative<Event<Args...>>(_events[eventName]))
		{
			std::get<Event<Args...>>(_events[eventName]).RemoveListener(listenerName);
		}
	}
};
