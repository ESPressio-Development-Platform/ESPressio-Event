#pragma once

#include <ESPressio_IObserver.hpp>

#include "ESPressio_EventTypes.hpp"
#include "ESPressio_IEvent.hpp"

namespace ESPressio::Event {

/// <summary>Observes Events after the local EventManager has dispatched them.</summary>
class IEventManagerObserver : public virtual Observable::IObserver {
public:
    virtual ~IEventManagerObserver() = default;

    /// <summary>Called after an Event is dispatched locally, including dispatch method, priority, and provenance.</summary>
    /// <param name="event">Non-owning Event pointer valid for the duration of the callback.</param>
    /// <param name="dispatchMethod">Queue or stack dispatch method used for the Event.</param>
    /// <param name="priority">Application/local-dispatch priority assigned to the Event.</param>
    /// <param name="context">Transport-independent local/remote provenance of this dispatch.</param>
    virtual void OnEventDispatched(
        IEvent* event,
        EventDispatchMethod dispatchMethod,
        EventPriority priority,
        const EventDispatchContext& context
    ) {
        (void)event;
        (void)dispatchMethod;
        (void)priority;
        (void)context;
    }
};

}
