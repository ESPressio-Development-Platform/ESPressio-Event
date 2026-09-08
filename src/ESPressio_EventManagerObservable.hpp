#pragma once

#include <memory>

#include <ESPressio_Memory.hpp>
#include <ESPressio_ThreadSafeObservable.hpp>
#include "ESPressio_IEventManagerObserver.hpp"

namespace ESPressio::Event {

/// <summary>Thread-safe observable used by EventManager to publish completed local dispatches.</summary>
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes known bases + sizeof(std::enable_shared_from_this<ThreadSafeObservable>) + 4 bytes known members + sizeof(std::recursive_mutex) [0 bytes dynamic allocation]
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 4 bytes known bases + sizeof(std::enable_shared_from_this<ThreadSafeObservable>) + 4 bytes known members + sizeof(std::recursive_mutex) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class EventManagerObservable final : public Observable::ThreadSafeObservable {
public:
    /// <summary>Notifies registered manager observers after an Event dispatch completes.</summary>
    /// <remarks>Observer exceptions are contained so they cannot alter EventManager dispatch state.</remarks>
    void EventDispatched(
        IEvent* event,
        EventDispatchMethod method,
        EventPriority priority,
        const EventDispatchContext& context
    ) {
        ExecuteNotification([&](NotificationContext& notification) {
            notification.WithObservers<IEventManagerObserver>([&](IEventManagerObserver* observer) {
                try { observer->OnEventDispatched(event, method, priority, context); }
                catch (...) {}
            });
        });
    }
};

/// <summary>Creates a shared EventManager observable whose object and control-block storage prefer external memory.</summary>
inline std::shared_ptr<EventManagerObservable> CreateEventManagerObservable() {
    return System::Memory::MakeShared<
        EventManagerObservable,
        System::Memory::MemoryPolicy::ExternalPreferred
    >();
}

}
