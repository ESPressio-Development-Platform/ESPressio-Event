#pragma once

#include <ESPressio_IObserver.hpp>
#include "ESPressio_EventTransportTypes.hpp"
#include "ESPressio_IEvent.hpp"

namespace ESPressio::Event {

/// <summary>Observes Events after the local EventManager has dispatched them.</summary>
class IEventManagerObserver : public virtual Observable::IObserver {
public:
    virtual ~IEventManagerObserver() = default;

    /// <summary>Called after an Event is dispatched locally, including its dispatch method, priority, and transport-origin context.</summary>
    /// <param name="event">Non-owning Event pointer valid for the duration of the callback.</param>
    /// <param name="dispatchMethod">Queue or stack dispatch method used for the Event.</param>
    /// <param name="priority">Priority assigned to the Event.</param>
    /// <param name="context">Origin and transport-routing context carried by the Event.</param>
    virtual void OnEventDispatched(
        IEvent* event,
        EventDispatchMethod dispatchMethod,
        EventPriority priority,
        const EventDispatchContext& context
    ) {}
};

}
