#pragma once

#include <ESPressio_IObserver.hpp>

#include "ESPressio_EventTypes.hpp"
#include "ESPressio_IEvent.hpp"

namespace ESPressio::Event {

/// <summary>Observes Events after the local EventManager has dispatched them.</summary>
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members: none; polymorphic interface/object includes vptr storage where not supplied by a base.
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
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
