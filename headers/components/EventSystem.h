#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>

// traits for deducing types
template<typename T>
struct callable_signature;

// Lambda с operator()
template<typename T> requires requires { &T::operator(); }
struct callable_signature<T> : callable_signature<decltype(&T::operator())> {};

// Const lambda without arguments
template<typename Class>
struct callable_signature<void(Class::*)() const>
{
	template<typename CallableT>
	static void call_add_listener(auto* eventSystem, const std::string& eventName, const std::string& listenerName,
	                              CallableT&& callback)
	{
		eventSystem->template AddListenerImpl<>(eventName, listenerName, std::forward<CallableT>(callback));
	}
};

// Mutable lambda without arguments
template<typename Class>
struct callable_signature<void(Class::*)()>
{
	template<typename CallableT>
	static void call_add_listener(auto* eventSystem, const std::string& eventName, const std::string& listenerName,
	                              CallableT&& callback)
	{
		eventSystem->template AddListenerImpl<>(eventName, listenerName, std::forward<CallableT>(callback));
	}
};

// Const lambda with arguments
template<typename Class, typename R, typename... Args>
struct callable_signature<R(Class::*)(Args...) const>
{
	template<typename CallableT>
	static void call_add_listener(auto* eventSystem, const std::string& eventName, const std::string& listenerName,
	                              CallableT&& callback)
	{
		eventSystem->template AddListenerImpl<Args...>(eventName, listenerName, std::forward<CallableT>(callback));
	}
};

// mutable lambda with arguments
template<typename Class, typename R, typename... Args>
struct callable_signature<R(Class::*)(Args...)>
{
	template<typename CallableT>
	static void call_add_listener(auto* eventSystem, const std::string& eventName, const std::string& listenerName,
	                              CallableT&& callback)
	{
		eventSystem->template AddListenerImpl<Args...>(eventName, listenerName, std::forward<CallableT>(callback));
	}
};

// function pointers
template<typename R, typename... Args>
struct callable_signature<R(*)(Args...)>
{
	template<typename CallableT>
	static void call_add_listener(auto* eventSystem, const std::string& eventName, const std::string& listenerName,
	                              CallableT&& callback)
	{
		eventSystem->template AddListenerImpl<Args...>(eventName, listenerName, std::forward<CallableT>(callback));
	}
};

// std::function
template<typename R, typename... Args>
struct callable_signature<std::function<R(Args...)>>
{
	template<typename CallableT>
	static void call_add_listener(auto* eventSystem, const std::string& eventName, const std::string& listenerName,
	                              CallableT&& callback)
	{
		eventSystem->template AddListenerImpl<Args...>(eventName, listenerName, std::forward<CallableT>(callback));
	}
};

// concepts for check a callable object
template<typename T>
concept Callable = requires { typename callable_signature<T>; };

class BaseEvent
{
public:
	virtual ~BaseEvent() = default;
	virtual void RemoveListener(const std::string& listenerName) = 0;
	virtual bool HasListeners() const = 0;
};

template<typename... Args>
class Event final : public BaseEvent
{
public:
	using callbackType = std::function<void(Args...)>;

	void AddListener(const std::string& listenerName, callbackType callback)
	{
		_listeners[listenerName] = std::move(callback);
	}

	template<typename... FwdArgs>
	void Emit(FwdArgs&&... args)
	{
		for (const auto& [_, callback]: _listeners)
		{
			callback(std::forward<FwdArgs>(args)...);
		}
	}

	void RemoveListener(const std::string& listenerName) override { _listeners.erase(listenerName); }

	bool HasListeners() const override { return !_listeners.empty(); }

private:
	std::unordered_map<std::string, callbackType> _listeners;
};

class EventSystem final
{
	// storing info about an event type
	struct EventInfo
	{
		std::unique_ptr<BaseEvent> event;
		std::type_info const* type_info;

		EventInfo(std::unique_ptr<BaseEvent> ev, std::type_info const* ti)
			: event(std::move(ev)), type_info(ti) {}
	};

	std::unordered_map<std::string, EventInfo> _events;

	// Helper for getting a typed event
	template<typename... Args>
	Event<Args...>* GetTypedEvent(const std::string& eventName)
	{
		if (const auto it = _events.find(eventName);
			it != _events.end() && *it->second.type_info == typeid(Event<Args...>))
		{
			return static_cast<Event<Args...>*>(it->second.event.get());
		}

		return nullptr;
	}

public:
	// Main overload for auto-deducing types
	template<Callable CallableT>
	void AddListener(const std::string& eventName, const std::string& listenerName, CallableT&& callback)
	{
		callable_signature<std::decay_t<CallableT>>::call_add_listener(this, eventName, listenerName,
		                                                               std::forward<CallableT>(callback));
	}

	// internal implementation for concrete types (used in callable_signature)
	template<typename... Args, Callable CallableT>
	void AddListenerImpl(const std::string& eventName, const std::string& listenerName, CallableT&& callback)
	{
		//create new if not exist
		if (const auto it = _events.find(eventName);
			it == _events.end())
		{
			_events.emplace(eventName, EventInfo{std::make_unique<Event<Args...>>(), &typeid(Event<Args...>)});
		}

		//add subscription
		if (auto* event = GetTypedEvent<Args...>(eventName))
		{
			event->AddListener(listenerName, std::forward<CallableT>(callback));
		}
	}

	// Overload for an explicit argument type set std::function
	// template<typename... Args>
	// void AddListener(const std::string& eventName, const std::string& listenerName,
	//                  std::function<void(Args...)> callback)
	// {
	// 	AddListenerImpl<Args...>(eventName, listenerName, std::move(callback));
	// }

	// EmitEvent with auto-deducing types
	template<typename... Args>
	void EmitEvent(const std::string& eventName, Args&&... args)
	{
		if (auto* event = GetTypedEvent<std::decay_t<Args>...>(eventName))
		{
			event->Emit(std::forward<Args>(args)...);
		}
	}

	void RemoveListener(const std::string& eventName, const std::string& listenerName)
	{
		if (const auto it = _events.find(eventName);
			it != _events.end())
		{
			it->second.event->RemoveListener(listenerName);
		}
	}

	bool HasEvent(const std::string& eventName) const { return _events.contains(eventName); }

	bool HasListeners(const std::string& eventName) const
	{
		const auto it = _events.find(eventName);
		return it != _events.end() && it->second.event->HasListeners();
	}

	// Get event info
	// template<typename... Args>
	// bool IsEventOfType(const std::string& eventName) const
	// {
	// 	const auto it = _events.find(eventName);
	// 	return it != _events.end() && *it->second.type_info == typeid(Event<Args...>);
	// }
};

//TODO: how to improve, duplicated code, std::string_view, NRVO, remove std::function, cleanup
