#pragma once

#include <memory>

#include <ESPressio_Memory.hpp>
#include <ESPressio_ThreadSafeObservable.hpp>
#include "ESPressio_IEventTransportManagerObserver.hpp"

namespace ESPressio::Event {

/// <summary>Thread-safe observable used internally to fan out Event transport-manager notifications.</summary>
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 96 bytes [ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Members: none (standalone empty object occupies 1 byte; an eligible empty base may be optimized to 0 bytes).
 * Total Memory: 96 bytes [ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
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
