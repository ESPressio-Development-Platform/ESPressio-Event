#include <cassert>
#include <cstdint>

#include "ESPressio_EventDispatcher.hpp"

using namespace ESPressio::Event;

class BrokerTestEvent final : public IEvent {
private:
    int _references = 0;
    EventDispatchContext _context{};

public:
    void __ref() noexcept override { ++_references; }
    void __unref() noexcept override {
        assert(_references > 0);
        --_references;
    }
    void __dispatch() override {}
    void __setDispatchContext(const EventDispatchContext& context) override {
        _context = context;
    }
    EventDispatchContext __getDispatchContext() const override { return _context; }
    EventTypeKey __getTypeKey() const noexcept override {
        return EventTypeKeyOf<BrokerTestEvent>();
    }
    void Queue(EventPriority = EventPriority::Normal) override {}
    void Stack(EventPriority = EventPriority::Normal) override {}
    uint64_t GetDispatchTimeNanoseconds() const override { return 0; }
    uint64_t GetTimeSinceDispatchNanoseconds() const override { return 0; }

    int References() const { return _references; }
};

class BrokerTestReceiver final : public EventReceiver {
public:
    int Processed = 0;

    void Drain() {
        WithEvents([&](IEvent*, EventDispatchMethod, EventPriority) {
            ++Processed;
        });
    }
};

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
    dispatcher.QueueEvent(&routed);
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
    assert(routed.References() == 0);

    saturated.Drain();
    assert(saturated.Processed == 1);
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
