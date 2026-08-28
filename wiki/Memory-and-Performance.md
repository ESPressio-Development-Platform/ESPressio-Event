# Memory and Performance

ESPressio Event is designed for asynchronous embedded workloads where both queue growth and infrastructure allocation must remain controlled.

## Bounded queues

Pending receiver work is bounded. Configure capacity for the expected burst rate and receiver latency rather than relying on unbounded heap growth.

Monitor current/peak pending counts and rejected/dropped Event diagnostics during hardware validation.

## Stable listener records

Listener dispatch uses stable records and deferred compaction. Registration/unregistration during dispatch therefore does not require a full listener-vector snapshot for every notification.

Callbacks are invoked in place; do not reintroduce recurring `std::function` copies merely to cross a lock boundary.

## External-preferred ESPressio storage

The 1.0.0 optimisation baseline moves appropriate ESPressio-owned variable-size state to System `ExternalPreferred` storage, including:

- listener deque storage and listener handles;
- teardown type bookkeeping;
- Event Transport registration and route maps;
- registered transport collections;
- inbound/outbound work queues;
- immutable runtime transport-registration allocations;
- packet/scratch backing whose lifetime crosses manager locks.

`ExternalPreferred` means prefer suitable external memory and retain the System allocator's fallback semantics; it does not mean that every allocation is guaranteed to reside in PSRAM.

## Native/platform memory remains native

Do not apply ESPressio container policy blindly to driver-, RTOS-, DMA- or hardware-owned allocations. Capability and lifetime requirements take precedence over generic heap pressure reduction.

## Lock boundaries

Snapshots or shared immutable records are retained where required to avoid holding Event manager locks across transport or user callbacks. Removing those boundaries merely to save memory can introduce deadlocks, reentrancy failures or excessive contention.

## Practical profiling

Profile on representative hardware with realistic Event rates and all intended transports active. Useful observations include:

- internal free heap and largest block;
- external heap usage;
- pending/peak Event counts;
- rejected/dropped counts;
- worker stack high-water marks;
- transport queue depth and burst behaviour.

Memory optimisation is accepted only when lifecycle, reentrancy and dispatch semantics remain intact.