#include <cassert>
#include <cstdint>

#include "ESPressio_EventDispatcher.hpp"

using namespace ESPressio::Event;

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

class CompletionReceiver final : public EventReceiver {
public:
    EventDispatchContext LastContext{};

    void Drain() {
        WithEvents([&](IEvent*, EventDispatchMethod, EventPriority, const EventDispatchContext& context) {
            LastContext = context;
        });
    }
};

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
