#pragma once

#include <functional>
#include <memory>
#include <string>
#include <variant>

template<typename... Args>
struct Event
{
	using ListenerCallback = std::function<void(Args...)>;

	void AddListener(const std::string& listenerName, ListenerCallback callback)
	{
		listeners[listenerName] = std::move(callback);
	}

	void Emit(Args&&... args)
	{
		for (auto& [_, callback]: listeners)
		{
			callback(std::forward<Args>(args)...);
		}
	}

	void RemoveListener(const std::string& listenerName) { listeners.erase(listenerName); }

private:
	std::unordered_map<std::string, ListenerCallback> listeners;
};

class EventSystem
{
	using EventVariant = std::variant<Event<>, Event<float>>;

	std::unordered_map<std::string, EventVariant> events;

public:
	template<typename... Args>
	void AddListener(const std::string& eventName, const std::string& listenerName, auto callback)
	{
		if (std::holds_alternative<Event<Args...>>(events[eventName]))
		{
			std::get<Event<Args...>>(events[eventName]).AddListener(listenerName, std::move(callback));
		}
		else
		{
			events[eventName] = Event<Args...>{};
			std::get<Event<Args...>>(events[eventName]).AddListener(listenerName, std::move(callback));
		}
	}

	template<typename... Args>
	void EmitEvent(const std::string& eventName, Args&... args)
	{
		if (std::holds_alternative<Event<Args...>>(events[eventName]))
		{
			std::get<Event<Args...>>(events[eventName]).Emit(std::forward<Args>(args)...);
		}
	}

	template<typename... Args>
	void RemoveListener(const std::string& eventName, const std::string& listenerName)
	{
		if (std::holds_alternative<Event<Args...>>(events[eventName]))
		{
			std::get<Event<Args...>>(events[eventName]).RemoveListener(listenerName);
		}
	}
};
