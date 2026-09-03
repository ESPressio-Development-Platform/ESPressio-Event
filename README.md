# ESPressio Event

Generic Event-Driven Development infrastructure for the ESPressio Development Platform.

ESPressio Event provides the asynchronous counterpart to ESPressio Observable: producers dispatch strongly typed occurrence contracts without knowing which consumers exist, while listeners process those Events independently on Event-aware Threads.

## 1.0.0 baseline

The structural-realignment branch is the source of truth for the platform-wide 1.0.0 baseline. Historical Event releases and transport contracts are not compatibility targets for this tranche.

# Why Event-Driven Development?

Event-driven design is useful when the producer of an occurrence should not depend on the implementation, execution context, or even existence of its consumers.

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

The Event itself is the occurrence data contract. The producer populates that contract; listeners read it. The producer never needs to retain references to listeners.

`Queue()` and `Stack()` are asynchronous. Independent listeners do not have a meaningful globally guaranteed execution order. An Event is one-way occurrence information: it does not intrinsically contain acknowledgement, response, completion, RPC, transport-route, or hop semantics.

# Namespace

```cpp
ESPressio::Event
```

Important public concepts include:

- `IEvent` — type-erased Event interface used by routing infrastructure.
- `Event<TTime>` — lifecycle/timestamp implementation base.
- `TypedEvent<TDerived, TTime>` — concrete RTTI-free local Event identity base.
- `SerializableEvent<TDerived>` — Serializable Event base with typed identity.
- `EventMetadata` — transport-independent conceptual Event metadata backed by ESPressio Primitive vocabulary.
- `EventDispatchContext` — local/remote provenance belonging to one dispatch operation rather than the Event object.
- `EventManager` — central local dispatch/routing manager.
- `EventListener` / listener handles — type-specific consumer registration.
- `EventThread` — asynchronous Event-processing Thread.
- `PrecisionEventThread` — deterministic periodic execution combined with Event processing.
- `EventPriority` and `EventDispatchMethod` — local/application dispatch choices, not Mesh QoS.
- `EventTransportManager` and `IEventTransport` — optional Event-family transport integration.

# Type identity and RTTI

Local Event routing is RTTI-free.

Every concrete locally routable Event provides a compiler-backed `EventTypeKey`. The normal way to do that is to inherit from `TypedEvent<TDerived>`:

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

`EventTypeKey` is a local-process routing key and is not itself the distributed/wire `EventTypeId`. `Event<TTime>` supplies lifecycle and timing behavior but does not supply a concrete Event identity. A concrete routable Event therefore normally derives from `TypedEvent<TDerived, TTime>` rather than directly from `Event<TTime>`.

There is no RTTI migration fallback. Listener dispatch uses `EventTypeKey` directly.

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

# Dispatch provenance

Transport provenance is deliberately not stored on the Event object. `EventDispatchContext` accompanies a queued reference through EventManager, EventDispatcher, EventThread/PrecisionEventThread, listeners and observers.

For the 1.0.0 Event layer the context contains only transport-independent provenance:

```cpp
Event::EventOrigin::Local
Event::EventOrigin::Remote
```

Transport-local message identifiers, addresses, routes and hop counts do not belong to Event. In particular, a remotely received Event is dispatched locally with `Remote` provenance and is not automatically transmitted onward again.

# Listening for Events

Listener registration is typed and receives the dispatch context explicitly:

```cpp
Event::EventListenerHandlePtr handle =
    eventThread.RegisterListener<TemperatureChangedEvent>(
        [](TemperatureChangedEvent* event,
           Event::EventDispatchMethod,
           Event::EventPriority,
           const Event::EventDispatchContext& context) {
            if (context.Origin == Event::EventOrigin::Remote) {
                // This occurrence arrived through an Event-family transport.
            }
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

Applications can select whether pending Events are processed before or after each iteration and how Events arriving between iteration boundaries are handled. Dispatch provenance is preserved whichever execution policy is selected.

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

Each retained queue entry contains the Event reference, deterministic sequence and compact dispatch provenance. Queue diagnostics expose current/peak pending Events and rejected/dropped Event counts.

# Serializable Events

Serializable support is optional. Local-only Events do not require ESPressio Serializable.

A Serializable Event automatically participates in typed local Event identity:

```cpp
class OperatorCommandEvent final :
    public Event::SerializableEvent<OperatorCommandEvent> {
public:
    // serializable members/schema
};
```

Event Transport uses the bounded EVTT envelope plus the ESPressio Serializable binary payload representation. The structural realignment intentionally advances the EVTT envelope to version 2 and removes hop-count semantics from that envelope. Historical envelope compatibility is not preserved for the 1.0.0 restructuring.

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

Event owns its Event-family transport integration without owning any physical or Mesh route semantics:

```text
Serializable Event
       |
       v
EventTransportManager
       |
       v
IEventTransport
       |
       +--> concrete transport/adaptor supplied downstream
```

Outbound transport subscription applies only to locally originated dispatches. Inbound packets are deserialized and submitted to EventManager with `EventOrigin::Remote`; that provenance travels beside the local dispatch and prevents automatic re-forwarding by EventTransportManager.

The Event Transport registration path stores immutable runtime registration metadata once and queued inbound/outbound work retains shared references to that metadata rather than deep-copying complete registration records. Runtime local Event matching uses `EventTypeKey`; no `std::type_index`, `typeid`, or `dynamic_cast` routing fallback is required.

# Primitive vocabulary

Event depends on `ESPressio-Primitive` for common conceptual-message vocabulary. `EventMessageId`, `EventCorrelationId` and `EventProtocolVersion` are aliases of the corresponding Primitive types. `EventMetadata` keeps this conceptual identity separate from transport-specific envelope mechanics.

Concrete ESPressio `PrimitiveFamilyId` numeric allocation is intentionally not performed in this branch until the platform-wide family registry allocation is explicitly settled.

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

For this Mesh propagation tranche, required dependencies are pinned as follows:

```text
ESPressio System     structural_realignment_propagation_ESPressio-Mesh
ESPressio Primitive  structural_realignment_propagation_ESPressio-Mesh
ESPressio Task       structural_realignment
ESPressio Threads    structural_realignment
ESPressio Observable structural_realignment
ESPressio Timing     structural_realignment
```

Optional Serializable Event/Event Transport support consumes ESPressio Serializable from `structural_realignment` when the application enables those headers.

# Installation

During this structural-realignment tranche, consume Event from the propagation branch together with the matching participating dependencies:

```ini
lib_deps =
    https://github.com/ESPressio-Development-Platform/ESPressio-Event.git#structural_realignment_propagation_ESPressio-Mesh
```

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

ESPressio Event targets ESP32-family microcontrollers using Arduino-ESP32 and C++17. The active architecture is designed to compile with RTTI disabled. Concrete locally routable Events use compiler-backed type identity and Event-aware Thread ownership uses `Threads::ThreadReleasePolicy` explicitly.

# License

Licensed under the Apache License 2.0. See [LICENSE](LICENSE).

# Changelog

See [CHANGELOG.md](CHANGELOG.md) for the current 1.0.0 restructuring history.
