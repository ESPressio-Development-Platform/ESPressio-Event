# Event Types and Identity

ESPressio Event routes concrete Event types without C++ RTTI.

## `IEvent`, `Event<TTime>` and `TypedEvent`

The public hierarchy separates type-erased infrastructure from concrete Event identity:

- `IEvent` is the type-erased interface used by routing infrastructure.
- `Event<TTime>` supplies lifecycle and timestamp behaviour.
- `TypedEvent<TDerived, TTime>` supplies compiler-backed identity for a concrete routable Event.

A normal concrete Event should therefore derive from `TypedEvent<TDerived>` rather than directly from `Event<TTime>`.

```cpp
#include <ESPressio_Event.hpp>

class TemperatureChangedEvent final :
    public ESPressio::Event::TypedEvent<TemperatureChangedEvent> {
public:
    TemperatureChangedEvent(float previous, float current) :
        Previous(previous), Current(current) {}

    const float Previous;
    const float Current;
};
```

## `EventTypeKey`

Each routable Event exposes a compiler-backed `EventTypeKey`. The key is used by listener dispatch and Event Transport runtime matching.

The 1.0.0 architecture deliberately has no `typeid`, `std::type_index` or `dynamic_cast` routing fallback. This keeps Event usable in builds with RTTI disabled and avoids RTTI registry/storage overhead.

## Immutability after dispatch

Populate an Event before dispatch. Once `Queue()` or `Stack()` transfers it to Event infrastructure, application code should treat it as immutable and should not retain ownership of the raw pointer.

The Event object is the data contract between producer and consumers; mutation during asynchronous fan-out would make receiver behaviour timing-dependent.

## Serializable Events

When an Event must cross a transport boundary, derive from `SerializableEvent<TDerived>`. Serializable Events participate in the same typed identity model while also providing the schema information required by Event Transport.

Local-only Events do not need ESPressio Serializable.

## Distributed Event type identity

Every Event contract that crosses a transport boundary declares an explicit, stable, non-zero 64-bit `EventTypeId` together with a diagnostic name:

```cpp
ESPRESSIO_EVENT_TRANSPORT_TYPE(
    TemperatureChangedEvent,
    0xF10D000000001001ULL,
    "application.temperature-changed.v1"
)
```

The numeric value is the distributed identity. The name is for diagnostics and runtime discovery only; it is never hashed or otherwise used to derive identity. Contract owners must allocate values deliberately and keep them stable across every producer and consumer of that contract. Zero remains Invalid/Unspecified.

Each conceptual Event occurrence receives one `EventMessageId` from a non-wrapping monotonic sequence scoped by authenticated source identity plus source incarnation. The same identifier survives serialization, transport fan-out, retries and downstream Mesh deliveries. A new causal Event receives a new identifier even when it correlates to an earlier occurrence.

## Design rule for extensions

New Event infrastructure must consume `EventTypeKey` rather than adding another runtime type mechanism. A feature that requires RTTI to discover the concrete Event type is outside the 1.0.0 Event routing contract.
