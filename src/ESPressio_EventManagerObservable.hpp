#pragma once

#include <memory>

#include <ESPressio_Memory.hpp>
#include <ESPressio_ThreadSafeObservable.hpp>
#include "ESPressio_IEventManagerObserver.hpp"

namespace ESPressio::Event {

/// <summary>Thread-safe observable used by EventManager to publish completed local dispatches.</summary>
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 96 bytes [ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Members: none (standalone empty object occupies 1 byte; an eligible empty base may be optimized to 0 bytes).
 * Total Memory: 96 bytes [ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
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
