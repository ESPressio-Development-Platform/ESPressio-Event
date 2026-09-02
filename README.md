# ESPressio Event

Generic Event-Driven Development infrastructure for the ESPressio Development Platform.

ESPressio Event provides the asynchronous counterpart to ESPressio Observable: producers dispatch strongly typed data contracts without knowing which consumers exist, while listeners process those Events independently on Event-aware Threads.

## Current manifest version — 6.0.3

The active development branch contains unreleased RTTI-free and memory-efficiency work. The manifest version has intentionally not been changed during this optimisation round.

# Why Event-Driven Development?

Event-driven design is useful when the producer of an operation should not depend on the implementation, execution context, or even existence of its consumers.

```text
Producer
   |
   | dispatches
   v
Typed Event
   |
   v
EventManager
   |
   +----------+----------+
   |          |          |
   v          v          v
Listener A Listener B Listener C
```

The Event itself is the data contract. The producer populates that contract; listeners read it. The producer never needs to retain references to listeners.

`Queue()` and `Stack()` are asynchronous. Independent listeners do not have a meaningful globally guaranteed execution order. If a caller requires synchronous ordered completion, direct sequencing or ESPressio Observable is generally the more accurate abstraction.

# Namespace

```cpp
ESPressio::Event
```

Important public concepts include:

- `IEvent` — type-erased Event interface used by routing infrastructure.
- `Event<TTime>` — lifecycle/timestamp implementation base.
- `TypedEvent<TDerived, TTime>` — concrete RTTI-free Event identity base.
- `SerializableEvent<TDerived>` — Serializable Event base with typed identity.
- `EventManager` — central dispatch/routing manager.
- `EventListener` / listener handles — type-specific consumer registration.
- `EventThread` — asynchronous Event-processing Thread.
- `PrecisionEventThread` — deterministic periodic execution combined with Event processing.
- `EventPriority` and `EventDispatchMethod`.
- `EventTransportManager` and `IEventTransport`.

# Type identity and RTTI

Event routing is RTTI-free.

Every concrete routable Event must provide a compiler-backed `EventTypeKey`. The normal way to do that is to inherit from `TypedEvent<TDerived>`:

```cpp
#include <ESPressio_Event.hpp>

class TemperatureChangedEvent final :
    public ESPressio::Event::TypedEvent<TemperatureChangedEvent> {
private:
    const float _previous;
    const float _current;

public:
    TemperatureChangedEvent(float previous, float current) :
        _previous(previous),
        _current(current) {}

    float GetPrevious() const { return _previous; }
    float GetCurrent() const { return _current; }
};
```

`Event<TTime>` supplies lifecycle and timing behavior but does not supply a concrete Event identity. A concrete routable Event therefore normally derives from `TypedEvent<TDerived, TTime>` rather than directly from `Event<TTime>`.

There is no RTTI migration fallback. Listener dispatch and Event Transport use `EventTypeKey` directly.

# Dispatching Events

Dispatch FIFO-style with `Queue()`:

```cpp
(new TemperatureChangedEvent(21.0f, 21.5f))->Queue();
```

or LIFO-style with `Stack()`:

```cpp
(new TemperatureChangedEvent(21.0f, 21.5f))->Stack();
```

Once dispatched, application code should treat an Event as immutable and should not retain ownership of the raw pointer. Event infrastructure manages its lifetime while interested receivers process it.

# Listening for Events

Listener registration is typed and uses compiler-backed Event identity:

```cpp
Event::EventListenerHandlePtr handle =
    eventThread.RegisterListener<TemperatureChangedEvent>(
        [](TemperatureChangedEvent* event,
           Event::EventDispatchMethod,
           Event::EventPriority) {
            // consume event
        }
    );
```

Keep the returned `EventListenerHandlePtr` alive for as long as the listener should remain registered.

Listener registration/unregistration is safe during dispatch. Listener storage uses stable records and deferred compaction so dispatch does not copy complete listener vectors or copy the callback merely to survive re-entrant registration.

# `EventThread`

Event-aware Threads use the explicit Threads ownership policy. Boolean `freeOnTerminate` constructors are not supported.

```cpp
class WorkerThread final : public Event::EventThread {
public:
    WorkerThread() :
        Event::EventThread(
            Threads::ThreadReleasePolicy::ExplicitRelease
        ) {}
};
```

For an automatically released heap-owned Thread, use:

```cpp
Threads::ThreadReleasePolicy::ReleaseOnTerminate
```

This keeps lifecycle ownership explicit throughout the ESPressio stack.

# `PrecisionEventThread`

`PrecisionEventThread` combines periodic deterministic work with Event reception. Its construction is also policy-based:

```cpp
class SetpointEvent final :
    public Event::TypedEvent<SetpointEvent> {
public:
    const int Setpoint;
    explicit SetpointEvent(int value) : Setpoint(value) {}
};

class ControlThread final : public Event::PrecisionEventThread<> {
private:
    int _setpoint = 0;

protected:
    void OnIteration(
        IterationTime,
        IterationTime,
        Threads::SkippedIterationCount
    ) override {
        // deterministic periodic work
    }

public:
    ControlThread() :
        Event::PrecisionEventThread<>(
            Threads::ThreadReleasePolicy::ExplicitRelease
        ) {}

    void ApplySetpoint(SetpointEvent* event) {
        _setpoint = event->Setpoint;
    }
};
```

Applications can select whether pending Events are processed before or after each iteration and how Events arriving between iteration boundaries are handled.

See:

```text
examples/PrecisionEventThread/PrecisionEventThread.ino
```

# Event lifecycle timing

`Event<TTime>` uses ESPressio Timing for lifecycle timestamps. The default public representation is `Timing::DefaultClockTime`.

```cpp
auto dispatched = event.GetDispatchTime();
auto age = event.GetTimeSinceDispatch();
```

The first dispatch timestamp is retained if the same Event is redispatched. Type-erased infrastructure also exposes nanosecond timing values so routing internals do not depend on a particular public Unit representation.

# Bounded Event queues and diagnostics

Event receiver queues are bounded by default so an embedded application cannot grow pending Event storage without limit merely because a consumer falls behind.

The default maximum can be configured with:

```cpp
ESPRESSIO_EVENT_DEFAULT_MAX_PENDING_EVENT_COUNT
```

Queue diagnostics expose current/peak pending Events and rejected/dropped Event counts.

# Serializable Events

Serializable support is optional. Local-only Events do not require ESPressio Serializable.

A Serializable Event automatically participates in typed Event identity:

```cpp
class OperatorCommandEvent final :
    public Event::SerializableEvent<OperatorCommandEvent> {
public:
    // serializable members/schema
};
```

Event Transport uses the existing EVTT transport envelope and ESPB v2 Serializable payload representation. The RTTI-free changes do not change the wire type ID, envelope, schema versioning or payload representation.

# Runtime Serializable Event discovery

The Serializable Event registry can be inspected without compile-time knowledge of every concrete Event type:

```cpp
auto& manager = Event::EventTransportManager::GetInstance();

for (const auto& descriptor :
     manager.GetRegisteredSerializableEvents()) {
    // descriptor.TypeID
    // descriptor.TypeName
    // descriptor.SchemaVersion
    // descriptor.Properties
    // descriptor.CanConstruct
}
```

Descriptors are snapshots; callers do not gain mutable references to Event Transport's private registration table.

# Event Transport

Event owns the transport-neutral contract:

```text
Serializable Event
       |
       v
EventTransportManager
       |
       v
IEventTransport
       |
       +--> concrete transport supplied downstream
```

Concrete transports are provided by libraries such as ESPressio ESP-Now and ESPressio Sockets.

The Event Transport registration path stores immutable runtime registration metadata once and queued inbound/outbound work retains shared references to that metadata rather than deep-copying complete registration records. Runtime Event matching uses `EventTypeKey`; no `std::type_index`, `typeid`, or `dynamic_cast` routing fallback is required.

# Timing/SystemClock Event bridge

Timing is a required upstream dependency of Event, so its Observer-to-Event bridge lives in Event without introducing a reciprocal dependency.

```cpp
#include <ESPressio_SystemClockEventBridge.hpp>
```

A Serializable counterpart is also available when Serializable Event transport of Timing lifecycle observations is required:

```cpp
#include <ESPressio_SystemClockEventBridge_Serializable.hpp>
```

# Threads infrastructure Event bridges

Threads is a required upstream dependency of Event. Event supplies asynchronous representations for the infrastructure that exists in the current Threads contract:

```cpp
#include <ESPressio_ThreadEventBridges.hpp>
```

and, where needed:

```cpp
#include <ESPressio_ThreadEventBridges_Serializable.hpp>
```

The current bridges cover ThreadManager cleanup/lifecycle and ThreadTerminationDispatcher observations. The removed legacy Thread Garbage Collector subsystem is not emulated by Event and there are no `ThreadGarbageCollection*Event` compatibility types or bridges.

See:

```text
examples/ThreadInfrastructureEventBridges/
```

# Dependency direction

Event does not own concrete Event representations for ESPressio Command, Security, Sockets or ESP-Now. Those integrations live with their owning libraries.

```text
Command  - - -> Event
Security - - -> Event
Sockets  - - -> Event
ESP-Now  - - -> Event
```

Event CI enforces that reverse dependencies do not return.

# Dependencies

The active working branch consumes the corresponding lead dependency branches through `library.json`:

```text
ESPressio Threads    optimisation/69-resource-footprint
ESPressio Observable feature/16-rtti-free-observer-registry
ESPressio Timing     main
```

Optional Serializable Event/Event Transport support consumes ESPressio Serializable when the application enables those headers.

# Installation

Released PlatformIO usage remains:

```ini
lib_deps =
    espressio-development-platform/ESPressio-Event@^6.0.3
```

For development against this unreleased branch, consume the branch directly with PlatformIO/Git.

# Examples

Current examples include:

```text
examples/EventTransportLoopback/
examples/EventTransportMultiTransportRouting/
examples/PrecisionEventThread/
examples/RuntimeSerializableEvents/
examples/SerializableEvent/
examples/SystemClockEventBridge/
examples/ThreadInfrastructureEventBridges/
```

# Compatibility

ESPressio Event targets ESP32-family microcontrollers using Arduino-ESP32 and C++17. The active architecture is designed to compile with RTTI disabled. Concrete routable Events use compiler-backed type identity and Event-aware Thread ownership uses `Threads::ThreadReleasePolicy` explicitly.

# License

Licensed under the Apache License 2.0. See [LICENSE](LICENSE).

# Changelog

See [CHANGELOG.md](CHANGELOG.md) for release-by-release history.
