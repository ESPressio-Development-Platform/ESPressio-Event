#pragma once

#include <cstdint>

#include <ESPressio_ClockTypes.hpp>

#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventTransportTypes.hpp"
#include "ESPressio_EventTypeKey.hpp"

namespace ESPressio {
namespace Event {

using EventTime = Timing::DefaultClockTime;

class IEvent {
public:
    virtual ~IEvent() = default;

    virtual void __ref() noexcept = 0;
    virtual void __unref() noexcept = 0;
    virtual void __dispatch() = 0;
    virtual void __setDispatchContext(const EventDispatchContext& context) = 0;
    virtual EventDispatchContext __getDispatchContext() const = 0;

    /// Compiler-backed local process identity for routing/listener dispatch.
    /// Legacy Event<TTime> returns nullptr; TypedEvent/SerializableEvent return
    /// their concrete Event type key. RTTI-enabled builds may fall back for
    /// legacy events during migration.
    virtual EventTypeKey __getTypeKey() const noexcept = 0;

    virtual void Queue(
        EventPriority priority = EventPriority::Normal
    ) = 0;

    virtual void Stack(
        EventPriority priority = EventPriority::Normal
    ) = 0;

    virtual uint64_t GetDispatchTimeNanoseconds() const = 0;
    virtual uint64_t GetTimeSinceDispatchNanoseconds() const = 0;
};

} // namespace Event
} // namespace ESPressio
