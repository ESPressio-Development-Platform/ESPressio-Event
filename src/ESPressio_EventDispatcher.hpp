#pragma once

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <vector>

#include "ESPressio_IEvent.hpp"
#include "ESPressio_EventReceiver.hpp"
#include "ESPressio_EventTypeKey.hpp"

namespace ESPressio {
namespace Event {

/// <summary>Contract for routing event types to registered receivers.</summary>
class IEventDispatcher {
public:
    virtual ~IEventDispatcher() = default;

    /// <summary>Registers a receiver for the supplied RTTI-free event type key.</summary>
    virtual void RegisterReceiver(
        EventTypeKey type,
        IEventReceiver* receiver
    ) = 0;

    /// <summary>Removes a receiver registration for the supplied event type key.</summary>
    virtual void UnregisterReceiver(
        EventTypeKey type,
        IEventReceiver* receiver
    ) = 0;
};

/// <summary>Event receiver that redispatches queued events to receivers registered for each event type.</summary>
/// <remarks>Receiver removal is safe during nested dispatch; inactive records are compacted when dispatch depth returns to zero.</remarks>
class EventDispatcher : public EventReceiver, public IEventDispatcher {
private:
    struct ReceiverRecord {
        EventTypeKey Type = nullptr;
        IEventReceiver* Receiver = nullptr;
        bool Active = false;
    };

    std::vector<ReceiverRecord> _eventReceivers;
    mutable std::recursive_mutex _eventReceiversMutex;
    std::size_t _dispatchDepth = 0;
    bool _needsCompaction = false;

    void CompactReceiversLocked() {
        _eventReceivers.erase(
            std::remove_if(
                _eventReceivers.begin(),
                _eventReceivers.end(),
                [](const ReceiverRecord& record) {
                    return !record.Active;
                }
            ),
            _eventReceivers.end()
        );
        _needsCompaction = false;
    }

    void FinishReceiverDispatchLocked() {
        if (_dispatchDepth > 0) --_dispatchDepth;
        if (_dispatchDepth == 0 && _needsCompaction) {
            CompactReceiversLocked();
        }
    }

protected:
    /// <summary>Hook invoked after an event has been marked dispatched and before it is routed to typed receivers.</summary>
    virtual void OnEventDispatched(
        IEvent*,
        EventDispatchMethod,
        EventPriority
    ) {}

    /// <summary>Removes all registered typed receivers, deferring physical compaction during active dispatch.</summary>
    void ClearEventReceivers() {
        std::lock_guard<std::recursive_mutex> lock(_eventReceiversMutex);
        if (_dispatchDepth > 0) {
            for (auto& record : _eventReceivers) record.Active = false;
            _needsCompaction = true;
        } else {
            _eventReceivers.clear();
        }
    }

    /// <summary>Drains received events and routes each one to matching typed receivers.</summary>
    void DispatchEvents() {
        WithEvents(
            [&](
                IEvent* event,
                EventDispatchMethod dispatchMethod,
                EventPriority priority
            ) {
                event->__dispatch();
                OnEventDispatched(event, dispatchMethod, priority);

                const EventTypeKey eventType = event->__getTypeKey();
                if (eventType == nullptr) {
                    // RTTI-free routing requires concrete events to use TypedEvent
                    // (SerializableEvent already does). A legacy Event<TTime>
                    // deliberately has no routable type identity.
                    return;
                }

                std::lock_guard<std::recursive_mutex> lock(_eventReceiversMutex);
                ++_dispatchDepth;
                const std::size_t receiverCount = _eventReceivers.size();

                try {
                    for (std::size_t index = 0; index < receiverCount; ++index) {
                        const ReceiverRecord& record = _eventReceivers[index];
                        if (
                            !record.Active ||
                            record.Receiver == nullptr ||
                            record.Type != eventType
                        ) {
                            continue;
                        }

                        if (dispatchMethod == EventDispatchMethod::Queue) {
                            record.Receiver->QueueEvent(event, priority);
                        } else {
                            record.Receiver->StackEvent(event, priority);
                        }
                    }
                } catch (...) {
                    FinishReceiverDispatchLocked();
                    throw;
                }

                FinishReceiverDispatchLocked();
            }
        );
    }

public:
    EventDispatcher() = default;

    ~EventDispatcher() override {
        ClearEventReceivers();
    }

    /// <inheritdoc/>
    void RegisterReceiver(
        EventTypeKey type,
        IEventReceiver* receiver
    ) override {
        if (type == nullptr || receiver == nullptr) return;

        std::lock_guard<std::recursive_mutex> lock(_eventReceiversMutex);
        for (const auto& record : _eventReceivers) {
            if (
                record.Active &&
                record.Type == type &&
                record.Receiver == receiver
            ) {
                return;
            }
        }

        _eventReceivers.emplace_back(ReceiverRecord{type, receiver, true});
    }

    /// <inheritdoc/>
    void UnregisterReceiver(
        EventTypeKey type,
        IEventReceiver* receiver
    ) override {
        std::lock_guard<std::recursive_mutex> lock(_eventReceiversMutex);
        for (auto& record : _eventReceivers) {
            if (
                record.Active &&
                record.Type == type &&
                record.Receiver == receiver
            ) {
                record.Active = false;
                if (_dispatchDepth > 0) {
                    _needsCompaction = true;
                } else {
                    CompactReceiversLocked();
                }
                return;
            }
        }
    }
};

} // namespace Event
} // namespace ESPressio
