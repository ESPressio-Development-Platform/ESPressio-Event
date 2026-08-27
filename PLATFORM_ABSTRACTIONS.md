# Platform Abstractions Audit Trail

This file records Event changes made during the platform-abstraction tranche tracked by issue #60.

## 2026-08-27

### Thread wake-up signalling
- Replaced FreeRTOS task handles and task-notification calls in `EventThreadBase` with `ESPressio::System::Synchronization::ISignal`.
- Replaced the same FreeRTOS notification mechanism in `EventManager` with a System binary signal.
- Preserved the existing lost-wakeup protection: when events are already pending, the signal is consumed non-blockingly and work is drained immediately.
- Event dispatch, queue ownership and `ESPressio::Threads::Thread` lifecycle semantics remain unchanged.

### Remaining work
- `EventTransportManager` still contains the same FreeRTOS task-notification wake-up pattern and must be migrated to the System signal before issue #60 is complete.
- Perform a final source-wide target-runtime audit after that migration.

## Boundary rule

Event owns event dispatch, routing, transport envelopes and event lifecycle. Thread execution and wake-up primitives are supplied by ESPressio-Threads/System.
