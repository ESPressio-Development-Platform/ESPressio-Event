#pragma once

#include <algorithm>
#include <cstddef>
#include <mutex>

#include <ESPressio_Memory.hpp>
#include <ESPressio_Synchronization.hpp>

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
    virtual void RegisterReceiver(EventTypeKey type, IEventReceiver* receiver) = 0;
    /// <summary>Removes a receiver registration for the supplied RTTI-free event type key.</summary>
    virtual void UnregisterReceiver(EventTypeKey type, IEventReceiver* receiver) = 0;
};

/// <summary>Event receiver that redispatches queued events to receivers registered for each event type.</summary>
/// <remarks>
/// Receiver removal is safe during nested dispatch; inactive records are compacted when dispatch depth returns to zero.
/// Receiver-registry backing storage prefers external memory. Downstream admission is always non-blocking so one saturated
/// consumer cannot stall the global dispatcher. Receiver calls execute without the registry lock held so nested routing
/// and registry mutation cannot deadlock the registry.
/// </remarks>
class EventDispatcher : public EventReceiver, public IEventDispatcher {
private:
    struct ReceiverRecord {
        EventTypeKey Type = nullptr;
        IEventReceiver* Receiver = nullptr;
        bool Active = false;
    };

    using ReceiverStorage = System::Memory::Vector<
        ReceiverRecord,
        System::Memory::MemoryPolicy::ExternalPreferred
    >;

    ReceiverStorage _eventReceivers;
    mutable System::Synchronization::RecursiveMutex _eventReceiversMutex;
    std::size_t _dispatchDepth = 0;
    bool _needsCompaction = false;

    void CompactReceiversLocked() {
        _eventReceivers.erase(
            std::remove_if(
                _eventReceivers.begin(),
                _eventReceivers.end(),
                [](const ReceiverRecord& record) { return !record.Active; }
            ),
            _eventReceivers.end()
        );
        _needsCompaction = false;
    }

    void FinishReceiverDispatchLocked() {
        if (_dispatchDepth > 0) {
            --_dispatchDepth;
        }
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
        std::lock_guard<System::Synchronization::RecursiveMutex> lock(
            _eventReceiversMutex
        );
        if (_dispatchDepth > 0) {
            for (auto& record : _eventReceivers) {
                record.Active = false;
            }
            _needsCompaction = true;
        } else {
            _eventReceivers.clear();
        }
    }

    /// <summary>Drains received events and routes each one to matching typed receivers.</summary>
    void DispatchEvents() {
        WithEvents(
            [&](IEvent* event, EventDispatchMethod dispatchMethod, EventPriority priority) {
                event->__dispatch();
                OnEventDispatched(event, dispatchMethod, priority);

                const EventTypeKey eventType = event->__getTypeKey();
                if (eventType == nullptr) {
                    return;
                }

                std::size_t receiverCount = 0;
                {
                    std::lock_guard<System::Synchronization::RecursiveMutex> lock(
                        _eventReceiversMutex
                    );
                    ++_dispatchDepth;
                    receiverCount = _eventReceivers.size();
                }

                try {
                    for (std::size_t index = 0; index < receiverCount; ++index) {
                        IEventReceiver* receiver = nullptr;
                        {
                            std::lock_guard<System::Synchronization::RecursiveMutex> lock(
                                _eventReceiversMutex
                            );
                            if (index >= _eventReceivers.size()) {
                                continue;
                            }
                            const ReceiverRecord& record = _eventReceivers[index];
                            if (
                                record.Active &&
                                record.Receiver != nullptr &&
                                record.Type == eventType
                            ) {
                                receiver = record.Receiver;
                            }
                        }

                        if (receiver == nullptr) {
                            continue;
                        }

                        if (dispatchMethod == EventDispatchMethod::Queue) {
                            (void)receiver->TryQueueEvent(event, priority);
                        } else {
                            (void)receiver->TryStackEvent(event, priority);
                        }
                    }
                } catch (...) {
                    std::lock_guard<System::Synchronization::RecursiveMutex> lock(
                        _eventReceiversMutex
                    );
                    FinishReceiverDispatchLocked();
                    throw;
                }

                std::lock_guard<System::Synchronization::RecursiveMutex> lock(
                    _eventReceiversMutex
                );
                FinishReceiverDispatchLocked();
            }
        );
    }

public:
    EventDispatcher() = default;
    ~EventDispatcher() override {
        ClearEventReceivers();
    }

    void RegisterReceiver(
        EventTypeKey type,
        IEventReceiver* receiver
    ) override {
        if (type == nullptr || receiver == nullptr) {
            return;
        }
        std::lock_guard<System::Synchronization::RecursiveMutex> lock(
            _eventReceiversMutex
        );
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

    void UnregisterReceiver(
        EventTypeKey type,
        IEventReceiver* receiver
    ) override {
        std::lock_guard<System::Synchronization::RecursiveMutex> lock(
            _eventReceiversMutex
        );
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
