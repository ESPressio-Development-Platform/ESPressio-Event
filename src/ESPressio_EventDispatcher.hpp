#pragma once

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <utility>

#include <ESPressio_Memory.hpp>
#include <ESPressio_Synchronization.hpp>

#include "ESPressio_IEvent.hpp"
#include "ESPressio_EventReceiver.hpp"
#include "ESPressio_EventTypeKey.hpp"

namespace ESPressio {
namespace Event {

/// <summary>Contract for routing Event types to registered receivers.</summary>
/**
 * ESPressio Memory Audit
 * Members: none; polymorphic interface/object includes vptr storage where not supplied by a base.
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class IEventDispatcher {
public:
    virtual ~IEventDispatcher() = default;
    virtual void RegisterReceiver(EventTypeKey type, IEventReceiver* receiver) = 0;
    virtual void UnregisterReceiver(EventTypeKey type, IEventReceiver* receiver) = 0;
};

/// <summary>Event receiver that redispatches queued Events to receivers registered for each Event type.</summary>
/// <remarks>
/// Receiver removal is safe during nested dispatch. Fan-out admission is always non-blocking, and downstream receiver
/// calls execute without the registry lock held. Dispatch provenance is propagated beside each Event reference rather
/// than being retained by the Event itself. A caller may supply a lightweight post-fanout completion action; this is
/// intended for lifecycle bookkeeping such as scheduling asynchronous observation, never for consumer work.
/// </remarks>
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes known bases + 59 bytes known members + sizeof(EventCollection) + sizeof(EventCollection) [0 bytes dynamic allocation]
 * Members:
 * - _eventReceivers (ReceiverStorage): sizeof(ReceiverStorage) [0 bytes dynamic allocation]
 * - _eventReceiversMutex (System::Synchronization::RecursiveMutex): sizeof(System::Synchronization::RecursiveMutex) [0 bytes dynamic allocation]
 * - _dispatchDepth (std::size_t): 4 bytes [0 bytes dynamic allocation]
 * - _needsCompaction (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 4 bytes known bases + 4 bytes known bases + 59 bytes known members + sizeof(EventCollection) + sizeof(EventCollection) + 5 bytes known members + sizeof(ReceiverStorage) + sizeof(System::Synchronization::RecursiveMutex) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class EventDispatcher : public EventReceiver, public IEventDispatcher {
private:
/**
 * ESPressio Memory Audit
 * Members:
 * - Type (EventTypeKey): sizeof(EventTypeKey) [0 bytes dynamic allocation]
 * - Receiver (IEventReceiver*): 4 bytes [0 bytes dynamic allocation]
 * - Active (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 5 bytes known members + sizeof(EventTypeKey) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
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
        if (_dispatchDepth > 0) --_dispatchDepth;
        if (_dispatchDepth == 0 && _needsCompaction) CompactReceiversLocked();
    }

    template<typename TCompletion>
    void DispatchEventsWithCompletion(TCompletion&& completion) {
        WithEvents(
            [&](IEvent* event,
                EventDispatchMethod dispatchMethod,
                EventPriority priority,
                const EventDispatchContext& context) {
                event->__dispatch();

                const EventTypeKey eventType = event->__getTypeKey();
                if (eventType != nullptr) {
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
                                if (index >= _eventReceivers.size()) continue;
                                const ReceiverRecord& record = _eventReceivers[index];
                                if (
                                    record.Active &&
                                    record.Receiver != nullptr &&
                                    record.Type == eventType
                                ) {
                                    receiver = record.Receiver;
                                }
                            }

                            if (receiver == nullptr) continue;
                            if (dispatchMethod == EventDispatchMethod::Queue) {
                                (void)receiver->TryQueueEvent(event, priority, context);
                            } else {
                                (void)receiver->TryStackEvent(event, priority, context);
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

                completion(event, dispatchMethod, priority, context);
            }
        );
    }

protected:
    void ClearEventReceivers() {
        std::lock_guard<System::Synchronization::RecursiveMutex> lock(_eventReceiversMutex);
        if (_dispatchDepth > 0) {
            for (auto& record : _eventReceivers) record.Active = false;
            _needsCompaction = true;
        } else {
            _eventReceivers.clear();
        }
    }

    /// <summary>Drains and routes pending Events without a dispatch-completion action.</summary>
    void DispatchEvents() {
        DispatchEventsWithCompletion(
            [](IEvent*, EventDispatchMethod, EventPriority, const EventDispatchContext&) {}
        );
    }

    /// <summary>Drains and routes pending Events, then invokes a lightweight completion action for each dispatch.</summary>
    template<typename TCompletion>
    void DispatchEvents(TCompletion&& completion) {
        DispatchEventsWithCompletion(std::forward<TCompletion>(completion));
    }

public:
    EventDispatcher() = default;
    ~EventDispatcher() override { ClearEventReceivers(); }

    void RegisterReceiver(EventTypeKey type, IEventReceiver* receiver) override {
        if (type == nullptr || receiver == nullptr) return;
        std::lock_guard<System::Synchronization::RecursiveMutex> lock(_eventReceiversMutex);
        for (const auto& record : _eventReceivers) {
            if (record.Active && record.Type == type && record.Receiver == receiver) return;
        }
        _eventReceivers.emplace_back(ReceiverRecord{type, receiver, true});
    }

    void UnregisterReceiver(EventTypeKey type, IEventReceiver* receiver) override {
        std::lock_guard<System::Synchronization::RecursiveMutex> lock(_eventReceiversMutex);
        for (auto& record : _eventReceivers) {
            if (record.Active && record.Type == type && record.Receiver == receiver) {
                record.Active = false;
                if (_dispatchDepth > 0) _needsCompaction = true;
                else CompactReceiversLocked();
                return;
            }
        }
    }
};

} // namespace Event
} // namespace ESPressio
