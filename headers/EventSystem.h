#pragma once

#include "GameMode.h"

#include <functional>
#include <string>
#include <variant>

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
	using EventVariant = std::variant<Event<>,
	                                  Event<const float>,
	                                  Event<const std::string&, const std::string&>,
	                                  Event<const std::string&, const std::string&, int>,
	                                  Event<GameMode>,
	                                  Event<bool>>;

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
