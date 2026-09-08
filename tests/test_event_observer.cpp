#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

#include "ESPressio_EventListener.hpp"

using namespace ESPressio::Event;

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members:
 * - _references (int): 4 bytes [0 bytes dynamic allocation]
 * - _ageNanoseconds (uint64_t): 8 bytes [0 bytes dynamic allocation]
 * Total Memory: 16 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class TestEvent final : public IEvent {
    private:
        int _references = 1;
        uint64_t _ageNanoseconds = 0;

    public:
        explicit TestEvent(unsigned long ageMilliseconds = 0)
            : _ageNanoseconds(
                static_cast<uint64_t>(ageMilliseconds) * 1000000ULL
            ) {}
        void __ref() noexcept override { ++_references; }
        void __unref() noexcept override { --_references; }
        void __dispatch() override {}
        EventTypeKey __getTypeKey() const noexcept override {
            return EventTypeKeyOf<TestEvent>();
        }
        void Queue(EventPriority = EventPriority::Normal) override {}
        void Stack(EventPriority = EventPriority::Normal) override {}
        uint64_t GetDispatchTimeNanoseconds() const override { return 0; }
        uint64_t GetTimeSinceDispatchNanoseconds() const override {
            return _ageNanoseconds;
        }
        int References() const { return _references; }
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class OtherEvent final : public IEvent {
    public:
        void __ref() noexcept override {}
        void __unref() noexcept override {}
        void __dispatch() override {}
        EventTypeKey __getTypeKey() const noexcept override {
            return EventTypeKeyOf<OtherEvent>();
        }
        void Queue(EventPriority = EventPriority::Normal) override {}
        void Stack(EventPriority = EventPriority::Normal) override {}
        uint64_t GetDispatchTimeNanoseconds() const override { return 0; }
        uint64_t GetTimeSinceDispatchNanoseconds() const override { return 0; }
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members:
 * - calls (int): 4 bytes [0 bytes dynamic allocation]
 * - interested (bool): 1 bytes [0 bytes dynamic allocation]
 * - lastEvent (TestEvent*): 4 bytes [0 bytes dynamic allocation]
 * - lastMethod (EventDispatchMethod): 4 bytes [0 bytes dynamic allocation]
 * - lastPriority (EventPriority): 4 bytes [0 bytes dynamic allocation]
 * - lastOrigin (EventOrigin): 1 bytes [0 bytes dynamic allocation]
 * - unregisterOnEvent (IEventListenerHandle*): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 32 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class TestObserver final : public IEventObserver<TestEvent> {
    public:
        int calls = 0;
        bool interested = true;
        TestEvent* lastEvent = nullptr;
        EventDispatchMethod lastMethod = EventDispatchMethod::Queue;
        EventPriority lastPriority = EventPriority::Normal;
        EventOrigin lastOrigin = EventOrigin::Local;
        IEventListenerHandle* unregisterOnEvent = nullptr;

        void OnEvent(
            TestEvent* event,
            EventDispatchMethod dispatchMethod,
            EventPriority priority,
            const EventDispatchContext& context
        ) override {
            ++calls;
            lastEvent = event;
            lastMethod = dispatchMethod;
            lastPriority = priority;
            lastOrigin = context.Origin;
            if (unregisterOnEvent != nullptr) {
                unregisterOnEvent->Unregister();
            }
        }

        bool IsInterestedInEvent(TestEvent*) override {
            return interested;
        }
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 8 bytes [0 bytes dynamic allocation]
 * Members:
 * - testCalls (int): 4 bytes [0 bytes dynamic allocation]
 * - otherCalls (int): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 16 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class MultiEventObserver final :
    public IEventObserver<TestEvent>,
    public IEventObserver<OtherEvent> {
    public:
        int testCalls = 0;
        int otherCalls = 0;

        void OnEvent(
            TestEvent*, EventDispatchMethod, EventPriority, const EventDispatchContext&
        ) override {
            ++testCalls;
        }

        void OnEvent(
            OtherEvent*, EventDispatchMethod, EventPriority, const EventDispatchContext&
        ) override {
            ++otherCalls;
        }
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes known bases + 5 bytes known members + sizeof(ListenerStorage) + sizeof(System::Synchronization::RecursiveMutex) [0 bytes dynamic allocation]
 * Members:
 * - registrations (int): 4 bytes [0 bytes dynamic allocation]
 * - unregistrations (int): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 4 bytes known bases + 5 bytes known members + sizeof(ListenerStorage) + sizeof(System::Synchronization::RecursiveMutex) + 8 bytes known members [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class TrackingEventListener final : public EventListener {
    public:
        int registrations = 0;
        int unregistrations = 0;

        void Shutdown() { UnregisterAllListeners(); }

    protected:
        void OnListenerRegistered(EventTypeKey) override { ++registrations; }
        void OnListenerUnregistered(EventTypeKey) override { ++unregistrations; }
};

static_assert(std::is_base_of<
    ESPressio::Observable::IObserver,
    IEventObserver<TestEvent>>::value,
    "Event Observers must satisfy the ESPressio Observable IObserver interface");
static_assert(std::is_convertible<MultiEventObserver*,
    ESPressio::Observable::IObserver*>::value,
    "Multi-Event Observers must have one unambiguous IObserver identity");

void Process(
    EventListener& listener,
    IEvent& event,
    EventDispatchMethod method = EventDispatchMethod::Queue,
    EventPriority priority = EventPriority::Normal,
    EventDispatchContext context = {}) {
    listener.ProcessEvent(&event, method, priority, context);
}

int main() {
    TrackingEventListener listener;
    TestObserver observer;

    bool nullThrown = false;
    try { listener.RegisterObserver<TestEvent>(nullptr); }
    catch (const ESPressio::Observable::InvalidObserverRegistrationException&) {
        nullThrown = true;
    }
    assert(nullThrown);

    MultiEventObserver multiObserver;
    EventListenerHandlePtr multiTestHandle =
        listener.RegisterObserver<TestEvent>(&multiObserver);
    EventListenerHandlePtr multiOtherHandle =
        listener.RegisterObserver<OtherEvent>(&multiObserver);
    TestEvent multiTestEvent;
    OtherEvent multiOtherEvent;
    Process(listener, multiTestEvent);
    Process(listener, multiOtherEvent);
    assert(multiObserver.testCalls == 1);
    assert(multiObserver.otherCalls == 1);
    multiTestHandle.reset();
    multiOtherHandle.reset();

    EventListenerHandlePtr observerHandle =
        listener.RegisterObserver<TestEvent>(&observer);
    assert(observerHandle->IsRegistered());
    const int routingRegistrations = listener.registrations;
    const int routingUnregistrations = listener.unregistrations;

    int callbackCalls = 0;
    EventOrigin callbackOrigin = EventOrigin::Local;
    EventListenerHandlePtr callbackHandle = listener.RegisterListener<TestEvent>(
        [&](TestEvent*, EventDispatchMethod, EventPriority, const EventDispatchContext& context) {
            ++callbackCalls;
            callbackOrigin = context.Origin;
        });
    assert(listener.registrations == routingRegistrations);

    TestEvent event;
    Process(
        listener,
        event,
        EventDispatchMethod::Stack,
        EventPriority::High,
        EventDispatchContext{EventOrigin::Remote}
    );
    assert(observer.calls == 1);
    assert(observer.lastEvent == &event);
    assert(observer.lastMethod == EventDispatchMethod::Stack);
    assert(observer.lastPriority == EventPriority::High);
    assert(observer.lastOrigin == EventOrigin::Remote);
    assert(callbackCalls == 1);
    assert(callbackOrigin == EventOrigin::Remote);
    assert(event.References() == 1);

    observerHandle->Unregister();
    observerHandle->Unregister();
    assert(!observerHandle->IsRegistered());
    Process(listener, event);
    assert(observer.calls == 1);
    assert(callbackCalls == 2);
    assert(listener.unregistrations == routingUnregistrations);
    observerHandle.reset();
    callbackHandle.reset();
    assert(listener.unregistrations == routingUnregistrations + 1);

    TestObserver customObserver;
    EventListenerHandlePtr customHandle = listener.RegisterObserver<TestEvent>(
        &customObserver, EventListenerInterest::Custom);
    customObserver.interested = false;
    Process(listener, event);
    assert(customObserver.calls == 0);
    customObserver.interested = true;
    Process(listener, event);
    assert(customObserver.calls == 1);
    customHandle.reset();

    TestObserver youngObserver;
    EventListenerHandlePtr youngHandle = listener.RegisterObserver<TestEvent>(
        &youngObserver,
        EventListenerInterest::YoungerThan,
        EventTime(10, ESPressio::Units::Milli)
    );
    TestEvent youngEvent(9);
    TestEvent oldEvent(10);
    Process(listener, youngEvent);
    Process(listener, oldEvent);
    assert(youngObserver.calls == 1);
    youngHandle.reset();

    TestObserver selfRemovingObserver;
    EventListenerHandlePtr selfRemovingHandle =
        listener.RegisterObserver<TestEvent>(&selfRemovingObserver);
    selfRemovingObserver.unregisterOnEvent = selfRemovingHandle.get();
    Process(listener, event);
    Process(listener, event);
    assert(selfRemovingObserver.calls == 1);
    assert(!selfRemovingHandle->IsRegistered());
    selfRemovingHandle.reset();

    EventListener reentrantListener;
    EventListenerHandlePtr appendedHandle;
    int firstCalls = 0;
    int appendedCalls = 0;
    EventListenerHandlePtr firstHandle = reentrantListener.RegisterListener<TestEvent>(
        [&](TestEvent*, EventDispatchMethod, EventPriority, const EventDispatchContext&) {
            ++firstCalls;
            if (!appendedHandle) {
                appendedHandle = reentrantListener.RegisterListener<TestEvent>(
                    [&](TestEvent*, EventDispatchMethod, EventPriority, const EventDispatchContext&) {
                        ++appendedCalls;
                    }
                );
            }
        }
    );
    Process(reentrantListener, event);
    assert(firstCalls == 1);
    assert(appendedCalls == 0);
    Process(reentrantListener, event);
    assert(firstCalls == 2);
    assert(appendedCalls == 1);
    firstHandle.reset();
    appendedHandle.reset();

    EventListenerHandlePtr throwingHandle = listener.RegisterListener<TestEvent>(
        [](TestEvent*, EventDispatchMethod, EventPriority, const EventDispatchContext&) {
            throw std::runtime_error("expected callback failure");
        });
    bool callbackThrown = false;
    const int referencesBeforeThrow = event.References();
    try { Process(listener, event); }
    catch (const std::runtime_error&) { callbackThrown = true; }
    assert(callbackThrown);
    assert(event.References() == referencesBeforeThrow);
    throwingHandle.reset();

    OtherEvent otherEvent;
    Process(listener, otherEvent);

    TestObserver survivingObserver;
    EventListenerHandlePtr survivingHandle;
    {
        EventListener temporaryListener;
        survivingHandle = temporaryListener.RegisterObserver<TestEvent>(
            &survivingObserver);
        assert(survivingHandle->IsRegistered());
    }
    assert(!survivingHandle->IsRegistered());
    survivingHandle->Unregister();
    survivingHandle.reset();

    TrackingEventListener shutdownListener;
    EventListenerHandlePtr shutdownHandle =
        shutdownListener.RegisterObserver<TestEvent>(&survivingObserver);
    shutdownListener.Shutdown();
    assert(shutdownListener.unregistrations == 1);
    assert(!shutdownHandle->IsRegistered());
    shutdownHandle.reset();
}
