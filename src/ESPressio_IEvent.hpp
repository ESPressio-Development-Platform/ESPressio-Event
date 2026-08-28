#pragma once

#include <cstdint>

#include <ESPressio_ClockTypes.hpp>

#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventTransportTypes.hpp"
#include "ESPressio_EventTypeKey.hpp"

namespace ESPressio {
namespace Event {

/// <summary>Default public time representation used by Event APIs.</summary>
using EventTime = Timing::DefaultClockTime;

/// <summary>Base contract for locally dispatchable and transport-routable events.</summary>
/// <remarks>Concrete events normally derive through TypedEvent or SerializableEvent so a stable RTTI-free type key is supplied automatically.</remarks>
class IEvent {
public:
    virtual ~IEvent() = default;

    /// <summary>Adds one intrusive lifetime reference to this event.</summary>
    virtual void __ref() noexcept = 0;
    /// <summary>Releases one intrusive lifetime reference and may destroy the event when the count reaches zero.</summary>
    virtual void __unref() noexcept = 0;
    /// <summary>Records that the event has entered dispatch.</summary>
    virtual void __dispatch() = 0;
    /// <summary>Associates routing/transport context with this dispatch.</summary>
    virtual void __setDispatchContext(const EventDispatchContext& context) = 0;
    /// <summary>Returns the routing/transport context associated with this dispatch.</summary>
    virtual EventDispatchContext __getDispatchContext() const = 0;

    /// <summary>Returns the compiler-backed local process identity used for routing and listener dispatch.</summary>
    /// <remarks>Every concrete routable Event must provide a stable EventTypeKey; use TypedEvent&lt;TDerived&gt; or SerializableEvent&lt;TDerived&gt; for the standard CRTP path.</remarks>
    virtual EventTypeKey __getTypeKey() const noexcept = 0;

    /// <summary>Queues the event for normal FIFO-style processing at the specified priority.</summary>
    virtual void Queue(
        EventPriority priority = EventPriority::Normal
    ) = 0;

    /// <summary>Stacks the event for priority processing ahead of queued events at the specified priority.</summary>
    virtual void Stack(
        EventPriority priority = EventPriority::Normal
    ) = 0;

    /// <summary>Returns the recorded dispatch timestamp in canonical nanoseconds, or zero before dispatch.</summary>
    virtual uint64_t GetDispatchTimeNanoseconds() const = 0;
    /// <summary>Returns elapsed nanoseconds since dispatch, or zero before dispatch.</summary>
    virtual uint64_t GetTimeSinceDispatchNanoseconds() const = 0;
};

} // namespace Event
} // namespace ESPressio
