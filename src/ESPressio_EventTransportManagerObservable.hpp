#pragma once

#include <memory>
#include <ESPressio_ThreadSafeObservable.hpp>
#include "ESPressio_IEventTransportManagerObserver.hpp"

namespace ESPressio::Event {

/// <summary>Thread-safe observable used internally to fan out Event transport-manager notifications.</summary>
class EventTransportManagerObservable final : public Observable::ThreadSafeObservable {
public:
    /// <summary>Invokes a callback for each registered Event transport-manager observer.</summary>
    /// <typeparam name="TCallback">Callable accepting an IEventTransportManagerObserver pointer.</typeparam>
    /// <remarks>Exceptions raised by individual observers are contained so one observer cannot interrupt manager state transitions or other notifications.</remarks>
    template<typename TCallback>
    void Notify(TCallback&& callback) {
        ExecuteNotification([&](NotificationContext& notification) {
            notification.WithObservers<IEventTransportManagerObserver>(
                [&](IEventTransportManagerObserver* observer) {
                    try { callback(observer); } catch (...) {}
                });
        });
    }
};

/// <summary>Creates a shared Event transport-manager observable.</summary>
inline std::shared_ptr<EventTransportManagerObservable> CreateEventTransportManagerObservable() {
    return std::make_shared<EventTransportManagerObservable>();
}

}
