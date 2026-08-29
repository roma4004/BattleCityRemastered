#pragma once

// NOTE: 65 TUs include this, so each include here is paid 65 times. No <ranges> on purpose: a
// `| views::` pipeline costs ~52k lines, a projection on an <algorithm> range algorithm does not.
// Keep pipelines, and anything needing <iostream>/<cassert>, in EventSystem.cpp.
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

// nullptr `what` = non-std exception; origin.line() == 0 = release build.
void ReportListenerException(const char* context, const char* what, const std::source_location& origin) noexcept;

// A throwing listener is reported by its registration site - the only identity a type-erased
// callback has - and every remaining listener still gets its event.
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

// Shared by both dispatch paths. A callback may unsubscribe anyone - itself, or a listener further
// down this very list. Neither frees a node here: mid-dispatch RemoveListener only clears `alive`
// (see Event::RemoveListener), so the lookahead below can never be left pointing at released memory.
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
	{
	}

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
// Defined in EventSystem.cpp, same reason as ReportListenerException above.
void ReportLeftoverListener(const char* kind, const char* eventTypeName,
							const std::source_location& origin) noexcept;
#endif
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
// Templated on Class only so the assert stays dependent and fires on instantiation, not on sight -
// same trick as callable_signature_args below, which exists for the same de-duplication reason.
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
#ifndef NDEBUG
	virtual void ReportLeftoverListeners(const char* kind, const char* eventTypeName) const = 0;
#endif
};

// std::list, not a name-keyed map: node addresses stay stable, so a subscription can carry
// its own iterator as identity.
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

	// args are copied into every listener, never forwarded: several listeners can share one event
	// type, and the first one's by-value std::function parameter would move out of a forwarded arg.
	template<typename... FwdArgs>
	void Emit(FwdArgs&&... args)
	{
		++_emitDepth;
		detail::EmitToList("event callback", _listeners, args...);
		--_emitDepth;

		CompactIfIdle();
	}

	// Erasing while a dispatch walks this list would strand its lookahead iterator, so a listener
	// leaving mid-dispatch is only marked. Handles stay valid either way - list nodes do not move.
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
	void CompactIfIdle()
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

// Parallel to BaseEvent/Event<Args...> - backs the keyed (per-instance) dispatch overloads.
// Kept as a fully separate hierarchy so the plain broadcast path above is never touched by this.
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

	// Copies rather than forwards, for the same reason as Event<Args...>::Emit - see its comment.
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

		CompactIfIdle();
	}

	// Deferred mid-dispatch, exactly as in Event::RemoveListener - the reason is the same loop.
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
	void CompactIfIdle()
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

	~EventSystem();

	// Auto-deduces EventType from the listener's parameter (type_index(typeid(EventType))).
	// Identity-free. Keep the returned EventSubscription alive while the listener should stay
	// registered. Never pass `origin` by hand: as a default argument it resolves at the call site,
	// which is the whole point - only the overloads below forward it on.
	template<Callable CallableT>
	[[nodiscard]] EventSubscription AddListener(CallableT&& callback,
												const std::source_location& origin = std::source_location::current())
	{
		return callable_signature<std::decay_t<CallableT>>::call_add_listener(
				this, std::forward<CallableT>(callback), origin);
	}

	// Sugar: AddListener(this, &Class::OnFoo) instead of a forwarding lambda. OnFoo takes the whole
	// event struct, e.g. void OnFoo(const FooEvent& event). Forwards origin - defaulting it again
	// would report this header.
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

	// internal implementation for the concrete EventType (used in callable_signature)
	template<typename EventType, Callable CallableT>
	EventSubscription AddListenerImpl(CallableT&& callback, const std::source_location& origin)
	{
		const std::type_index key(typeid(EventType));

		//create new if not exist
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

	// Keyed overload. Uses Key() to disambiguate from a raw KeyT, which would let a string literal
	// silently bind to the wrong overload.
	template<typename KeyT, Callable CallableT>
	[[nodiscard]] EventSubscription AddListener(detail::EventKey<KeyT> key, CallableT&& callback,
												const std::source_location& origin = std::source_location::current())
	{
		return callable_signature<std::decay_t<CallableT>>::call_add_keyed_listener(
				this, key.value, std::forward<CallableT>(callback), origin);
	}

	// Sugar, keyed variant - see the plain-overload sugar above.
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

	// internal implementation for the concrete keyed EventType (used in callable_signature)
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
