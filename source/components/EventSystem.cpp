#include "components/EventSystem.h"

#include <cassert>
#include "utils/Log.h"
#include <string>

namespace detail
{
void ReportListenerException(const char* const context, const char* const what,
							 const std::source_location& origin) noexcept
{
	std::string message = what ? "Exception in " : "Unknown exception in ";
	message += context;
	message += " registered at ";
	message += origin.line() > 0u
				   ? std::string{origin.file_name()} + ':' + std::to_string(origin.line())
				   : std::string{"unknown location"};
	if (what)
	{
		message += std::string{": "} + what;
	}

	Log::Error(message);
	//NOTE: deliberately swallowed - the remaining listeners still get their event
}

#ifndef NDEBUG
void ReportLeftoverListener(const char* const kind, const char* const eventTypeName,
							const std::source_location& origin) noexcept
{
	Log::Error(std::string{"EventSystem: "} + kind + " \"" + eventTypeName + "\" still has a listener registered at "
			   + origin.file_name() + ':' + std::to_string(origin.line()) + " (" + origin.function_name()
			   + ") - its Unsubscribe()/RemoveListener() was never called.");
}
#endif
}// namespace detail

EventSystem::~EventSystem()
{
#ifndef NDEBUG
	// Anything still subscribed here never had its Unsubscribe() run. Unreachable while every listener
	// comes in through an EventSubscription, since that one holds the bus alive
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
