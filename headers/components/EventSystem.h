#pragma once

// NOTE: 65 TUs include this, so each include here is paid 65 times - a `| views::` pipeline alone
// costs ~52k lines. Pipelines, and anything needing <iostream>/<cassert>, live in EventSystem.cpp
#include <algorithm>
#include <exception>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <source_location>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>

class EventSystem;

// Unsubscribes when it dies, and holds the bus alive for as long as it lives. Unsubscribing early is
// destroying the handle: erase it from its vector, or move-assign a fresh one over a named member
class EventSubscription final
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

	// True while this handle owns a live subscription, so a Subscribe() can skip an already-filled slot
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
// Marks an EmitEvent argument as a dispatch key rather than a payload
template<typename KeyT>
struct EventKey
{
	KeyT value;
};

// Pulls the single EventType out of a listener's parameter list, once the pack is known to hold one
template<typename... Args>
struct first_type;

template<typename T, typename... Rest>
struct first_type<T, Rest...>
{
	using type = T;
};

template<typename... Args>
using first_type_t = first_type<Args...>::type;

// nullptr `what` = non-std exception; origin.line() == 0 = release build.
void ReportListenerException(const char* context, const char* what, const std::source_location& origin) noexcept;

// A throwing listener is reported by its registration site, and everyone after it still gets the event
template<typename ListenerT, typename... CallArgs>
void InvokeGuarded(const char* const context, ListenerT& listener, const CallArgs&... args) noexcept
{
	try
	{
		listener.callback(args...);
	}
	catch (const std::exception& e)
	{
		ReportListenerException(context, e.what(), listener.Origin());
	}
	catch (...)
	{
		ReportListenerException(context, nullptr, listener.Origin());
	}
}

// A callback may unsubscribe anyone, itself included; mid-dispatch that only clears `alive` and frees
// nothing, so the lookahead below cannot end up on released memory
template<typename ListenerListT, typename... CallArgs>
void EmitToList(const char* const context, ListenerListT& listeners, const CallArgs&... args)
{
	for (auto it = listeners.begin(); it != listeners.end();)
	{
		const auto next = std::next(it);

		if (it->alive)
		{
			InvokeGuarded(context, *it, args...);
		}

		it = next;
	}
}

// Stored listener. Debug also keeps its AddListener site; release has no such member at all.
template<typename CallbackT>
struct Listener
{
	Listener(CallbackT&& cb, [[maybe_unused]] const std::source_location& where)
		: callback{std::move(cb)}
#ifndef NDEBUG
		, origin{where}
#endif
	{}

	// Both diagnostics go through this, so the #ifdef lives here and nowhere else.
	[[nodiscard]] const std::source_location& Origin() const noexcept
	{
#ifndef NDEBUG
		return origin;
#else
		static constexpr std::source_location kUnknown{};
		return kUnknown;
#endif
	}

	CallbackT callback;
	// Cleared instead of erased when the unsubscribe lands mid-dispatch; swept once the walk ends.
	bool alive{true};
#ifndef NDEBUG
	std::source_location origin;
#endif
};

constexpr auto IsAlive = [](const auto& listener) { return listener.alive; };

#ifndef NDEBUG
// Defined in EventSystem.cpp, so <iostream> stays out of this header
void ReportLeftoverListener(const char* kind, const char* eventTypeName,
							const std::source_location& origin) noexcept;
#endif
}// namespace detail

// Wraps a per-instance key, a tank's uuid say: the emit then reaches only listeners registered under
// that same (EventType, key) pair, and the broadcast bucket for the type stays untouched
template<typename KeyT>
detail::EventKey<KeyT> Key(KeyT key)
{
	return detail::EventKey<KeyT>{std::move(key)};
}

template<typename T>
struct callable_signature;

template<typename T>
	requires requires { &T::operator(); }
struct callable_signature<T> : callable_signature<decltype(&T::operator())> {};

// Templated on Class only so the assert stays dependent and fires on instantiation, not on sight
template<typename Class>
struct callable_signature_zero_args
{
	static_assert(sizeof(Class) == 0,
				  "EventSystem: listener must take exactly one event-struct parameter (e.g. "
				  "`[](const FooEvent&){}`), even for events with no data - give it an empty tag "
				  "struct instead. Zero-parameter listeners are no longer supported.");
};

template<typename Class>
struct callable_signature<void (Class::*)() const> : callable_signature_zero_args<Class> {};

template<typename Class>
struct callable_signature<void (Class::*)()> : callable_signature_zero_args<Class> {};

// Shared body for every callable shape carrying a concrete Args... pack - const lambda, mutable
// lambda, function pointer, std::function. They inherit it whole and differ only in the match
template<typename... Args>
struct callable_signature_args
{
	static_assert(sizeof...(Args) == 1,
				  "EventSystem: listener callback must take exactly one event-struct parameter, "
				  "e.g. `[](const FooEvent& e){...}` - a single eventName string used to carry "
				  "several distinct payload shapes is no longer supported, split into separate "
				  "event structs.");

	using EventType = std::decay_t<detail::first_type_t<Args...>>;

	// O(1) unsubscribe: the subscription carries the (Event*, iterator) it was made from
	template<typename CallableT>
	static EventSubscription call_add_listener(auto* eventSystem, CallableT&& callback,
											   const std::source_location& origin)
	{
		return eventSystem->template AddListenerImpl<EventType>(std::forward<CallableT>(callback), origin);
	}

	template<typename KeyT, typename CallableT>
	static EventSubscription call_add_keyed_listener(auto* eventSystem, const KeyT& key, CallableT&& callback,
													 const std::source_location& origin)
	{
		return eventSystem->template AddKeyedListenerImpl<KeyT, EventType>(key, std::forward<CallableT>(callback),
																		   origin);
	}
};

template<typename Class, typename R, typename... Args>
struct callable_signature<R (Class::*)(Args...) const> : callable_signature_args<Args...> {};

template<typename Class, typename R, typename... Args>
struct callable_signature<R (Class::*)(Args...)> : callable_signature_args<Args...> {};

template<typename R, typename... Args>
struct callable_signature<R (*)(Args...)> : callable_signature_args<Args...> {};

template<typename R, typename... Args>
struct callable_signature<std::function<R(Args...)>> : callable_signature_args<Args...> {};

template<typename T>
concept Callable = requires { typename callable_signature<T>; };

class BaseEvent
{
public:
	virtual ~BaseEvent() = default;
	virtual bool HasListeners() const = 0;
#ifndef NDEBUG
	virtual void ReportLeftoverListeners(const char* kind, const char* eventTypeName) const = 0;
#endif
};

// std::list because node addresses stay stable - a subscription carries its own iterator as identity
template<typename... Args>
class Event final : public BaseEvent
{
public:
	using callbackType = std::function<void(Args...)>;
	using ListenerList = std::list<detail::Listener<callbackType>>;
	using ListenerHandle = ListenerList::iterator;

	ListenerHandle AddListener(callbackType callback, const std::source_location& origin)
	{
		_listeners.emplace_back(std::move(callback), origin);
		return std::prev(_listeners.end());
	}

	// Copied into every listener, never forwarded: several listeners share one payload, and the first
	// by-value parameter would move out of it
	template<typename... FwdArgs>
	void Emit(FwdArgs&&... args)
	{
		++_emitDepth;
		detail::EmitToList("event callback", _listeners, args...);
		--_emitDepth;

		FinishDeferredRemovals();
	}

	// Erasing mid-dispatch would strand the walk's lookahead, so a listener leaving is only marked
	void RemoveListener(const ListenerHandle handle)
	{
		if (_emitDepth > 0)
		{
			handle->alive = false;
			_hasDead = true;

			return;
		}

		_listeners.erase(handle);
	}

	//NOTE: a dead node exists only inside a dispatch, and the bus cannot be destroyed in the middle of
	//its own - so at rest a non-empty list means live listeners
	bool HasListeners() const override { return !_listeners.empty(); }

#ifndef NDEBUG
	void ReportLeftoverListeners(const char* const kind, const char* const eventTypeName) const override
	{
		for (const auto& listener: _listeners)
		{
			detail::ReportLeftoverListener(kind, eventTypeName, listener.origin);
		}
	}
#endif

private:
	// Only the outermost Emit may sweep: a nested one is still walking what this would free.
	void FinishDeferredRemovals()
	{
		if (_emitDepth > 0 || !_hasDead)
		{
			return;
		}

		std::erase_if(_listeners, std::not_fn(detail::IsAlive));
		_hasDead = false;
	}

	ListenerList _listeners;
	int _emitDepth{};
	bool _hasDead{};
};

class BaseKeyedEvent
{
public:
	virtual ~BaseKeyedEvent() = default;
	virtual bool HasListeners() const = 0;
#ifndef NDEBUG
	virtual void ReportLeftoverListeners(const char* kind, const char* eventTypeName) const = 0;
#endif
};

template<typename KeyT, typename... Args>
class KeyedEvent final : public BaseKeyedEvent
{
public:
	using callbackType = std::function<void(Args...)>;
	using ListenerList = std::list<detail::Listener<callbackType>>;
	using ListenerMap = std::unordered_map<KeyT, ListenerList>;
	using ListenerHandle = ListenerList::iterator;

	ListenerHandle AddListener(const KeyT& key, callbackType callback, const std::source_location& origin)
	{
		auto& listeners = _listeners[key];
		listeners.emplace_back(std::move(callback), origin);
		return std::prev(listeners.end());
	}

	// Copies rather than forwards: several listeners share one payload, and the first would move out of it
	template<typename... FwdArgs>
	void Emit(const KeyT& key, FwdArgs&&... args)
	{
		if (const auto it = _listeners.find(key); it != _listeners.end())
		{
			//NOTE: the list is bound by reference before the walk - a listener added under a new key
			//mid-dispatch can rehash the map, which invalidates its iterators but not its elements
			ListenerList& listeners = it->second;

			++_emitDepth;
			detail::EmitToList("keyed event callback", listeners, args...);
			--_emitDepth;
		}

		FinishDeferredRemovals();
	}

	// Marked rather than erased, because a dispatch may be walking this list
	void RemoveListener(const KeyT& key, ListenerHandle handle)
	{
		if (const auto it = _listeners.find(key); it != _listeners.end())
		{
			if (_emitDepth > 0)
			{
				handle->alive = false;
				_hasDead = true;

				return;
			}

			it->second.erase(handle);

			//NOTE: keys are per-instance (a tank's uuid, a player slot) - a key whose last listener left
			//would otherwise sit in the map for the rest of the match
			if (it->second.empty())
			{
				_listeners.erase(it);
			}
		}
	}

	bool HasListeners() const override { return !_listeners.empty(); }

#ifndef NDEBUG
	void ReportLeftoverListeners(const char* const kind, const char* const eventTypeName) const override
	{
		for (const auto& [key, listeners]: _listeners)
		{
			for (const auto& listener: listeners)
			{
				detail::ReportLeftoverListener(kind, eventTypeName, listener.origin);
			}
		}
	}
#endif

private:
	// One counter for the whole map: a dispatch on any key must hold off every sweep, since the
	// callback it runs is free to unsubscribe under a different key.
	void FinishDeferredRemovals()
	{
		if (_emitDepth > 0 || !_hasDead)
		{
			return;
		}

		for (auto& entry: _listeners)
		{
			std::erase_if(entry.second, std::not_fn(detail::IsAlive));
		}

		std::erase_if(_listeners, [](const auto& entry) { return entry.second.empty(); });

		_hasDead = false;
	}

	ListenerMap _listeners;
	int _emitDepth{};
	bool _hasDead{};
};

// enable_shared_from_this so every EventSubscription can hold the bus alive for its own lifetime
class EventSystem final : public std::enable_shared_from_this<EventSystem>
{
	// The map key is the type identity itself, so a lookup hit is the right C++ type by construction
	struct EventInfo
	{
		std::unique_ptr<BaseEvent> event;
	};

	std::unordered_map<std::type_index, EventInfo> _events;

	template<typename EventType>
	Event<EventType>* GetTypedEvent()
	{
		if (const auto it = _events.find(std::type_index(typeid(EventType))); it != _events.end())
		{
			return static_cast<Event<EventType>*>(it->second.event.get());
		}

		return nullptr;
	}

	struct KeyedEventInfo
	{
		std::unique_ptr<BaseKeyedEvent> event;
	};

	std::unordered_map<std::type_index, KeyedEventInfo> _keyedEvents;

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

	~EventSystem();

	// EventType is deduced from the listener's parameter. Never pass `origin` by hand - as a default
	// argument it resolves at the call site, which is the point; only the overloads below forward it
	template<Callable CallableT>
	[[nodiscard]] EventSubscription AddListener(CallableT&& callback,
												const std::source_location& origin = std::source_location::current())
	{
		return callable_signature<std::decay_t<CallableT>>::call_add_listener(
				this, std::forward<CallableT>(callback), origin);
	}

	// Forwards `origin` rather than defaulting it again, which would report this header
	template<typename Class, typename EventT>
	[[nodiscard]] EventSubscription AddListener(Class* instance, void (Class::*method)(const EventT&),
												const std::source_location& origin = std::source_location::current())
	{
		return AddListener([instance, method](const EventT& event) { (instance->*method)(event); }, origin);
	}

	template<typename Class, typename EventT>
	[[nodiscard]] EventSubscription AddListener(Class* instance, void (Class::*method)(const EventT&) const,
												const std::source_location& origin = std::source_location::current())
	{
		return AddListener([instance, method](const EventT& event) { (instance->*method)(event); }, origin);
	}

	template<typename EventType, Callable CallableT>
	EventSubscription AddListenerImpl(CallableT&& callback, const std::source_location& origin)
	{
		const std::type_index key(typeid(EventType));

		if (const auto it = _events.find(key); it == _events.end())
		{
			_events.emplace(key, EventInfo{std::make_unique<Event<EventType>>()});
		}

		auto* event = GetTypedEvent<EventType>();
		const auto handle = event->AddListener(std::forward<CallableT>(callback), origin);

		return EventSubscription(shared_from_this(), [event, handle]()
		{
			event->RemoveListener(handle);
		});
	}

	// Key() disambiguates from a raw KeyT, which would let a string literal bind to the wrong overload
	template<typename KeyT, Callable CallableT>
	[[nodiscard]] EventSubscription AddListener(detail::EventKey<KeyT> key, CallableT&& callback,
												const std::source_location& origin = std::source_location::current())
	{
		return callable_signature<std::decay_t<CallableT>>::call_add_keyed_listener(
				this, key.value, std::forward<CallableT>(callback), origin);
	}

	template<typename KeyT, typename Class, typename EventT>
	[[nodiscard]] EventSubscription AddListener(detail::EventKey<KeyT> key, Class* instance,
												void (Class::*method)(const EventT&),
												const std::source_location& origin = std::source_location::current())
	{
		return AddListener(key, [instance, method](const EventT& event) { (instance->*method)(event); }, origin);
	}

	template<typename KeyT, typename Class, typename EventT>
	[[nodiscard]] EventSubscription AddListener(detail::EventKey<KeyT> key, Class* instance,
												void (Class::*method)(const EventT&) const,
												const std::source_location& origin = std::source_location::current())
	{
		return AddListener(key, [instance, method](const EventT& event) { (instance->*method)(event); }, origin);
	}

	template<typename KeyT, typename EventType, Callable CallableT>
	EventSubscription AddKeyedListenerImpl(const KeyT& key, CallableT&& callback, const std::source_location& origin)
	{
		const std::type_index typeKey(typeid(EventType));

		if (const auto it = _keyedEvents.find(typeKey); it == _keyedEvents.end())
		{
			_keyedEvents.emplace(typeKey, KeyedEventInfo{std::make_unique<KeyedEvent<KeyT, EventType>>()});
		}

		auto* event = GetTypedKeyedEvent<KeyT, EventType>();
		const auto handle = event->AddListener(key, std::forward<CallableT>(callback), origin);

		return EventSubscription(shared_from_this(), [event, key, handle]()
		{
			event->RemoveListener(key, handle);
		});
	}

	// No listener registered for this type is a legitimate no-op, not an error
	template<typename EventType>
	void EmitEvent(const EventType& eventInstance)
	{
		if (auto* typedEvent = GetTypedEvent<EventType>())
		{
			typedEvent->Emit(eventInstance);
		}
	}

	// EventKey<KeyT> is a parameter type of its own, so this never competes with the plain overload
	template<typename KeyT, typename EventType>
	void EmitEvent(detail::EventKey<KeyT> key, const EventType& eventInstance)
	{
		if (auto* typedEvent = GetTypedKeyedEvent<KeyT, EventType>())
		{
			typedEvent->Emit(key.value, eventInstance);
		}
	}
};
