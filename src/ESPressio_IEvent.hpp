#pragma once

#include <cstdint>

#include <ESPressio_ClockTypes.hpp>

#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventTypes.hpp"
#include "ESPressio_EventTypeKey.hpp"

namespace ESPressio {
namespace Event {

/// <summary>Default public time representation used by Event APIs.</summary>
using EventTime = Timing::DefaultClockTime;

/// <summary>Base contract for a locally dispatchable Event occurrence.</summary>
/// <remarks>
/// Concrete Events normally derive through TypedEvent or SerializableEvent so a stable RTTI-free
/// local type key is supplied automatically. Transport provenance is carried by dispatch context,
/// not retained intrinsically by the Event object.
/// </remarks>
class IEvent {
public:
    virtual ~IEvent() = default;

    /// <summary>Adds one intrusive lifetime reference to this Event.</summary>
    virtual void __ref() noexcept = 0;
    /// <summary>Releases one intrusive lifetime reference and may destroy the Event when the count reaches zero.</summary>
    virtual void __unref() noexcept = 0;
    /// <summary>Records that the Event has entered local dispatch.</summary>
    virtual void __dispatch() = 0;

    /// <summary>Returns the compiler-backed local-process identity used for listener dispatch.</summary>
    /// <remarks>Every concrete routable Event must provide a stable EventTypeKey; use TypedEvent&lt;TDerived&gt; or SerializableEvent&lt;TDerived&gt; for the standard CRTP path.</remarks>
    virtual EventTypeKey __getTypeKey() const noexcept = 0;

    /// <summary>Queues the Event for normal FIFO-style local processing at the specified priority.</summary>
    virtual void Queue(
        EventPriority priority = EventPriority::Normal
    ) = 0;

    /// <summary>Stacks the Event for local priority processing ahead of queued Events at the specified priority.</summary>
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
