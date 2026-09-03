#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <utility>

#include <ESPressio_Memory.hpp>
#include <ESPressio_Synchronization.hpp>

#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventTypes.hpp"
#include "ESPressio_IEvent.hpp"

#ifndef ESPRESSIO_EVENT_DEFAULT_MAX_PENDING_EVENT_COUNT
    #define ESPRESSIO_EVENT_DEFAULT_MAX_PENDING_EVENT_COUNT 64
#endif

namespace ESPressio {
namespace Event {

enum class EventQueueOverflowPolicy : uint8_t {
    BlockProducer,
    RejectIncoming,
    DropOldest,
    DropLowestPriority
};

enum class EventCollectionCapacityPolicy : uint8_t {
    Retain,
    ShrinkWhenUnderutilized,
    ReleaseAfterDrain
};

/// <summary>Contract for an object that can retain queued or stacked Event references with dispatch provenance.</summary>
class IEventReceiver {
public:
    virtual ~IEventReceiver() = default;

    /// <summary>Queues an Event using the receiver's configured producer-overflow policy.</summary>
    virtual void QueueEvent(
        IEvent* event,
        EventPriority priority = EventPriority::Normal,
        EventDispatchContext context = {}
    ) = 0;

    /// <summary>Stacks an Event using the receiver's configured producer-overflow policy.</summary>
    virtual void StackEvent(
        IEvent* event,
        EventPriority priority = EventPriority::Normal,
        EventDispatchContext context = {}
    ) = 0;

    /// <summary>Attempts to queue an Event without ever blocking the caller.</summary>
    /// <returns>True when the Event and its dispatch context were admitted; false when rejected.</returns>
    virtual bool TryQueueEvent(
        IEvent* event,
        EventPriority priority = EventPriority::Normal,
        EventDispatchContext context = {}
    ) = 0;

    /// <summary>Attempts to stack an Event without ever blocking the caller.</summary>
    /// <returns>True when the Event and its dispatch context were admitted; false when rejected.</returns>
    virtual bool TryStackEvent(
        IEvent* event,
        EventPriority priority = EventPriority::Normal,
        EventDispatchContext context = {}
    ) = 0;
};

/// <summary>Thread-safe priority receiver retaining Event references and dispatch provenance until processing.</summary>
/// <remarks>
/// Direct QueueEvent/StackEvent calls honour the configured producer-overflow policy, including BlockProducer.
/// Broker fan-out should use TryQueueEvent/TryStackEvent so a saturated consumer can never block a global dispatcher.
/// Queue entries are FIFO, stack entries LIFO, higher priorities drain first, and backing storage prefers external memory.
/// Dispatch provenance travels beside the retained reference and is never written into the Event object itself.
/// </remarks>
class EventReceiver : public IEventReceiver {
private:
    struct PendingEvent {
        IEvent* event = nullptr;
        uint64_t sequence = 0;
        EventDispatchContext context{};
    };

    static constexpr auto ExternalPreferred =
        System::Memory::MemoryPolicy::ExternalPreferred;
    static constexpr uint32_t CapacityWaitRecheckMilliseconds = 10;
    static constexpr size_t PriorityCount =
        static_cast<size_t>(EventPriority::High) + 1;
    static constexpr size_t CapacitySampleCount = 16;

    using EventDispatchCollection =
        System::Memory::Vector<PendingEvent, ExternalPreferred>;
    using EventCollection =
        std::array<EventDispatchCollection, PriorityCount>;

    mutable System::Synchronization::Mutex _eventsMutex;
    std::unique_ptr<System::Synchronization::ISignal> _capacityAvailable =
        System::Synchronization::CreateBinarySignal(false);
    EventCollection _priorityQueues;
    EventCollection _priorityStacks;
    size_t _pendingEventCount = 0;
    size_t _processingEventCount = 0;
    size_t _peakPendingEventCount = 0;
    size_t _maximumPendingEventCount = ESPRESSIO_EVENT_DEFAULT_MAX_PENDING_EVENT_COUNT;
    EventQueueOverflowPolicy _overflowPolicy = EventQueueOverflowPolicy::BlockProducer;
    EventCollectionCapacityPolicy _capacityPolicy = EventCollectionCapacityPolicy::Retain;
    size_t _minimumRetainedCapacity = 4;
    size_t _capacityExcessFactor = 2;
    std::array<size_t, CapacitySampleCount> _recentDrainSizes{};
    size_t _recentDrainIndex = 0;
    size_t _recentDrainCount = 0;
    uint64_t _nextSequence = 0;
    uint64_t _rejectedEventCount = 0;
    uint64_t _droppedEventCount = 0;
    bool _acceptingPendingEvents = true;

    static constexpr size_t PriorityIndex(EventPriority priority) {
        return static_cast<size_t>(priority);
    }

    void SignalCapacityChange() noexcept {
        if (_capacityAvailable != nullptr) {
            (void)_capacityAvailable->Give();
        }
    }

    size_t RetainedEventCountLocked() const {
        return _pendingEventCount + _processingEventCount;
    }

    static uint64_t NextSequence(uint64_t& sequence) {
        const uint64_t result = sequence;
        if (sequence != std::numeric_limits<uint64_t>::max()) {
            ++sequence;
        }
        return result;
    }

    void RecordDrainSizeLocked(size_t size) {
        _recentDrainSizes[_recentDrainIndex] = size;
        _recentDrainIndex = (_recentDrainIndex + 1) % CapacitySampleCount;
        _recentDrainCount = std::min(_recentDrainCount + 1, CapacitySampleCount);
    }

    size_t RecentPeakLocked() const {
        size_t peak = 0;
        for (size_t index = 0; index < _recentDrainCount; ++index) {
            peak = std::max(peak, _recentDrainSizes[index]);
        }
        return peak;
    }

    void ApplyCapacityPolicyLocked(EventDispatchCollection& collection) {
        if (_capacityPolicy == EventCollectionCapacityPolicy::Retain) {
            return;
        }

        if (_capacityPolicy == EventCollectionCapacityPolicy::ReleaseAfterDrain) {
            if (collection.empty()) {
                EventDispatchCollection replacement;
                collection.swap(replacement);
                return;
            }
            EventDispatchCollection replacement;
            replacement.reserve(collection.size());
            replacement.insert(
                replacement.end(),
                std::make_move_iterator(collection.begin()),
                std::make_move_iterator(collection.end())
            );
            collection.swap(replacement);
            return;
        }

        const size_t recentPeak = RecentPeakLocked();
        const size_t target = std::max(
            _minimumRetainedCapacity,
            recentPeak > std::numeric_limits<size_t>::max() / _capacityExcessFactor
                ? std::numeric_limits<size_t>::max()
                : recentPeak * _capacityExcessFactor
        );
        if (collection.capacity() > target) {
            EventDispatchCollection replacement;
            replacement.reserve(std::max(target, collection.size()));
            replacement.insert(
                replacement.end(),
                std::make_move_iterator(collection.begin()),
                std::make_move_iterator(collection.end())
            );
            collection.swap(replacement);
        }
    }

    PendingEvent RemoveOldestLocked() {
        EventDispatchCollection* selected = nullptr;
        size_t selectedIndex = 0;
        uint64_t selectedSequence = std::numeric_limits<uint64_t>::max();

        auto consider = [&](EventCollection& collections) {
            for (auto& collection : collections) {
                for (size_t index = 0; index < collection.size(); ++index) {
                    if (collection[index].sequence < selectedSequence) {
                        selected = &collection;
                        selectedIndex = index;
                        selectedSequence = collection[index].sequence;
                    }
                }
            }
        };

        consider(_priorityQueues);
        consider(_priorityStacks);
        if (selected == nullptr) {
            return {};
        }

        PendingEvent removed = (*selected)[selectedIndex];
        selected->erase(selected->begin() + selectedIndex);
        --_pendingEventCount;
        return removed;
    }

    PendingEvent RemoveLowestPriorityLocked(bool& removed) {
        for (size_t priorityID = 0; priorityID < PriorityCount; ++priorityID) {
            auto removeFrom = [&](EventCollection& collections) {
                EventDispatchCollection& collection = collections[priorityID];
                if (collection.empty()) {
                    return PendingEvent{};
                }
                PendingEvent result = collection.front();
                collection.erase(collection.begin());
                removed = true;
                --_pendingEventCount;
                return result;
            };

            PendingEvent result = removeFrom(_priorityQueues);
            if (removed) return result;
            result = removeFrom(_priorityStacks);
            if (removed) return result;
        }
        return {};
    }

    bool AddEvent(
        IEvent* event,
        EventPriority priority,
        EventDispatchMethod method,
        EventDispatchContext context,
        bool allowBlocking
    ) {
        if (event == nullptr) return false;

        event->__dispatch();
        event->__ref();
        IEvent* displacedEvent = nullptr;
        bool accepted = false;

        try {
            std::unique_lock<System::Synchronization::Mutex> lock(_eventsMutex);
            if (!_acceptingPendingEvents) {
                ++_rejectedEventCount;
                lock.unlock();
                event->__unref();
                return false;
            }

            while (
                _maximumPendingEventCount > 0 &&
                RetainedEventCountLocked() >= _maximumPendingEventCount
            ) {
                if (!_acceptingPendingEvents) {
                    ++_rejectedEventCount;
                    lock.unlock();
                    event->__unref();
                    return false;
                }

                switch (_overflowPolicy) {
                    case EventQueueOverflowPolicy::BlockProducer:
                        if (!allowBlocking) {
                            ++_rejectedEventCount;
                            lock.unlock();
                            event->__unref();
                            return false;
                        }
                        lock.unlock();
                        if (_capacityAvailable != nullptr) {
                            (void)_capacityAvailable->Wait(CapacityWaitRecheckMilliseconds);
                        }
                        lock.lock();
                        continue;

                    case EventQueueOverflowPolicy::RejectIncoming:
                        ++_rejectedEventCount;
                        lock.unlock();
                        event->__unref();
                        return false;

                    case EventQueueOverflowPolicy::DropOldest:
                        displacedEvent = RemoveOldestLocked().event;
                        if (displacedEvent == nullptr) {
                            ++_rejectedEventCount;
                            lock.unlock();
                            event->__unref();
                            return false;
                        }
                        ++_droppedEventCount;
                        break;

                    case EventQueueOverflowPolicy::DropLowestPriority: {
                        bool removed = false;
                        PendingEvent displaced = RemoveLowestPriorityLocked(removed);
                        if (!removed) {
                            ++_rejectedEventCount;
                            lock.unlock();
                            event->__unref();
                            return false;
                        }
                        displacedEvent = displaced.event;
                        ++_droppedEventCount;
                        break;
                    }
                }
                break;
            }

            EventCollection& collections =
                method == EventDispatchMethod::Queue ? _priorityQueues : _priorityStacks;
            collections[PriorityIndex(priority)].push_back(
                PendingEvent{event, NextSequence(_nextSequence), context}
            );
            ++_pendingEventCount;
            _peakPendingEventCount = std::max(
                _peakPendingEventCount,
                RetainedEventCountLocked()
            );
            accepted = true;
        } catch (...) {
            event->__unref();
            if (displacedEvent != nullptr) displacedEvent->__unref();
            throw;
        }

        if (displacedEvent != nullptr) displacedEvent->__unref();
        if (accepted) EventAdded();
        return accepted;
    }

    template<typename TCallback>
    void ProcessCollection(
        EventCollection& collections,
        EventPriority priority,
        EventDispatchMethod method,
        TCallback& callback
    ) {
        EventDispatchCollection pending;
        const size_t priorityIndex = PriorityIndex(priority);
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
            EventDispatchCollection& source = collections[priorityIndex];
            if (source.empty()) return;
            pending.swap(source);
            _pendingEventCount -= pending.size();
            _processingEventCount += pending.size();
            RecordDrainSizeLocked(pending.size());
        }

        class ProcessingGuard final {
            EventReceiver& _receiver;
            size_t _count;
        public:
            ProcessingGuard(EventReceiver& receiver, size_t count)
                : _receiver(receiver), _count(count) {}
            ~ProcessingGuard() {
                {
                    std::lock_guard<System::Synchronization::Mutex> lock(_receiver._eventsMutex);
                    _receiver._processingEventCount -= _count;
                }
                _receiver.SignalCapacityChange();
            }
        } processing(*this, pending.size());

        class PendingReferences final {
            EventDispatchCollection& _events;
        public:
            explicit PendingReferences(EventDispatchCollection& events) : _events(events) {}
            ~PendingReferences() {
                for (PendingEvent& pending : _events) {
                    if (pending.event != nullptr) pending.event->__unref();
                }
            }
            void Release(size_t index) {
                if (_events[index].event != nullptr) {
                    _events[index].event->__unref();
                    _events[index].event = nullptr;
                }
            }
        } references(pending);

        if (method == EventDispatchMethod::Stack) {
            for (size_t index = pending.size(); index > 0; --index) {
                const size_t current = index - 1;
                callback(
                    pending[current].event,
                    method,
                    priority,
                    pending[current].context
                );
                references.Release(current);
            }
        } else {
            for (size_t index = 0; index < pending.size(); ++index) {
                callback(
                    pending[index].event,
                    method,
                    priority,
                    pending[index].context
                );
                references.Release(index);
            }
        }

        pending.clear();
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        EventDispatchCollection& destination = collections[priorityIndex];
        if (pending.capacity() > destination.capacity()) {
            EventDispatchCollection arrivals;
            arrivals.swap(destination);
            pending.swap(destination);
            destination.insert(
                destination.end(),
                std::make_move_iterator(arrivals.begin()),
                std::make_move_iterator(arrivals.end())
            );
        }
        ApplyCapacityPolicyLocked(destination);
    }

protected:
    void StopAcceptingEvents() noexcept {
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
            _acceptingPendingEvents = false;
        }
        SignalCapacityChange();
    }

    template<typename TCallback>
    void WithEvents(TCallback&& callback) {
        for (
            int priorityID = static_cast<int>(EventPriority::High);
            priorityID >= 0;
            --priorityID
        ) {
            ProcessCollection(
                _priorityStacks,
                static_cast<EventPriority>(priorityID),
                EventDispatchMethod::Stack,
                callback
            );
        }
        for (
            int priorityID = static_cast<int>(EventPriority::High);
            priorityID >= 0;
            --priorityID
        ) {
            ProcessCollection(
                _priorityQueues,
                static_cast<EventPriority>(priorityID),
                EventDispatchMethod::Queue,
                callback
            );
        }
    }

    void ClearPendingEvents() noexcept {
        EventCollection queues;
        EventCollection stacks;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
            queues.swap(_priorityQueues);
            stacks.swap(_priorityStacks);
            _pendingEventCount = 0;
        }
        SignalCapacityChange();

        auto release = [](EventCollection& collections) {
            for (auto& collection : collections) {
                for (PendingEvent& pending : collection) {
                    if (pending.event != nullptr) pending.event->__unref();
                }
            }
        };
        release(queues);
        release(stacks);
    }

    virtual void EventAdded() {}

public:
    ~EventReceiver() override {
        StopAcceptingEvents();
        ClearPendingEvents();
    }

    void QueueEvent(
        IEvent* event,
        EventPriority priority = EventPriority::Normal,
        EventDispatchContext context = {}
    ) override {
        (void)AddEvent(event, priority, EventDispatchMethod::Queue, context, true);
    }

    void StackEvent(
        IEvent* event,
        EventPriority priority = EventPriority::Normal,
        EventDispatchContext context = {}
    ) override {
        (void)AddEvent(event, priority, EventDispatchMethod::Stack, context, true);
    }

    bool TryQueueEvent(
        IEvent* event,
        EventPriority priority = EventPriority::Normal,
        EventDispatchContext context = {}
    ) override {
        return AddEvent(event, priority, EventDispatchMethod::Queue, context, false);
    }

    bool TryStackEvent(
        IEvent* event,
        EventPriority priority = EventPriority::Normal,
        EventDispatchContext context = {}
    ) override {
        return AddEvent(event, priority, EventDispatchMethod::Stack, context, false);
    }

    size_t GetMaximumPendingEventCount() const {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        return _maximumPendingEventCount;
    }

    void SetMaximumPendingEventCount(size_t maximum) {
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
            _maximumPendingEventCount = maximum;
        }
        SignalCapacityChange();
    }

    EventQueueOverflowPolicy GetEventQueueOverflowPolicy() const {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        return _overflowPolicy;
    }

    void SetEventQueueOverflowPolicy(EventQueueOverflowPolicy policy) {
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
            _overflowPolicy = policy;
        }
        SignalCapacityChange();
    }

    EventCollectionCapacityPolicy GetEventCollectionCapacityPolicy() const {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        return _capacityPolicy;
    }

    void SetEventCollectionCapacityPolicy(
        EventCollectionCapacityPolicy policy
    ) {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        _capacityPolicy = policy;
        auto apply = [&](EventCollection& collections) {
            for (auto& collection : collections) ApplyCapacityPolicyLocked(collection);
        };
        apply(_priorityQueues);
        apply(_priorityStacks);
    }

    size_t GetPendingEventCount() const {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        return _pendingEventCount;
    }

    size_t GetPeakPendingEventCount() const {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        return _peakPendingEventCount;
    }

    size_t GetRetainedEventCapacity() const {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        size_t capacity = 0;
        auto addCapacity = [&](const EventCollection& collections) {
            for (const auto& collection : collections) capacity += collection.capacity();
        };
        addCapacity(_priorityQueues);
        addCapacity(_priorityStacks);
        return capacity;
    }

    uint64_t GetRejectedEventCount() const {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        return _rejectedEventCount;
    }

    uint64_t GetDroppedEventCount() const {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        return _droppedEventCount;
    }

    void ResetEventQueueStatistics() {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        _peakPendingEventCount = _pendingEventCount;
        _rejectedEventCount = 0;
        _droppedEventCount = 0;
    }

    size_t GetMinimumRetainedEventCapacity() const {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        return _minimumRetainedCapacity;
    }

    void SetMinimumRetainedEventCapacity(size_t capacity) {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        _minimumRetainedCapacity = capacity;
    }

    size_t GetEventCapacityExcessFactor() const {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        return _capacityExcessFactor;
    }

    void SetEventCapacityExcessFactor(size_t factor) {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventsMutex);
        _capacityExcessFactor = std::max<size_t>(factor, 1);
    }
};

} // namespace Event
} // namespace ESPressio
