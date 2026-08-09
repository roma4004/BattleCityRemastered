#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <ranges>
#include <typeindex>
#include <unordered_set>

class EventSystem;

// RAII handle returned by AddListener: movable, not copyable. Holds a shared_ptr<EventSystem> so
// the bus outlives the subscription for as long as this handle is alive, and unsubscribes
// automatically on destruction (or on move-assignment replacing an already-held subscription) via
// the type-erased callback captured at AddListener call time. release() opts out of the
// auto-unsubscribe for call sites that want the old fire-and-forget (manual Unsubscribe()) behavior.
class EventSubscription
{
public:
	EventSubscription() = default;

	EventSubscription(std::shared_ptr<EventSystem> events, std::function<void()> unsubscribe)
		: _events{std::move(events)}
		, _unsubscribe{std::move(unsubscribe)} {}

	EventSubscription(const EventSubscription&) = delete;
	EventSubscription& operator=(const EventSubscription&) = delete;

	EventSubscription(EventSubscription&& other) noexcept
		: _events{std::move(other._events)}
		, _unsubscribe{std::move(other._unsubscribe)}
	{
		other._unsubscribe = nullptr;
	}

	EventSubscription& operator=(EventSubscription&& other) noexcept
	{
		if (this != &other)
		{
			Unsubscribe();
			_events = std::move(other._events);
			_unsubscribe = std::move(other._unsubscribe);
			other._unsubscribe = nullptr;
		}

		return *this;
	}

	~EventSubscription() { Unsubscribe(); }

	// Opts out of auto-unsubscribe - the subscription then lives until the EventSystem itself
	// removes it (e.g. RemoveAllListeners(listenerName) elsewhere), same as pre-RAII behavior.
	void release() { _unsubscribe = nullptr; }

	// True while this handle actively owns a live subscription. Lets a Subscribe()-style method
	// guard against re-registering into an already-populated single-slot member (see
	// Pawn::SubscribeTickUpdate) - without this, reassigning while already subscribed would
	// unsubscribe-then-replace, and since AddListener's map insert for the same (EventType,
	// listenerName) key already happened before the assignment runs, that unsubscribe call would
	// erase the brand new listener instead of a stale one.
	explicit operator bool() const { return static_cast<bool>(_unsubscribe); }

private:
	void Unsubscribe()
	{
		if (_unsubscribe)
		{
			_unsubscribe();
			_unsubscribe = nullptr;
		}
	}

	std::shared_ptr<EventSystem> _events;
	std::function<void()> _unsubscribe;
};

namespace detail
{
// Wrapper marking a variadic EmitEvent argument as a dispatch KEY rather than a payload value.
// Used to disambiguate the keyed EmitEvent overload from the plain variadic one without any
// risk of colliding with a genuine payload type - see the Key() factory function below.
template<typename KeyT>
struct EventKey
{
	KeyT value;
};

template<typename T>
struct is_event_key : std::false_type {};

template<typename KeyT>
struct is_event_key<EventKey<KeyT>> : std::true_type {};

template<typename T>
constexpr bool is_event_key_v = is_event_key<std::decay_t<T>>::value;

// Extracts the first type of a non-empty pack - used below to pull the single EventType out of
// a listener lambda's parameter list once callable_signature has verified there's exactly one.
template<typename... Args>
struct first_type;

template<typename T, typename... Rest>
struct first_type<T, Rest...>
{
	using type = T;
};

template<typename... Args>
using first_type_t = typename first_type<Args...>::type;
}// namespace detail

// Wrap a per-instance dispatch key (e.g. a tank's uuid) for the keyed EmitEvent/AddListener
// overloads: EmitEvent(Key(uuid), PosEvent{...}) delivers only to listeners registered via
// AddListener(uuid, listenerName, callback) for that same key - everyone else registered under
// the plain (non-keyed) bucket for that same EventType is unaffected, and vice versa.
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

// Zero-parameter lambdas are no longer supported under the type_index-keyed design: the single
// EventType parameter's C++ type IS the dispatch key, so there's nothing to key off of without
// one. Every event - even ones that used to carry no data - gets a uniquely-named struct (an
// empty tag struct if it truly has no fields) and the listener takes it by value/const-ref,
// e.g. `[](const FrameStartEvent&){}` instead of `[](){}`.
template<typename Class>
struct callable_signature<void (Class::*)() const>
{
	static_assert(sizeof(Class) == 0,
				  "EventSystem: listener must take exactly one event-struct parameter (e.g. "
				  "`[](const FooEvent&){}`), even for events with no data - give it an empty tag "
				  "struct instead. Zero-parameter listeners are no longer supported.");
};

template<typename Class>
struct callable_signature<void (Class::*)()>
{
	static_assert(sizeof(Class) == 0,
				  "EventSystem: listener must take exactly one event-struct parameter (e.g. "
				  "`[](const FooEvent&){}`), even for events with no data - give it an empty tag "
				  "struct instead. Zero-parameter listeners are no longer supported.");
};

// Const lambda with arguments
template<typename Class, typename R, typename... Args>
struct callable_signature<R (Class::*)(Args...) const>
{
	static_assert(sizeof...(Args) == 1,
				  "EventSystem: listener callback must take exactly one event-struct parameter, "
				  "e.g. `[](const FooEvent& e){...}` - a single eventName string used to carry "
				  "several distinct payload shapes is no longer supported, split into separate "
				  "event structs.");

	using EventType = std::decay_t<detail::first_type_t<Args...>>;

	template<typename CallableT>
	static EventSubscription call_add_listener(auto* eventSystem, const std::string& listenerName, CallableT&& callback)
	{
		return eventSystem->template AddListenerImpl<EventType>(listenerName, std::forward<CallableT>(callback));
	}

	template<typename KeyT, typename CallableT>
	static EventSubscription call_add_keyed_listener(auto* eventSystem, const KeyT& key,
													 const std::string& listenerName,
													 CallableT&& callback)
	{
		return eventSystem->template AddKeyedListenerImpl<KeyT, EventType>(key, listenerName,
																		   std::forward<CallableT>(callback));
	}
};

// mutable lambda with arguments
template<typename Class, typename R, typename... Args>
struct callable_signature<R (Class::*)(Args...)>
{
	static_assert(sizeof...(Args) == 1,
				  "EventSystem: listener callback must take exactly one event-struct parameter, "
				  "e.g. `[](const FooEvent& e){...}` - a single eventName string used to carry "
				  "several distinct payload shapes is no longer supported, split into separate "
				  "event structs.");

	using EventType = std::decay_t<detail::first_type_t<Args...>>;

	template<typename CallableT>
	static EventSubscription call_add_listener(auto* eventSystem, const std::string& listenerName, CallableT&& callback)
	{
		return eventSystem->template AddListenerImpl<EventType>(listenerName, std::forward<CallableT>(callback));
	}

	template<typename KeyT, typename CallableT>
	static EventSubscription call_add_keyed_listener(auto* eventSystem, const KeyT& key,
													 const std::string& listenerName,
													 CallableT&& callback)
	{
		return eventSystem->template AddKeyedListenerImpl<KeyT, EventType>(key, listenerName,
																		   std::forward<CallableT>(callback));
	}
};

// function pointers
template<typename R, typename... Args>
struct callable_signature<R (*)(Args...)>
{
	static_assert(sizeof...(Args) == 1,
				  "EventSystem: listener callback must take exactly one event-struct parameter.");

	using EventType = std::decay_t<detail::first_type_t<Args...>>;

	template<typename CallableT>
	static EventSubscription call_add_listener(auto* eventSystem, const std::string& listenerName, CallableT&& callback)
	{
		return eventSystem->template AddListenerImpl<EventType>(listenerName, std::forward<CallableT>(callback));
	}

	template<typename KeyT, typename CallableT>
	static EventSubscription call_add_keyed_listener(auto* eventSystem, const KeyT& key,
													 const std::string& listenerName,
													 CallableT&& callback)
	{
		return eventSystem->template AddKeyedListenerImpl<KeyT, EventType>(key, listenerName,
																		   std::forward<CallableT>(callback));
	}
};

// std::function
template<typename R, typename... Args>
struct callable_signature<std::function<R(Args...)>>
{
	static_assert(sizeof...(Args) == 1,
				  "EventSystem: listener callback must take exactly one event-struct parameter.");

	using EventType = std::decay_t<detail::first_type_t<Args...>>;

	template<typename CallableT>
	static EventSubscription call_add_listener(auto* eventSystem, const std::string& listenerName, CallableT&& callback)
	{
		return eventSystem->template AddListenerImpl<EventType>(listenerName, std::forward<CallableT>(callback));
	}

	template<typename KeyT, typename CallableT>
	static EventSubscription call_add_keyed_listener(auto* eventSystem, const KeyT& key,
													 const std::string& listenerName,
													 CallableT&& callback)
	{
		return eventSystem->template AddKeyedListenerImpl<KeyT, EventType>(key, listenerName,
																		   std::forward<CallableT>(callback));
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

// enable_shared_from_this so AddListener can hand the EventSubscription it returns a
// shared_ptr<EventSystem> that keeps the bus alive for as long as that subscription lives -
// every EventSystem in this codebase is already held via shared_ptr (see CLAUDE.md), so this
// never introduces a new ownership requirement, just exposes the existing one.
class EventSystem final : public std::enable_shared_from_this<EventSystem>
{
	// storing info about an event type - the map key (std::type_index of the EventType payload
	// struct) already IS the type identity, so unlike the old string-keyed design there's no
	// separate runtime type_info re-check needed here: a lookup hit is always exactly the right
	// C++ type by construction (AddListenerImpl<EventType> is the only thing that ever inserts
	// under type_index(typeid(EventType))).
	struct EventInfo
	{
		std::unique_ptr<BaseEvent> event;
	};

	std::unordered_map<std::type_index, EventInfo> _events;

	// Helper for getting a typed event
	template<typename EventType>
	Event<EventType>* GetTypedEvent()
	{
		if (const auto it = _events.find(std::type_index(typeid(EventType))); it != _events.end())
		{
			return static_cast<Event<EventType>*>(it->second.event.get());
		}

		return nullptr;
	}

	// storing info about a keyed event type - separate map from EventInfo/_events above,
	// so nothing about the plain broadcast path changes.
	struct KeyedEventInfo
	{
		std::unique_ptr<BaseKeyedEvent> event;
	};

	std::unordered_map<std::type_index, KeyedEventInfo> _keyedEvents;

	// Helper for getting a typed keyed event
	template<typename KeyT, typename EventType>
	KeyedEvent<KeyT, EventType>* GetTypedKeyedEvent()
	{
		if (const auto it = _keyedEvents.find(std::type_index(typeid(EventType))); it != _keyedEvents.end())
		{
			return static_cast<KeyedEvent<KeyT, EventType>*>(it->second.event.get());
		}

		return nullptr;
	}

	// Reverse index: listenerName -> the event types it's currently registered under (in either
	// _events or _keyedEvents). Lets RemoveAllListeners(listenerName) sweep only the event types
	// that listener actually touched instead of every event type known to the system. Kept up to
	// date on both the add side (AddListenerImpl/AddKeyedListenerImpl) and the single-target
	// remove side (RemoveListener<EventType>) - a stale leftover entry here is harmless (it just
	// costs one no-op erase attempt later), never a correctness problem.
	std::unordered_map<std::string, std::unordered_set<std::type_index>> _listenerToEventTypes;

	void TrackSubscription(const std::string& listenerName, const std::type_index eventType)
	{
		_listenerToEventTypes[listenerName].insert(eventType);
	}

	void UntrackSubscription(const std::string& listenerName, const std::type_index eventType)
	{
		if (const auto it = _listenerToEventTypes.find(listenerName); it != _listenerToEventTypes.end())
		{
			it->second.erase(eventType);
			if (it->second.empty())
			{
				_listenerToEventTypes.erase(it);
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
		// EmitEvent mismatch check used to be: this is a debug-only diagnostic, a no-op in release.
		bool anyLeftoverListeners = false;

		for (const auto& [eventType, eventInfo]: _events)
		{
			if (eventInfo.event->HasListeners())
			{
				std::cerr << "EventSystem: event type \"" << eventType.name() << "\" still has listeners at "
						<< "shutdown - some object's Unsubscribe()/RemoveListener() was never called.\n";
				anyLeftoverListeners = true;
			}
		}

		for (const auto& [eventType, keyedEventInfo]: _keyedEvents)
		{
			if (keyedEventInfo.event->HasListeners())
			{
				std::cerr << "EventSystem: keyed event type \"" << eventType.name() << "\" still has listeners "
						<< "at shutdown - some object's Unsubscribe()/RemoveListener() was never called.\n";
				anyLeftoverListeners = true;
			}
		}

		assert(!anyLeftoverListeners && "EventSystem: listeners still registered at destruction, see stderr");
#endif
	}

	// Main overload for auto-deducing types. eventName is gone: the listener's single parameter
	// type IS the dispatch key (type_index(typeid(EventType))) - see callable_signature above.
	// Returns an EventSubscription RAII handle - keep it alive (e.g. in a std::vector<EventSubscription>
	// member) for as long as the listener should stay registered; letting it go out of scope
	// unsubscribes automatically. Call .release() on it to opt back into manual/never-auto-unsubscribe
	// behavior.
	template<Callable CallableT>
	[[nodiscard]] EventSubscription AddListener(const std::string& listenerName, CallableT&& callback)
	{
		return callable_signature<std::decay_t<CallableT>>::call_add_listener(this, listenerName,
																			  std::forward<CallableT>(callback));
	}

	// internal implementation for the concrete EventType (used in callable_signature)
	template<typename EventType, Callable CallableT>
	EventSubscription AddListenerImpl(const std::string& listenerName, CallableT&& callback)
	{
		const std::type_index key(typeid(EventType));

		//create new if not exist
		if (const auto it = _events.find(key); it == _events.end())
		{
			_events.emplace(key, EventInfo{std::make_unique<Event<EventType>>()});
		}

		//add subscription
		if (auto* event = GetTypedEvent<EventType>())
		{
			event->AddListener(listenerName, std::forward<CallableT>(callback));
			TrackSubscription(listenerName, key);
		}

		return EventSubscription(shared_from_this(), [this, listenerName]()
		{
			this->RemoveListener<EventType>(listenerName);
		});
	}

	// Keyed overload - subscribe to a specific dispatch key (e.g. a tank's uuid) instead of the
	// plain broadcast bucket. A distinct 3-arg overload, so it can never be confused with the
	// plain 2-arg AddListener above at any call site.
	template<typename KeyT, Callable CallableT>
	[[nodiscard]] EventSubscription AddListener(const KeyT& key, const std::string& listenerName, CallableT&& callback)
	{
		return callable_signature<std::decay_t<CallableT>>::call_add_keyed_listener(this, key, listenerName,
			std::forward<CallableT>(callback));
	}

	// internal implementation for the concrete keyed EventType (used in callable_signature)
	template<typename KeyT, typename EventType, Callable CallableT>
	EventSubscription AddKeyedListenerImpl(const KeyT& key, const std::string& listenerName, CallableT&& callback)
	{
		const std::type_index typeKey(typeid(EventType));

		if (const auto it = _keyedEvents.find(typeKey); it == _keyedEvents.end())
		{
			_keyedEvents.emplace(typeKey, KeyedEventInfo{std::make_unique<KeyedEvent<KeyT, EventType>>()});
		}

		if (auto* event = GetTypedKeyedEvent<KeyT, EventType>())
		{
			event->AddListener(key, listenerName, std::forward<CallableT>(callback));
			TrackSubscription(listenerName, typeKey);
		}

		return EventSubscription(shared_from_this(), [this, key, listenerName]()
		{
			this->RemoveListener<KeyT, EventType>(key, listenerName);
		});
	}

	// EmitEvent - the payload struct's own type is the dispatch key, found by exact type_index
	// lookup. No listeners registered for this EventType yet is a legitimate no-op (nobody's
	// listening), not an error - unlike the old string-keyed design there's no "name matched but
	// types didn't" case left to diagnose: type_index lookup can't partially match.
	template<typename EventType>
	void EmitEvent(const EventType& eventInstance)
	{
		if (auto* typedEvent = GetTypedEvent<EventType>())
		{
			typedEvent->Emit(eventInstance);
		}
	}

	// Keyed overload - EmitEvent(Key(uuid), FooEvent{...}) delivers only to listeners registered
	// via the keyed AddListener overload for that same (EventType, key) pair, leaving the plain
	// broadcast bucket for FooEvent (if any) untouched. Resolved unambiguously against the plain
	// overload above because EventKey<KeyT> is a distinct fixed parameter type, never confusable
	// with a genuine EventType payload.
	template<typename KeyT, typename EventType>
	void EmitEvent(detail::EventKey<KeyT> key, const EventType& eventInstance)
	{
		if (auto* typedEvent = GetTypedKeyedEvent<KeyT, EventType>())
		{
			typedEvent->Emit(key.value, eventInstance);
		}
	}

	template<typename EventType>
	void RemoveListener(const std::string& listenerName)
	{
		const std::type_index key(typeid(EventType));
		if (const auto it = _events.find(key); it != _events.end())
		{
			it->second.event->RemoveListener(listenerName);
			UntrackSubscription(listenerName, key);
		}
	}

	// Keyed counterpart of RemoveListener<EventType> above - removes listenerName from just the
	// one (KeyT, EventType, key) bucket it was registered under, not every key it might share a
	// name with. This is what a keyed EventSubscription's auto-unsubscribe calls.
	template<typename KeyT, typename EventType>
	void RemoveListener(const KeyT& key, const std::string& listenerName)
	{
		const std::type_index typeKey(typeid(EventType));
		if (const auto it = _keyedEvents.find(typeKey); it != _keyedEvents.end())
		{
			if (auto* event = static_cast<KeyedEvent<KeyT, EventType>*>(it->second.event.get()))
			{
				event->RemoveListener(key, listenerName);
				UntrackSubscription(listenerName, typeKey);
			}
		}
	}

	void RemoveAllListeners(const std::string& listenerName)
	{
		// Only sweep the event types this listenerName is actually known to be subscribed to,
		// instead of every event type registered in the whole system.
		const auto it = _listenerToEventTypes.find(listenerName);
		if (it == _listenerToEventTypes.end())
		{
			return;
		}

		// copy: RemoveListener()/UntrackSubscription() below mutate _listenerToEventTypes, which
		// would invalidate iterators into the very set we're iterating.
		const std::vector<std::type_index> eventTypes(it->second.begin(), it->second.end());

		for (const auto& eventType: eventTypes)
		{
			if (const auto eventIt = _events.find(eventType); eventIt != _events.end())
			{
				eventIt->second.event->RemoveListener(listenerName);
			}

			if (const auto keyedIt = _keyedEvents.find(eventType); keyedIt != _keyedEvents.end())
			{
				keyedIt->second.event->RemoveListener(listenerName);
			}
		}

		_listenerToEventTypes.erase(listenerName);
	}

	template<typename EventType>
	bool HasEvent() const
	{
		return _events.contains(std::type_index(typeid(EventType)));
	}

	template<typename EventType>
	bool HasListeners() const
	{
		const auto it = _events.find(std::type_index(typeid(EventType)));
		return it != _events.end() && it->second.event->HasListeners();
	}

	template<typename EventType>
	size_t GetEventArgumentCount() const
	{
		const auto it = _events.find(std::type_index(typeid(EventType)));
		return it != _events.end() ? it->second.event->GetArgumentCount() : 0;
	}
};
