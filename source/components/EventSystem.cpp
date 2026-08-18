#include "components/EventSystem.h"

#include <cassert>
#include <iostream>

namespace detail
{
void ReportListenerException(const char* const context, const char* const what,
							 const std::source_location& origin) noexcept
{
	std::cerr << (what ? "Exception in " : "Unknown exception in ") << context << " registered at ";
	if (origin.line() > 0u)
	{
		std::cerr << origin.file_name() << ':' << origin.line();
	}
	else
	{
		std::cerr << "unknown location";
	}

	if (what)
	{
		std::cerr << ": " << what;
	}
	std::cerr << '\n';
	//NOTE: deliberately swallowed - the remaining listeners still get their event
}

#ifndef NDEBUG
void ReportLeftoverListener(const char* const kind, const char* const eventTypeName,
							const std::source_location& origin) noexcept
{
	std::cerr << "EventSystem: " << kind << " \"" << eventTypeName << "\" still has a listener registered at "
			<< origin.file_name() << ':' << origin.line() << " (" << origin.function_name()
			<< ") - its Unsubscribe()/RemoveListener() was never called.\n";
}
#endif
}// namespace detail

EventSystem::~EventSystem()
{
#ifndef NDEBUG
	// Anything still subscribed here never had its Unsubscribe() run - nothing will ever deliver to
	// it again. Currently unreachable: EventSubscription holds a shared_ptr to this bus, so no
	// listener can outlive it. Kept as a guard for any future non-RAII registration path.
	bool anyLeftoverListeners = false;

	for (const auto& [eventType, eventInfo]: _events)
	{
		if (eventInfo.event->HasListeners())
		{
			eventInfo.event->ReportLeftoverListeners("event type", eventType.name());
			anyLeftoverListeners = true;
		}
	}

	for (const auto& [eventType, keyedEventInfo]: _keyedEvents)
	{
		if (keyedEventInfo.event->HasListeners())
		{
			keyedEventInfo.event->ReportLeftoverListeners("keyed event type", eventType.name());
			anyLeftoverListeners = true;
		}
	}

	assert(!anyLeftoverListeners && "EventSystem: listeners still registered at destruction, see stderr");
#endif
}
