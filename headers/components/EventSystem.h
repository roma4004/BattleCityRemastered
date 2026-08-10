#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <ranges>
#include <typeindex>

class EventSystem;

// RAII handle returned by AddListener: movable, not copyable. Holds a shared_ptr<EventSystem> so
// the bus outlives the subscription for as long as this handle is alive, and unsubscribes
// automatically on destruction (or on move-assignment replacing an already-held subscription) via
// the type-erased callback captured at AddListener call time. To unsubscribe early, either erase the
// handle from its owning std::vector<EventSubscription>, or move-assign a fresh EventSubscription{}
// over a single named member (see Pawn::UnsubscribeTickUpdate) - both just trigger the destructor.
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

	// True while this handle actively owns a live subscription. Lets a Subscribe()-style method
	// guard against re-registering into an already-populated single-slot member (see
	// Pawn::SubscribeTickUpdate) - without this, reassigning while already subscribed would
	// unsubscribe-then-replace, and since the new AddListener call's list insert already happened
	// before the assignment runs, that unsubscribe call would erase the brand new listener instead
	// of the stale one.
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
using first_type_t = first_type<Args...>::type;
}// namespace detail

// Wrap a per-instance dispatch key (e.g. a tank's uuid) for the keyed EmitEvent/AddListener
// overloads: EmitEvent(Key(uuid), PosEvent{...}) delivers only to listeners registered via
// AddListener(Key(uuid), callback) for that same key - everyone else registered under the plain
// (non-keyed) bucket for that same EventType is unaffected, and vice versa.
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

// Shared body for every callable shape below that carries a concrete Args... pack (const lambda,
// mutable lambda, function pointer, std::function) - all four previously repeated this verbatim,
// differing only in which callable_signature<T> specialization pattern-matched. EventType/
// call_add_listener/call_add_keyed_listener are inherited as-is; only the pattern match differs.
template<typename... Args>
struct callable_signature_args
{
	static_assert(sizeof...(Args) == 1,
				  "EventSystem: listener callback must take exactly one event-struct parameter, "
				  "e.g. `[](const FooEvent& e){...}` - a single eventName string used to carry "
				  "several distinct payload shapes is no longer supported, split into separate "
				  "event structs.");

	using EventType = std::decay_t<detail::first_type_t<Args...>>;

	// Identity-free: O(1) unsubscribe via captured (Event*, iterator), no name lookup.
	template<typename CallableT>
	static EventSubscription call_add_listener(auto* eventSystem, CallableT&& callback)
	{
		return eventSystem->template AddListenerImpl<EventType>(std::forward<CallableT>(callback));
	}

	template<typename KeyT, typename CallableT>
	static EventSubscription call_add_keyed_listener(auto* eventSystem, const KeyT& key, CallableT&& callback)
	{
		return eventSystem->template AddKeyedListenerImpl<KeyT, EventType>(key, std::forward<CallableT>(callback));
	}
};

// Const lambda with arguments
template<typename Class, typename R, typename... Args>
struct callable_signature<R (Class::*)(Args...) const> : callable_signature_args<Args...> {};

// mutable lambda with arguments
template<typename Class, typename R, typename... Args>
struct callable_signature<R (Class::*)(Args...)> : callable_signature_args<Args...> {};

// function pointers
template<typename R, typename... Args>
struct callable_signature<R (*)(Args...)> : callable_signature_args<Args...> {};

// std::function
template<typename R, typename... Args>
struct callable_signature<std::function<R(Args...)>> : callable_signature_args<Args...> {};

// concepts for check a callable object
template<typename T>
concept Callable = requires { typename callable_signature<T>; };

class BaseEvent
{
public:
	virtual ~BaseEvent() = default;
	virtual bool HasListeners() const = 0;
};

// std::list, not a name-keyed map: node addresses stay stable, so a subscription can carry
// its own iterator as identity.
template<typename... Args>
class Event final : public BaseEvent
{
public:
	using callbackType = std::function<void(Args...)>;
	using ListenerHandle = std::list<callbackType>::iterator;

	ListenerHandle AddListener(callbackType callback)
	{
		_listeners.push_back(std::move(callback));
		return std::prev(_listeners.end());
	}

	template<typename... FwdArgs>
	void Emit(FwdArgs&&... args)
	{
		// Advance before invoking: a self-unsubscribing callback erases this node mid-loop.
		for (auto it = _listeners.begin(); it != _listeners.end();)
		{
			const auto next = std::next(it);

			try
			{
				(*it)(args...);
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

			it = next;
		}
	}

	void RemoveListener(ListenerHandle handle) { _listeners.erase(handle); }

	bool HasListeners() const override { return !_listeners.empty(); }

private:
	std::list<callbackType> _listeners;
};

// Parallel to BaseEvent/Event<Args...> - backs the keyed (per-instance) dispatch overloads.
// Kept as a fully separate hierarchy so the plain broadcast path above is never touched by this.
class BaseKeyedEvent
{
public:
	virtual ~BaseKeyedEvent() = default;
	virtual bool HasListeners() const = 0;
};

template<typename KeyT, typename... Args>
class KeyedEvent final : public BaseKeyedEvent
{
public:
	using callbackType = std::function<void(Args...)>;
	using ListenerList = std::list<callbackType>;
	using ListenerHandle = ListenerList::iterator;

	ListenerHandle AddListener(const KeyT& key, callbackType callback)
	{
		auto& listeners = _listeners[key];
		listeners.push_back(std::move(callback));
		return std::prev(listeners.end());
	}

	template<typename... FwdArgs>
	void Emit(const KeyT& key, FwdArgs&&... args)
	{
		const auto it = _listeners.find(key);
		if (it == _listeners.end())
		{
			return;
		}

		// Same next-iterator-first guarantee as Event<Args...>::Emit above - see its comment.
		for (auto lit = it->second.begin(); lit != it->second.end();)
		{
			const auto next = std::next(lit);

			try
			{
				(*lit)(args...);
			}
			catch (const std::exception& e)
			{
				std::cerr << "Exception in keyed event callback: " << e.what() << '\n';
			}
			catch (...)
			{
				std::cerr << "Unknown exception in keyed event callback" << '\n';
			}

			lit = next;
		}
	}

	void RemoveListener(const KeyT& key, ListenerHandle handle)
	{
		if (const auto it = _listeners.find(key); it != _listeners.end())
		{
			it->second.erase(handle);
		}
	}

	bool HasListeners() const override
	{
		return std::ranges::any_of(_listeners | std::views::values,
								   [](const auto& perKeyListeners) { return !perKeyListeners.empty(); });
	}

private:
	std::unordered_map<KeyT, ListenerList> _listeners;
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

	// Auto-deduces EventType from the listener's parameter (type_index(typeid(EventType))).
	// Identity-free. Keep the returned EventSubscription alive while the listener should stay
	// registered.
	template<Callable CallableT>
	[[nodiscard]] EventSubscription AddListener(CallableT&& callback)
	{
		return callable_signature<std::decay_t<CallableT>>::call_add_listener(this, std::forward<CallableT>(callback));
	}

	// Compat shim: listenerName ignored, kept until call sites migrate off it.
	template<Callable CallableT>
	[[nodiscard]] EventSubscription AddListener(const std::string& /*listenerName*/, CallableT&& callback)
	{
		return AddListener(std::forward<CallableT>(callback));
	}

	// internal implementation for the concrete EventType (used in callable_signature)
	template<typename EventType, Callable CallableT>
	EventSubscription AddListenerImpl(CallableT&& callback)
	{
		const std::type_index key(typeid(EventType));

		//create new if not exist
		if (const auto it = _events.find(key); it == _events.end())
		{
			_events.emplace(key, EventInfo{std::make_unique<Event<EventType>>()});
		}

		auto* event = GetTypedEvent<EventType>();
		const auto handle = event->AddListener(std::forward<CallableT>(callback));

		return EventSubscription(shared_from_this(), [event, handle]()
		{
			event->RemoveListener(handle);
		});
	}

	// Keyed overload. Uses Key() to disambiguate from the string-taking compat shim below - a raw
	// KeyT would let a string literal silently bind to the wrong overload.
	template<typename KeyT, Callable CallableT>
	[[nodiscard]] EventSubscription AddListener(detail::EventKey<KeyT> key, CallableT&& callback)
	{
		return callable_signature<std::decay_t<CallableT>>::call_add_keyed_listener(this, key.value,
			std::forward<CallableT>(callback));
	}

	// Compat shim, keyed variant - see the plain-overload shim above.
	template<typename KeyT, Callable CallableT>
	[[nodiscard]] EventSubscription AddListener(const KeyT& key, const std::string& /*listenerName*/, CallableT&& callback)
	{
		return AddListener(Key(key), std::forward<CallableT>(callback));
	}

	// internal implementation for the concrete keyed EventType (used in callable_signature)
	template<typename KeyT, typename EventType, Callable CallableT>
	EventSubscription AddKeyedListenerImpl(const KeyT& key, CallableT&& callback)
	{
		const std::type_index typeKey(typeid(EventType));

		if (const auto it = _keyedEvents.find(typeKey); it == _keyedEvents.end())
		{
			_keyedEvents.emplace(typeKey, KeyedEventInfo{std::make_unique<KeyedEvent<KeyT, EventType>>()});
		}

		auto* event = GetTypedKeyedEvent<KeyT, EventType>();
		const auto handle = event->AddListener(key, std::forward<CallableT>(callback));

		return EventSubscription(shared_from_this(), [event, key, handle]()
		{
			event->RemoveListener(key, handle);
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
};
