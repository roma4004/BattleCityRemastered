#pragma once

#include <functional>
#include <iostream>
#include <memory>

//TODO: template for deducing lambda parameters type can be constexpr?
//TODO: create eventType and derived just like network command to compile time parameter check and replace event name
namespace detail
{
// Type adapter for auto conversion const char* to std::string
template<typename T>
struct type_adapter
{
	using type = T;
};

// specialization for const char*
template<>
struct type_adapter<const char*>
{
	using type = std::string;
};

// specialization for char*
template<>
struct type_adapter<char*>
{
	using type = std::string;
};

// specialization for a char array
template<size_t N>
struct type_adapter<char[N]>
{
	using type = std::string;
};

template<size_t N>
struct type_adapter<const char[N]>
{
	using type = std::string;
};

template<typename T>
using type_adapter_t = type_adapter<std::decay_t<T>>::type;
}// namespace detail

// traits for deducing types
template<typename T>
struct callable_signature;

// Lambda с operator()
template<typename T>
	requires requires { &T::operator(); }
struct callable_signature<T> : callable_signature<decltype(&T::operator())> {};

// Const lambda without arguments
template<typename Class>
struct callable_signature<void (Class::*)() const>
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
struct callable_signature<void (Class::*)()>
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
struct callable_signature<R (Class::*)(Args...) const>
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
struct callable_signature<R (Class::*)(Args...)>
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
struct callable_signature<R (*)(Args...)>
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
	virtual size_t GetArgumentCount() const = 0;
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
			try
			{
				callback(std::forward<FwdArgs>(args)...);
			}
			catch (const std::exception& e)
			{
				std::cerr << "Exception in event callback: " << e.what() << '\n';
				// continue listening to other events
			}
			catch (...)
			{
				std::cerr << "Unknown exception in event callback" << '\n';
			}
		}
	}

	void RemoveListener(const std::string& listenerName) override { _listeners.erase(listenerName); }

	bool HasListeners() const override { return !_listeners.empty(); }

	size_t GetArgumentCount() const override { return sizeof...(Args); }

private:
	std::unordered_map<std::string, callbackType> _listeners;
};

class EventSystem final
{
	// storing info about an event type
	struct EventInfo
	{
		std::unique_ptr<BaseEvent> event;
		const std::type_info* type_info;

		EventInfo(std::unique_ptr<BaseEvent> ev, const std::type_info* ti)
			: event(std::move(ev))
			, type_info(ti) {}
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

	// Helper for getting event by name and argument count
	BaseEvent* GetEventByNameAndArgCount(const std::string& eventName, const size_t argCount)
	{
		if (const auto it = _events.find(eventName);
			it != _events.end() && it->second.event->GetArgumentCount() == argCount)
		{
			return it->second.event.get();
		}
		return nullptr;
	}

	// Helper for getting any event by name without any checking
	BaseEvent* GetEventByName(const std::string& eventName)
	{
		if (const auto it = _events.find(eventName); it != _events.end())
		{
			return it->second.event.get();
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
		if (const auto it = _events.find(eventName); it == _events.end())
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

	// EmitEvent with auto-deducing types, find by name and argument count
	template<typename... Args>
	void EmitEvent(const std::string& eventName, Args&&... args)
	{
		constexpr size_t argCount = sizeof...(Args);
		if (auto* event = GetEventByNameAndArgCount(eventName, argCount))
		{
			// if (auto* typedEvent = static_cast<Event<std::decay_t<Args>...>*>(event))
			if (auto* typedEvent = static_cast<Event<detail::type_adapter_t<Args>...>*>(event))
			{
				typedEvent->Emit(std::forward<Args>(args)...);
			}
		}
	}

	// spec with no arguments
	void EmitEvent(const std::string& eventName)
	{
		if (auto* event = GetEventByNameAndArgCount(eventName, 0))
		{
			if (auto* typedEvent = static_cast<Event<>*>(event))
			{
				typedEvent->Emit();
			}
		}
	}

	void RemoveListener(const std::string& eventName, const std::string& listenerName)
	{
		if (auto* event = GetEventByName(eventName))
		{
			event->RemoveListener(listenerName);
		}
	}

	bool HasEvent(const std::string& eventName) const { return _events.contains(eventName); }

	bool HasListeners(const std::string& eventName) const
	{
		const auto it = _events.find(eventName);
		return it != _events.end() && it->second.event->HasListeners();
	}

	size_t GetEventArgumentCount(const std::string& eventName) const
	{
		const auto it = _events.find(eventName);
		return it != _events.end() ? it->second.event->GetArgumentCount() : 0;
	}
};
