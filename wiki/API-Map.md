# API Map

This page is a navigation map rather than a substitute for symbol-level API documentation.

## Event model

- `IEvent` — type-erased Event infrastructure interface.
- `Event<TTime>` — Event lifecycle and timestamp behaviour.
- `TypedEvent<TDerived, TTime>` — compiler-backed concrete Event identity.
- `EventTypeKey` — RTTI-free local routing identity.
- `EventPriority` — Event priority metadata.
- `EventDispatchMethod` — queue/stack dispatch method metadata.

## Dispatch and listeners

- `EventManager` — central Event dispatch/routing manager.
- `EventListener` and listener handle types — typed receiver registration/lifetime.
- `EventThread` — Thread with asynchronous Event reception.
- `PrecisionEventThread` — precision periodic Thread plus Event reception.

## Serialization and transport

- `SerializableEvent<TDerived>` — Serializable typed Event base.
- `EventTransportManager` — transport registration/routing and Serializable Event discovery.
- `IEventTransport` — transport-neutral Event transport contract.

## Integration bridges

- Timing System Clock Event bridge.
- Serializable Timing System Clock Event bridge.
- Threads infrastructure Event bridges.
- Serializable Threads infrastructure Event bridges.

## Upstream dependencies

Event builds on the contracts supplied by:

- ESPressio Threads;
- ESPressio Observable;
- ESPressio Timing;
- ESPressio System for the 1.0.0 memory-policy baseline.

ESPressio Serializable remains optional for Serializable Event/transport paths.

For conceptual behaviour and extension rules, follow the dedicated Wiki pages rather than inferring architecture solely from individual type signatures.