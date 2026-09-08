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
class CompletionEvent final : public IEvent {
private:
    int _references = 0;
public:
    void __ref() noexcept override { ++_references; }
    void __unref() noexcept override { assert(_references > 0); --_references; }
    void __dispatch() override {}
    EventTypeKey __getTypeKey() const noexcept override { return EventTypeKeyOf<CompletionEvent>(); }
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
 * - LastContext (EventDispatchContext): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 232 bytes [EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventReceiver: _capacityAvailable: owned object: 4 bytes; EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class CompletionReceiver final : public EventReceiver {
public:
    EventDispatchContext LastContext{};

    void Drain() {
        WithEvents([&](IEvent*, EventDispatchMethod, EventPriority, const EventDispatchContext& context) {
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
class CompletionDispatcher final : public EventDispatcher {
public:
    template<typename TCompletion>
    void Dispatch(TCompletion&& completion) {
        DispatchEvents(std::forward<TCompletion>(completion));
    }
};

int main() {
    CompletionDispatcher dispatcher;
    CompletionReceiver receiver;
    dispatcher.RegisterReceiver(EventTypeKeyOf<CompletionEvent>(), &receiver);

    CompletionEvent event;
    const EventDispatchContext remote{EventOrigin::Remote};
    dispatcher.QueueEvent(&event, EventPriority::Normal, remote);
    assert(event.References() == 1);

    IEvent* completionReference = nullptr;
    dispatcher.Dispatch([&](
        IEvent* dispatched,
        EventDispatchMethod method,
        EventPriority priority,
        const EventDispatchContext& context
    ) {
        assert(dispatched == &event);
        assert(method == EventDispatchMethod::Queue);
        assert(priority == EventPriority::Normal);
        assert(context.Origin == EventOrigin::Remote);
        assert(event.References() == 2); // dispatcher + receiver mailbox
        dispatched->__ref();            // model async observer ownership
        completionReference = dispatched;
    });

    // Dispatcher released its own ingress ref. Receiver + completion work remain.
    assert(event.References() == 2);

    completionReference->__unref();
    assert(event.References() == 1);

    receiver.Drain();
    assert(receiver.LastContext.Origin == EventOrigin::Remote);
    assert(event.References() == 0);
    return 0;
}
