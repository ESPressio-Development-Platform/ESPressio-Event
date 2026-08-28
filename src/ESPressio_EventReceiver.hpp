#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <typeindex>
#include <utility>
#include <vector>

#include "ESPressio_EventEnums.hpp"
#include "ESPressio_IEvent.hpp"

#ifndef ESPRESSIO_EVENT_DEFAULT_MAX_PENDING_EVENT_COUNT
    #define ESPRESSIO_EVENT_DEFAULT_MAX_PENDING_EVENT_COUNT 64
#endif

namespace ESPressio {
    namespace Event {

        /// <summary>Defines how a receiver behaves when its retained event capacity is exhausted.</summary>
        enum class EventQueueOverflowPolicy : uint8_t {
            BlockProducer,
            RejectIncoming,
            DropOldest,
            DropLowestPriority
        };

        /// <summary>Controls how queue/stack backing capacity is retained after events are drained.</summary>
        enum class EventCollectionCapacityPolicy : uint8_t {
            Retain,
            ShrinkWhenUnderutilized,
            ReleaseAfterDrain
        };

        /// <summary>Receives Event instances using FIFO queue or LIFO stack dispatch semantics.</summary>
        class IEventReceiver {
            public:
                virtual ~IEventReceiver() = default;

                /// <summary>Adds an Event for FIFO processing at the specified priority.</summary>
                virtual void QueueEvent(
                    IEvent* event,
                    EventPriority priority = EventPriority::Normal
                ) = 0;

                /// <summary>Adds an Event for LIFO processing at the specified priority.</summary>
                virtual void StackEvent(
                    IEvent* event,
                    EventPriority priority = EventPriority::Normal
                ) = 0;
        };

        /// <summary>Thread-safe priority receiver that retains Event references until queued or stacked work is processed.</summary>
        /// <remarks>Queue entries are processed FIFO, stack entries LIFO, and higher priorities are drained before lower priorities. Capacity and overflow behavior are configurable.</remarks>
        class EventReceiver : public IEventReceiver {
            private:
                struct PendingEvent {
                    IEvent* event = nullptr;
                    uint64_t sequence = 0;
                };

                using EventDispatchCollection = std::vector<PendingEvent>;

                static constexpr size_t PriorityCount =
                    static_cast<size_t>(EventPriority::High) + 1;

                using EventCollection =
                    std::array<
                        EventDispatchCollection,
                        PriorityCount
                    >;

                static constexpr size_t CapacitySampleCount = 16;

                static constexpr size_t PriorityIndex(
                    EventPriority priority
                ) {
                    return static_cast<size_t>(priority);
                }

                mutable std::mutex _eventsMutex;
                std::condition_variable _capacityAvailable;
                EventCollection _priorityQueues;
                EventCollection _priorityStacks;
                size_t _pendingEventCount = 0;
                size_t _processingEventCount = 0;
                size_t _peakPendingEventCount = 0;
                size_t _maximumPendingEventCount =
                    ESPRESSIO_EVENT_DEFAULT_MAX_PENDING_EVENT_COUNT;
                EventQueueOverflowPolicy _overflowPolicy =
                    EventQueueOverflowPolicy::BlockProducer;
                EventCollectionCapacityPolicy _capacityPolicy =
                    EventCollectionCapacityPolicy::ShrinkWhenUnderutilized;
                size_t _minimumRetainedCapacity = 4;
                size_t _capacityExcessFactor = 2;
                std::array<size_t, CapacitySampleCount> _recentDrainSizes{};
                size_t _recentDrainIndex = 0;
                size_t _recentDrainCount = 0;
                uint64_t _nextSequence = 0;
                uint64_t _rejectedEventCount = 0;
                uint64_t _droppedEventCount = 0;
                bool _acceptingPendingEvents = true;

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
                    _recentDrainIndex =
                        (_recentDrainIndex + 1) % CapacitySampleCount;
                    _recentDrainCount = std::min(
                        _recentDrainCount + 1, CapacitySampleCount
                    );
                }

                size_t RecentPeakLocked() const {
                    size_t peak = 0;
                    for (size_t index = 0;
                        index < _recentDrainCount; ++index) {
                        peak = std::max(peak, _recentDrainSizes[index]);
                    }
                    return peak;
                }

                void ApplyCapacityPolicyLocked(
                    EventDispatchCollection& collection
                ) {
                    if (_capacityPolicy ==
                        EventCollectionCapacityPolicy::Retain) {
                        return;
                    }
                    if (_capacityPolicy ==
                        EventCollectionCapacityPolicy::ReleaseAfterDrain) {
                        EventDispatchCollection replacement(collection);
                        replacement.shrink_to_fit();
                        collection.swap(replacement);
                        return;
                    }

                    const size_t recentPeak = RecentPeakLocked();
                    const size_t target = std::max(
                        _minimumRetainedCapacity,
                        recentPeak > std::numeric_limits<size_t>::max() /
                            _capacityExcessFactor
                            ? std::numeric_limits<size_t>::max()
                            : recentPeak * _capacityExcessFactor
                    );
                    if (collection.capacity() > target) {
                        EventDispatchCollection replacement;
                        replacement.reserve(std::max(
                            target, collection.size()
                        ));
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
                    uint64_t selectedSequence =
                        std::numeric_limits<uint64_t>::max();

                    auto consider = [&](EventCollection& collections) {
                        for (auto& collection : collections) {
                            for (size_t index = 0;
                                index < collection.size(); ++index) {
                                if (collection[index].sequence <
                                    selectedSequence) {
                                    selected = &collection;
                                    selectedIndex = index;
                                    selectedSequence =
                                        collection[index].sequence;
                                }
                            }
                        }
                    };

                    consider(_priorityQueues);
                    consider(_priorityStacks);

                    if (selected == nullptr) {
                        return PendingEvent{};
                    }

                    PendingEvent removed = (*selected)[selectedIndex];
                    selected->erase(selected->begin() + selectedIndex);
                    --_pendingEventCount;
                    return removed;
                }

                PendingEvent RemoveLowestPriorityLocked(bool& removed) {
                    for (size_t priorityID = 0;
                        priorityID < PriorityCount;
                        ++priorityID) {
                        auto removeFrom = [&](EventCollection& collections) {
                            EventDispatchCollection& collection =
                                collections[priorityID];

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
                        if (removed) {
                            return result;
                        }

                        result = removeFrom(_priorityStacks);
                        if (removed) {
                            return result;
                        }
                    }
                    return PendingEvent{};
                }

                void AddEvent(
                    IEvent* event,
                    EventPriority priority,
                    EventDispatchMethod method
                ) {
                    event->__dispatch();
                    event->__ref();
                    IEvent* displacedEvent = nullptr;
                    bool accepted = false;
                    try {
                        std::unique_lock<std::mutex> lock(_eventsMutex);
                        if (!_acceptingPendingEvents) {
                            ++_rejectedEventCount;
                            lock.unlock();
                            event->__unref();
                            return;
                        }
                        while (_maximumPendingEventCount > 0 &&
                            RetainedEventCountLocked() >=
                                _maximumPendingEventCount) {
                            if (!_acceptingPendingEvents) {
                                ++_rejectedEventCount;
                                lock.unlock();
                                event->__unref();
                                return;
                            }
                            switch (_overflowPolicy) {
                                case EventQueueOverflowPolicy::BlockProducer:
                                    _capacityAvailable.wait(lock, [&]() {
                                        return _maximumPendingEventCount == 0 ||
                                            RetainedEventCountLocked() <
                                                _maximumPendingEventCount ||
                                            !_acceptingPendingEvents ||
                                            _overflowPolicy !=
                                                EventQueueOverflowPolicy::
                                                    BlockProducer;
                                    });
                                    continue;
                                case EventQueueOverflowPolicy::RejectIncoming:
                                    ++_rejectedEventCount;
                                    lock.unlock();
                                    event->__unref();
                                    return;
                                case EventQueueOverflowPolicy::DropOldest:
                                    displacedEvent =
                                        RemoveOldestLocked().event;
                                    if (displacedEvent == nullptr) {
                                        ++_rejectedEventCount;
                                        lock.unlock();
                                        event->__unref();
                                        return;
                                    }
                                    ++_droppedEventCount;
                                    break;
                                case EventQueueOverflowPolicy::
                                    DropLowestPriority: {
                                    bool removed = false;
                                    PendingEvent displaced =
                                        RemoveLowestPriorityLocked(removed);
                                    if (!removed) {
                                        ++_rejectedEventCount;
                                        lock.unlock();
                                        event->__unref();
                                        return;
                                    }
                                    displacedEvent = displaced.event;
                                    ++_droppedEventCount;
                                    break;
                                }
                            }
                            break;
                        }

                        EventCollection& collections =
                            method == EventDispatchMethod::Queue
                                ? _priorityQueues
                                : _priorityStacks;

                        collections[
                            PriorityIndex(priority)
                        ].push_back(PendingEvent{
                            event, NextSequence(_nextSequence)
                        });

                        ++_pendingEventCount;
                        _peakPendingEventCount = std::max(
                            _peakPendingEventCount,
                            RetainedEventCountLocked()
                        );
                        accepted = true;
                    } catch (...) {
                        event->__unref();
                        if (displacedEvent != nullptr) {
                            displacedEvent->__unref();
                        }
                        throw;
                    }
                    if (displacedEvent != nullptr) {
                        displacedEvent->__unref();
                    }
                    if (accepted) {
                        EventAdded();
                    }
                }

                void ProcessCollection(
                    EventCollection& collections,
                    EventPriority priority,
                    EventDispatchMethod method,
                    const std::function<void(
                        IEvent*, EventDispatchMethod, EventPriority
                    )>& callback
                ) {
                    EventDispatchCollection pending;
                    const size_t priorityIndex =
                        PriorityIndex(priority);

                    {
                        std::lock_guard<std::mutex> lock(_eventsMutex);
                        EventDispatchCollection& source =
                            collections[priorityIndex];

                        if (source.empty()) {
                            return;
                        }

                        pending.swap(source);
                        _pendingEventCount -= pending.size();
                        _processingEventCount += pending.size();
                        RecordDrainSizeLocked(pending.size());
                    }

                    class ProcessingGuard final {
                        private:
                            EventReceiver& _receiver;
                            size_t _count;
                        public:
                            ProcessingGuard(EventReceiver& receiver, size_t count)
                                : _receiver(receiver), _count(count) { }
                            ~ProcessingGuard() {
                                {
                                    std::lock_guard<std::mutex> lock(
                                        _receiver._eventsMutex);
                                    _receiver._processingEventCount -= _count;
                                }
                                _receiver._capacityAvailable.notify_all();
                            }
                    } processing(*this, pending.size());

                    class PendingReferences final {
                        private:
                            EventDispatchCollection& _events;
                        public:
                            explicit PendingReferences(
                                EventDispatchCollection& events
                            ) : _events(events) { }
                            ~PendingReferences() {
                                for (PendingEvent& pending : _events) {
                                    if (pending.event != nullptr) {
                                        pending.event->__unref();
                                    }
                                }
                            }
                            void Release(size_t index) {
                                _events[index].event->__unref();
                                _events[index].event = nullptr;
                            }
                    } references(pending);

                    if (method == EventDispatchMethod::Stack) {
                        for (size_t index = pending.size(); index > 0; --index) {
                            const size_t current = index - 1;
                            callback(pending[current].event, method, priority);
                            references.Release(current);
                        }
                    } else {
                        for (size_t index = 0; index < pending.size(); ++index) {
                            callback(pending[index].event, method, priority);
                            references.Release(index);
                        }
                    }

                    pending.clear();
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    EventDispatchCollection& destination =
                        collections[priorityIndex];
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
                /// <summary>Prevents new events from being accepted and wakes any producer blocked by the capacity policy.</summary>
                void StopAcceptingEvents() noexcept {
                    {
                        std::lock_guard<std::mutex> lock(_eventsMutex);
                        _acceptingPendingEvents = false;
                    }
                    _capacityAvailable.notify_all();
                }

                /// <summary>Drains pending events in priority order and invokes the callback for each retained event.</summary>
                /// <remarks>For each priority, stacked events are drained before queued events. Stack order is LIFO and queue order is FIFO.</remarks>
                void WithEvents(
                    std::function<void(
                        IEvent*, EventDispatchMethod, EventPriority
                    )> callback
                ) {
                    for (int priorityID =
                            static_cast<int>(EventPriority::High);
                        priorityID >= 0; --priorityID) {
                        ProcessCollection(
                            _priorityStacks,
                            static_cast<EventPriority>(priorityID),
                            EventDispatchMethod::Stack,
                            callback
                        );
                    }
                    for (int priorityID =
                            static_cast<int>(EventPriority::High);
                        priorityID >= 0; --priorityID) {
                        ProcessCollection(
                            _priorityQueues,
                            static_cast<EventPriority>(priorityID),
                            EventDispatchMethod::Queue,
                            callback
                        );
                    }
                }

                /// <summary>Removes every pending queue/stack entry and releases the receiver's retained Event references.</summary>
                void ClearPendingEvents() noexcept {
                    EventCollection queues;
                    EventCollection stacks;
                    {
                        std::lock_guard<std::mutex> lock(_eventsMutex);
                        queues.swap(_priorityQueues);
                        stacks.swap(_priorityStacks);
                        _pendingEventCount = 0;
                    }
                    _capacityAvailable.notify_all();
                    auto release = [](EventCollection& collections) {
                        for (auto& collection : collections) {
                            for (PendingEvent& pending : collection) {
                                pending.event->__unref();
                            }
                        }
                    };
                    release(queues);
                    release(stacks);
                }

                /// <summary>Hook invoked after an event has been accepted into pending storage.</summary>
                virtual void EventAdded() { }

            public:
                ~EventReceiver() override {
                    StopAcceptingEvents();
                    ClearPendingEvents();
                }

                /// <inheritdoc/>
                void QueueEvent(
                    IEvent* event,
                    EventPriority priority = EventPriority::Normal
                ) override {
                    AddEvent(event, priority, EventDispatchMethod::Queue);
                }

                /// <inheritdoc/>
                void StackEvent(
                    IEvent* event,
                    EventPriority priority = EventPriority::Normal
                ) override {
                    AddEvent(event, priority, EventDispatchMethod::Stack);
                }

                /// <summary>Returns the configured maximum number of retained pending/processing events; zero means unlimited.</summary>
                size_t GetMaximumPendingEventCount() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _maximumPendingEventCount;
                }

                /// <summary>Sets the maximum number of retained pending/processing events; zero disables the limit.</summary>
                void SetMaximumPendingEventCount(size_t maximum) {
                    {
                        std::lock_guard<std::mutex> lock(_eventsMutex);
                        _maximumPendingEventCount = maximum;
                    }
                    _capacityAvailable.notify_all();
                }

                /// <summary>Returns the overflow policy applied when retained event capacity is exhausted.</summary>
                EventQueueOverflowPolicy GetEventQueueOverflowPolicy() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _overflowPolicy;
                }

                /// <summary>Sets the overflow policy and wakes producers that may be waiting under BlockProducer.</summary>
                void SetEventQueueOverflowPolicy(
                    EventQueueOverflowPolicy policy
                ) {
                    {
                        std::lock_guard<std::mutex> lock(_eventsMutex);
                        _overflowPolicy = policy;
                    }
                    _capacityAvailable.notify_all();
                }

                /// <summary>Returns the policy used to retain or reclaim queue/stack backing capacity after drains.</summary>
                EventCollectionCapacityPolicy
                GetEventCollectionCapacityPolicy() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _capacityPolicy;
                }

                /// <summary>Sets the backing-capacity policy and applies it immediately to all priority collections.</summary>
                void SetEventCollectionCapacityPolicy(
                    EventCollectionCapacityPolicy policy
                ) {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    _capacityPolicy = policy;
                    auto apply = [&](EventCollection& collections) {
                        for (auto& collection : collections) {
                            ApplyCapacityPolicyLocked(collection);
                        }
                    };
                    apply(_priorityQueues);
                    apply(_priorityStacks);
                }

                /// <summary>Returns the number of events waiting in queue/stack storage.</summary>
                size_t GetPendingEventCount() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _pendingEventCount;
                }

                /// <summary>Returns the highest retained event count observed since statistics were last reset.</summary>
                size_t GetPeakPendingEventCount() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _peakPendingEventCount;
                }

                /// <summary>Returns the aggregate vector capacity currently retained by all priority queue and stack collections.</summary>
                size_t GetRetainedEventCapacity() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    size_t capacity = 0;
                    auto addCapacity = [&](const EventCollection& collections) {
                        for (const auto& collection : collections) {
                            capacity += collection.capacity();
                        }
                    };
                    addCapacity(_priorityQueues);
                    addCapacity(_priorityStacks);
                    return capacity;
                }

                /// <summary>Returns the number of incoming events rejected by capacity/lifecycle policy.</summary>
                uint64_t GetRejectedEventCount() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _rejectedEventCount;
                }

                /// <summary>Returns the number of previously retained events displaced by a drop overflow policy.</summary>
                uint64_t GetDroppedEventCount() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _droppedEventCount;
                }

                /// <summary>Resets peak, rejected, and dropped event statistics without altering pending events.</summary>
                void ResetEventQueueStatistics() {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    _peakPendingEventCount = _pendingEventCount;
                    _rejectedEventCount = 0;
                    _droppedEventCount = 0;
                }

                /// <summary>Returns the minimum vector capacity retained by the adaptive shrink policy.</summary>
                size_t GetMinimumRetainedEventCapacity() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _minimumRetainedCapacity;
                }

                /// <summary>Sets the minimum vector capacity retained by the adaptive shrink policy.</summary>
                void SetMinimumRetainedEventCapacity(size_t capacity) {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    _minimumRetainedCapacity = capacity;
                }

                /// <summary>Returns the multiplier applied to recent peak drain size when calculating retained capacity.</summary>
                size_t GetEventCapacityExcessFactor() const {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    return _capacityExcessFactor;
                }

                /// <summary>Sets the adaptive retained-capacity multiplier; values below one are normalized to one.</summary>
                void SetEventCapacityExcessFactor(size_t factor) {
                    std::lock_guard<std::mutex> lock(_eventsMutex);
                    _capacityExcessFactor = std::max<size_t>(factor, 1);
                }
        };
    }
}
