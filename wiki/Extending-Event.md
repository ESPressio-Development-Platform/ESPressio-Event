# Extending ESPressio Event

Extensions should preserve Event's role as a transport-neutral, asynchronous typed-dispatch layer.

## Adding Event types

Use `TypedEvent<TDerived>` for local routable Events and `SerializableEvent<TDerived>` when a transport representation is required.

Do not add RTTI-based identity or a parallel runtime type registry.

## Adding listener/dispatch behaviour

Preserve:

- safe registration/unregistration during dispatch;
- stable listener lifetime;
- no user callback while internal collection locks are held;
- bounded pending work;
- direct callback invocation without avoidable callable copies.

Changes in these areas require reentrancy and concurrency regression coverage.

## Adding transports

Implement `IEventTransport` in the downstream transport-owning library. Keep medium-specific addressing, discovery, framing and native driver concerns out of Event.

Use immutable/shared registration metadata when queued work must outlive the manager lock that selected it.

## Adding bridges

Only put a bridge in Event when the source library is upstream of Event. Downstream domain integrations belong in their owning library to prevent dependency cycles.

## Memory policy

Use ESPressio-System allocation/container facilities for ESPressio-owned variable-size state where the ownership/capability requirements permit it. Preserve internal/native placement for platform-owned resources that require it.

## Testing expectations

An extension should normally cover:

- typed routing with RTTI disabled;
- registration and unregistration during active dispatch;
- listener handle lifetime;
- queue saturation/rejection behaviour;
- Event ownership and destruction after fan-out;
- transport registration/unregistration races where applicable;
- serialization/wire compatibility for Serializable Events;
- memory-policy fallback when external memory is unavailable.

Hardware validation remains important for transport and resource-footprint changes even when host/CI tests are green.