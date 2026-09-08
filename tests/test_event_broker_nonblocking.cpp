#include <cassert>
#include <cstdint>

#include "ESPressio_EventDispatcher.hpp"

using namespace ESPressio::Event;

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members:
 * - _references (int): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 8 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class BrokerTestEvent final : public IEvent {
private:
    int _references = 0;

public:
    void __ref() noexcept override { ++_references; }
    void __unref() noexcept override {
        assert(_references > 0);
        --_references;
    }
    void __dispatch() override {}
    EventTypeKey __getTypeKey() const noexcept override {
        return EventTypeKeyOf<BrokerTestEvent>();
    }
    void Queue(EventPriority = EventPriority::Normal) override {}
    void Stack(EventPriority = EventPriority::Normal) override {}
    uint64_t GetDispatchTimeNanoseconds() const override { return 0; }
    uint64_t GetTimeSinceDispatchNanoseconds() const override { return 0; }

    int References() const { return _references; }
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 228 bytes [EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventReceiver: _capacityAvailable: owned object: 4 bytes; EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage]
 * Members:
 * - Processed (int): 4 bytes [0 bytes dynamic allocation]
 * - LastContext (EventDispatchContext): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 236 bytes [EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventReceiver: _capacityAvailable: owned object: 4 bytes; EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class BrokerTestReceiver final : public EventReceiver {
public:
    int Processed = 0;
    EventDispatchContext LastContext{};

    void Drain() {
        WithEvents([&](IEvent*, EventDispatchMethod, EventPriority, const EventDispatchContext& context) {
            ++Processed;
            LastContext = context;
        });
    }
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 272 bytes [EventDispatcher: EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventDispatcher: EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventDispatcher: EventReceiver: _capacityAvailable: owned object: 4 bytes; EventDispatcher: EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventDispatcher: EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; EventDispatcher: _eventReceivers: Capacity * (12 bytes) element storage; EventDispatcher: _eventReceiversMutex: _owned: owned object: 4 bytes; EventDispatcher: _eventReceiversMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Members: none (standalone empty object occupies 1 byte; an eligible empty base may be optimized to 0 bytes).
 * Total Memory: 272 bytes [EventDispatcher: EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventDispatcher: EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventDispatcher: EventReceiver: _capacityAvailable: owned object: 4 bytes; EventDispatcher: EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventDispatcher: EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; EventDispatcher: _eventReceivers: Capacity * (12 bytes) element storage; EventDispatcher: _eventReceiversMutex: _owned: owned object: 4 bytes; EventDispatcher: _eventReceiversMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class BrokerTestDispatcher final : public EventDispatcher {
public:
    void Dispatch() { DispatchEvents(); }
};

int main() {
    BrokerTestDispatcher dispatcher;
    BrokerTestReceiver saturated;
    BrokerTestReceiver healthy;

    saturated.SetMaximumPendingEventCount(1);
    saturated.SetEventQueueOverflowPolicy(EventQueueOverflowPolicy::BlockProducer);

    dispatcher.RegisterReceiver(EventTypeKeyOf<BrokerTestEvent>(), &saturated);
    dispatcher.RegisterReceiver(EventTypeKeyOf<BrokerTestEvent>(), &healthy);

    BrokerTestEvent retained;
    saturated.QueueEvent(&retained);
    assert(retained.References() == 1);

    BrokerTestEvent routed;
    dispatcher.QueueEvent(
        &routed,
        EventPriority::Normal,
        EventDispatchContext{EventOrigin::Remote}
    );
    assert(routed.References() == 1);

    // Dispatch must not block even though the first receiver is full and is
    // configured to BlockProducer for direct/local producer calls.
    dispatcher.Dispatch();

    assert(saturated.GetPendingEventCount() == 1);
    assert(saturated.GetRejectedEventCount() == 1);
    assert(healthy.GetPendingEventCount() == 1);
    assert(routed.References() == 1);

    healthy.Drain();
    assert(healthy.Processed == 1);
    assert(healthy.LastContext.Origin == EventOrigin::Remote);
    assert(routed.References() == 0);

    saturated.Drain();
    assert(saturated.Processed == 1);
    assert(saturated.LastContext.Origin == EventOrigin::Local);
    assert(retained.References() == 0);

    BrokerTestEvent directTry;
    saturated.QueueEvent(&retained);
    assert(retained.References() == 1);
    const bool admitted = saturated.TryQueueEvent(&directTry);
    assert(!admitted);
    assert(directTry.References() == 0);
    saturated.Drain();
    assert(retained.References() == 0);

    dispatcher.UnregisterReceiver(EventTypeKeyOf<BrokerTestEvent>(), &saturated);
    dispatcher.UnregisterReceiver(EventTypeKeyOf<BrokerTestEvent>(), &healthy);
    return 0;
}
