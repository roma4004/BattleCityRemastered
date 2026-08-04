#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <ranges>
#include <unordered_set>

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

// Wrapper marking a variadic EmitEvent argument as a dispatch KEY rather than a payload value.
// Used to disambiguate the keyed EmitEvent overload from the plain variadic one without any
// risk of colliding with a genuine payload type - see the Key() factory function below.
template<typename KeyT>
struct EventKey
{
	KeyT value;
};

template<typename T>
struct is_event_key : std::false_type
{
};

template<typename KeyT>
struct is_event_key<EventKey<KeyT>> : std::true_type
{
};

template<typename T>
constexpr bool is_event_key_v = is_event_key<std::decay_t<T>>::value;
}// namespace detail

// Wrap a per-instance dispatch key (e.g. a tank's uuid) for the keyed EmitEvent/AddListener
// overloads: EmitEvent("Pos", Key(uuid), pos, dir) delivers only to listeners registered via
// AddListener("Pos", uuid, listenerName, callback) for that same key - everyone else registered
// under the plain (non-keyed) "Pos" bucket is unaffected, and vice versa.
template<typename KeyT>
detail::EventKey<KeyT> Key(KeyT key)
{
	return detail::EventKey<KeyT>{std::move(key)};
}

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

	template<typename KeyT, typename CallableT>
	static void call_add_keyed_listener(auto* eventSystem, const std::string& eventName, const KeyT& key,
										const std::string& listenerName, CallableT&& callback)
	{
		eventSystem->template AddKeyedListenerImpl<KeyT>(eventName, key, listenerName,
														 std::forward<CallableT>(callback));
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

	template<typename KeyT, typename CallableT>
	static void call_add_keyed_listener(auto* eventSystem, const std::string& eventName, const KeyT& key,
										const std::string& listenerName, CallableT&& callback)
	{
		eventSystem->template AddKeyedListenerImpl<KeyT>(eventName, key, listenerName,
														 std::forward<CallableT>(callback));
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
		eventSystem->template AddListenerImpl<std::decay_t<Args>...>(
				eventName, listenerName, std::forward<CallableT>(callback));
	}

	template<typename KeyT, typename CallableT>
	static void call_add_keyed_listener(auto* eventSystem, const std::string& eventName, const KeyT& key,
										const std::string& listenerName, CallableT&& callback)
	{
		eventSystem->template AddKeyedListenerImpl<KeyT, std::decay_t<Args>...>(
				eventName, key, listenerName, std::forward<CallableT>(callback));
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
		eventSystem->template AddListenerImpl<std::decay_t<Args>...>(
				eventName, listenerName, std::forward<CallableT>(callback));
	}

	template<typename KeyT, typename CallableT>
	static void call_add_keyed_listener(auto* eventSystem, const std::string& eventName, const KeyT& key,
										const std::string& listenerName, CallableT&& callback)
	{
		eventSystem->template AddKeyedListenerImpl<KeyT, std::decay_t<Args>...>(
				eventName, key, listenerName, std::forward<CallableT>(callback));
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
		eventSystem->template AddListenerImpl<std::decay_t<Args>...>(
				eventName, listenerName, std::forward<CallableT>(callback));
	}

	template<typename KeyT, typename CallableT>
	static void call_add_keyed_listener(auto* eventSystem, const std::string& eventName, const KeyT& key,
										const std::string& listenerName, CallableT&& callback)
	{
		eventSystem->template AddKeyedListenerImpl<KeyT, std::decay_t<Args>...>(
				eventName, key, listenerName, std::forward<CallableT>(callback));
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
		eventSystem->template AddListenerImpl<std::decay_t<Args>...>(
				eventName, listenerName, std::forward<CallableT>(callback));
	}

	template<typename KeyT, typename CallableT>
	static void call_add_keyed_listener(auto* eventSystem, const std::string& eventName, const KeyT& key,
										const std::string& listenerName, CallableT&& callback)
	{
		eventSystem->template AddKeyedListenerImpl<KeyT, std::decay_t<Args>...>(
				eventName, key, listenerName, std::forward<CallableT>(callback));
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
				callback(args...);
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

// Parallel to BaseEvent/Event<Args...> - backs the keyed (per-instance) dispatch overloads.
// Kept as a fully separate hierarchy so the plain broadcast path above is never touched by this.
class BaseKeyedEvent
{
public:
	virtual ~BaseKeyedEvent() = default;
	// Removes listenerName from every key's bucket - used by RemoveAllListeners(listenerName),
	// which doesn't know (and shouldn't need to know) which key(s) an object subscribed under.
	virtual void RemoveListener(const std::string& listenerName) = 0;
	virtual bool HasListeners() const = 0;
	virtual size_t GetArgumentCount() const = 0;
};

template<typename KeyT, typename... Args>
class KeyedEvent final : public BaseKeyedEvent
{
public:
	using callbackType = std::function<void(Args...)>;

	void AddListener(const KeyT& key, const std::string& listenerName, callbackType callback)
	{
		_listeners[key][listenerName] = std::move(callback);
	}

	template<typename... FwdArgs>
	void Emit(const KeyT& key, FwdArgs&&... args)
	{
		const auto it = _listeners.find(key);
		if (it == _listeners.end())
		{
			return;
		}

		for (const auto& [_, callback]: it->second)
		{
			try
			{
				callback(args...);
			}
			catch (const std::exception& e)
			{
				std::cerr << "Exception in keyed event callback: " << e.what() << '\n';
			}
			catch (...)
			{
				std::cerr << "Unknown exception in keyed event callback" << '\n';
			}
		}
	}

	void RemoveListener(const KeyT& key, const std::string& listenerName)
	{
		if (const auto it = _listeners.find(key); it != _listeners.end())
		{
			it->second.erase(listenerName);
		}
	}

	void RemoveListener(const std::string& listenerName) override
	{
		for (auto& perKeyListeners: _listeners | std::views::values)
		{
			perKeyListeners.erase(listenerName);
		}
	}

	bool HasListeners() const override
	{
		return std::ranges::any_of(_listeners | std::views::values,
								   [](const auto& perKeyListeners) { return !perKeyListeners.empty(); });
	}

	size_t GetArgumentCount() const override { return sizeof...(Args); }

private:
	std::unordered_map<KeyT, std::unordered_map<std::string, callbackType>> _listeners;
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

	// storing info about a keyed event type - separate map from EventInfo/_events above,
	// so nothing about the plain broadcast path changes.
	struct KeyedEventInfo
	{
		std::unique_ptr<BaseKeyedEvent> event;
		const std::type_info* type_info;

		KeyedEventInfo(std::unique_ptr<BaseKeyedEvent> ev, const std::type_info* ti)
			: event(std::move(ev))
			, type_info(ti) {}
	};

	std::unordered_map<std::string, KeyedEventInfo> _keyedEvents;

	// Helper for getting a typed keyed event
	template<typename KeyT, typename... Args>
	KeyedEvent<KeyT, Args...>* GetTypedKeyedEvent(const std::string& eventName)
	{
		if (const auto it = _keyedEvents.find(eventName);
			it != _keyedEvents.end() && *it->second.type_info == typeid(KeyedEvent<KeyT, Args...>))
		{
			return static_cast<KeyedEvent<KeyT, Args...>*>(it->second.event.get());
		}

		return nullptr;
	}

	// Reverse index: listenerName -> the event names it's currently registered under (in either
	// _events or _keyedEvents). Lets RemoveAllListeners(listenerName) sweep only the event names
	// that listener actually touched instead of every event name known to the system. Kept up to
	// date on both the add side (AddListenerImpl/AddKeyedListenerImpl) and the single-target
	// remove side (the RemoveListener overloads) - a stale leftover entry here is harmless (it
	// just costs one no-op erase attempt later), never a correctness problem.
	std::unordered_map<std::string, std::unordered_set<std::string>> _listenerToEventNames;

	void TrackSubscription(const std::string& listenerName, const std::string& eventName)
	{
		_listenerToEventNames[listenerName].insert(eventName);
	}

	void UntrackSubscription(const std::string& listenerName, const std::string& eventName)
	{
		if (const auto it = _listenerToEventNames.find(listenerName); it != _listenerToEventNames.end())
		{
			it->second.erase(eventName);
			if (it->second.empty())
			{
				_listenerToEventNames.erase(it);
			}
		}
	}

public:
	EventSystem() = default;

	~EventSystem()
	{
#ifndef NDEBUG
		// If something's still subscribed when the bus itself is being torn down, some object's
		// Unsubscribe() was never called (or ran too late/never at all) - a real cleanup bug, since
		// nothing will ever deliver to these listeners again anyway. Loud on purpose, same as the
		// EmitEvent mismatch check above: this is a debug-only diagnostic, a no-op in release.
		bool anyLeftoverListeners = false;

		for (const auto& [eventName, eventInfo]: _events)
		{
			if (eventInfo.event->HasListeners())
			{
				std::cerr << "EventSystem: event \"" << eventName << "\" still has listeners at shutdown "
						<< "- some object's Unsubscribe()/RemoveListener() was never called.\n";
				anyLeftoverListeners = true;
			}
		}

		for (const auto& [eventName, keyedEventInfo]: _keyedEvents)
		{
			if (keyedEventInfo.event->HasListeners())
			{
				std::cerr << "EventSystem: keyed event \"" << eventName << "\" still has listeners at "
						<< "shutdown - some object's Unsubscribe()/RemoveListener() was never called.\n";
				anyLeftoverListeners = true;
			}
		}

		assert(!anyLeftoverListeners && "EventSystem: listeners still registered at destruction, see stderr");
#endif
	}

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
			TrackSubscription(listenerName, eventName);
		}
	}

	// Keyed overload - subscribe to a specific dispatch key (e.g. a tank's uuid) under this event
	// name instead of the plain broadcast bucket. A distinct 4-arg overload, so it can never be
	// confused with the plain 3-arg AddListener above at any call site.
	template<typename KeyT, Callable CallableT>
	void AddListener(const std::string& eventName, const KeyT& key, const std::string& listenerName,
					 CallableT&& callback)
	{
		callable_signature<std::decay_t<CallableT>>::call_add_keyed_listener(this, eventName, key, listenerName,
																			 std::forward<CallableT>(callback));
	}

	// internal implementation for concrete keyed types (used in callable_signature)
	template<typename KeyT, typename... Args, Callable CallableT>
	void AddKeyedListenerImpl(const std::string& eventName, const KeyT& key, const std::string& listenerName,
							  CallableT&& callback)
	{
		if (const auto it = _keyedEvents.find(eventName); it == _keyedEvents.end())
		{
			_keyedEvents.emplace(eventName, KeyedEventInfo{std::make_unique<KeyedEvent<KeyT, Args...>>(),
															&typeid(KeyedEvent<KeyT, Args...>)});
		}

		if (auto* event = GetTypedKeyedEvent<KeyT, Args...>(eventName))
		{
			event->AddListener(key, listenerName, std::forward<CallableT>(callback));
			TrackSubscription(listenerName, eventName);
		}
	}

	// EmitEvent with auto-deducing types, find by name and verify the exact stored Event<Args...> type.
	template<typename... Args>
	void EmitEvent(const std::string& eventName, Args&&... args)
	{
		if (auto* typedEvent = GetTypedEvent<detail::type_adapter_t<Args>...>(eventName))
		{
			typedEvent->Emit(std::forward<Args>(args)...);
			return;
		}

#ifndef NDEBUG
		// A registered event with this name and argument COUNT exists, but the typeid check above failed,
		// meaning the argument TYPES don't match any listener registered for this event name.
		if (GetEventByNameAndArgCount(eventName, sizeof...(Args)) != nullptr)
		{
			std::cerr << "EventSystem: EmitEvent(\"" << eventName << "\") argument types do not match "
					<< "the listener(s) registered for this event name (name + argument count matched, "
					<< "types did not).\n";
			assert(false && "EventSystem: EmitEvent argument type mismatch, see stderr");
		}
#endif
	}

	// Keyed overload - EmitEvent("Pos", Key(uuid), pos, dir) delivers only to listeners registered
	// via the keyed AddListener overload for that same (name, key) pair, leaving the plain
	// broadcast bucket for "Pos" (if any) untouched. Resolved unambiguously against the plain
	// overload above because EventKey<KeyT> is a fixed (non-pack) parameter, not part of Args -
	// verified against this exact compiler via a standalone overload-resolution test before wiring
	// this in, since GCC/Clang partial ordering between a variadic-only and a mixed template can
	// be subtle.
	template<typename KeyT, typename... Args>
	void EmitEvent(const std::string& eventName, detail::EventKey<KeyT> key, Args&&... args)
	{
		if (auto* typedEvent = GetTypedKeyedEvent<KeyT, detail::type_adapter_t<Args>...>(eventName))
		{
			typedEvent->Emit(key.value, std::forward<Args>(args)...);
			return;
		}

#ifndef NDEBUG
		if (const auto it = _keyedEvents.find(eventName);
			it != _keyedEvents.end() && it->second.event->GetArgumentCount() == sizeof...(Args))
		{
			std::cerr << "EventSystem: EmitEvent(\"" << eventName << "\", Key<...>) argument types do not "
					<< "match the keyed listener(s) registered for this event name (name + argument count "
					<< "matched, types did not).\n";
			assert(false && "EventSystem: keyed EmitEvent argument type mismatch, see stderr");
		}
#endif
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
			UntrackSubscription(listenerName, eventName);
		}
	}

	void RemoveAllListeners(const std::string& listenerName)
	{
		// Only sweep the event names this listenerName is actually known to be subscribed to,
		// instead of every event name registered in the whole system.
		const auto it = _listenerToEventNames.find(listenerName);
		if (it == _listenerToEventNames.end())
		{
			return;
		}

		// copy: RemoveListener()/UntrackSubscription() below mutate _listenerToEventNames, which
		// would invalidate iterators into the very set we're iterating.
		const std::vector<std::string> eventNames(it->second.begin(), it->second.end());

		for (const auto& eventName: eventNames)
		{
			if (const auto eventIt = _events.find(eventName); eventIt != _events.end())
			{
				eventIt->second.event->RemoveListener(listenerName);
			}

			if (const auto keyedIt = _keyedEvents.find(eventName); keyedIt != _keyedEvents.end())
			{
				keyedIt->second.event->RemoveListener(listenerName);
			}
		}

		_listenerToEventNames.erase(listenerName);
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
