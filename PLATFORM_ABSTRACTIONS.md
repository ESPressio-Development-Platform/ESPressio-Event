# Platform Abstractions Audit Trail

This file records Event changes made during the platform-abstraction tranche tracked by issue #60.

## 2026-08-27

### Thread wake-up signalling
- Replaced FreeRTOS task handles and task-notification calls in `EventThreadBase` with `ESPressio::System::Synchronization::ISignal`.
- Replaced the same FreeRTOS notification mechanism in `EventManager` with a System binary signal.
- Replaced the same native task-notification wake-up mechanism in `EventTransportManager` with a System binary signal.
- Preserved the existing lost-wakeup protection: when work is already pending, the signal is consumed non-blockingly and work is drained immediately; otherwise the worker waits indefinitely on the shared System signal abstraction.
- Event dispatch, transport routing, queue ownership and `ESPressio::Threads::Thread` lifecycle semantics remain unchanged.

### Final runtime boundary
- Event infrastructure no longer owns FreeRTOS task handles or uses FreeRTOS task-notification APIs for its worker wake paths.
- Generic synchronization and wake-up behavior is supplied by ESPressio-System/Threads rather than duplicated in Event.
- Platform-specific execution remains below the Event layer; Event retains only event-domain dispatch, routing, transport-envelope and lifecycle responsibilities.

## Boundary rule

Event owns event dispatch, routing, transport envelopes and event lifecycle. Thread execution and wake-up primitives are supplied by ESPressio-Threads/System.
