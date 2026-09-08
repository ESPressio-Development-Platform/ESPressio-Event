#pragma once

#include <memory>

#include <ESPressio_Memory.hpp>
#include <ESPressio_ThreadSafeObservable.hpp>
#include "ESPressio_IEventTransportManagerObserver.hpp"

namespace ESPressio::Event {

/// <summary>Thread-safe observable used internally to fan out Event transport-manager notifications.</summary>
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes known bases + sizeof(std::enable_shared_from_this<ThreadSafeObservable>) + 4 bytes known members + sizeof(std::recursive_mutex) [0 bytes dynamic allocation]
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 4 bytes known bases + sizeof(std::enable_shared_from_this<ThreadSafeObservable>) + 4 bytes known members + sizeof(std::recursive_mutex) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class EventTransportManagerObservable final : public Observable::ThreadSafeObservable {
public:
    /// <summary>Invokes a callback for each registered Event transport-manager observer.</summary>
    /// <typeparam name="TCallback">Callable accepting an IEventTransportManagerObserver pointer.</typeparam>
    /// <remarks>
    /// Returns immediately through the lock-free observer-count fast path when no observer is registered, avoiding shared
    /// lifetime acquisition, notification locking, and callback machinery on the transport hot path.
    /// </remarks>
    template<typename TCallback>
    void Notify(TCallback&& callback) {
        if (!HasObservers()) return;
        ExecuteNotification([&](NotificationContext& notification) {
            notification.WithObservers<IEventTransportManagerObserver>(
                [&](IEventTransportManagerObserver* observer) {
                    try { callback(observer); } catch (...) {}
                });
        });
    }
};

/// <summary>Creates a shared Event transport-manager observable whose object and control block prefer external memory.</summary>
inline std::shared_ptr<EventTransportManagerObservable> CreateEventTransportManagerObservable() {
    return System::Memory::MakeShared<
        EventTransportManagerObservable,
        System::Memory::MemoryPolicy::ExternalPreferred
    >();
}

}
