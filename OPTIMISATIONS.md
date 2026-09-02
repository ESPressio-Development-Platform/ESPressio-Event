# Optimisations

## 2026-08-27

- **#58** Added ESPressio-System as the platform-neutral memory abstraction dependency.
- **#58** Moved Event listener deque storage to `ExternalPreferred` memory while preserving stable-record re-entrant dispatch semantics.
- **#58** Moved Event listener handle allocations to `ExternalPreferred` memory without changing `EventListenerHandlePtr`.
- **#58** Moved teardown type bookkeeping to external-preferred storage.
- **#58** Preserved direct in-place callback invocation; no listener `std::function` copy was reintroduced.
- **#58** Moved Event transport registration maps, per-transport route maps, registered-transport storage and inbound/outbound work queues to `ExternalPreferred` System containers.
- **#58** Moved inbound packet backing and bulk-operation/target-transport scratch away from worker-stack/default-heap storage where ownership must cross the manager lock.
- **#58** Allocated immutable runtime transport-registration objects with the System external-preferred allocator while preserving their `shared_ptr` lifetime contract.
