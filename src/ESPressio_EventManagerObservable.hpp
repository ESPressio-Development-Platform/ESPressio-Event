#pragma once

#include <memory>
#include <ESPressio_ThreadSafeObservable.hpp>
#include "ESPressio_IEventManagerObserver.hpp"

namespace ESPressio::Event {

/// <summary>Thread-safe observable used by EventManager to publish completed local dispatches.</summary>
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

/// <summary>Creates a shared EventManager observable.</summary>
inline std::shared_ptr<EventManagerObservable> CreateEventManagerObservable() {
    return std::make_shared<EventManagerObservable>();
}

}
