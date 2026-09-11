# Event tranche implementation and validation

Authorization: the user explicitly authorized the complete redesign and superseded historical planning prohibitions and tranche authorization pauses. No versioning, tagging, release publication or main integration is included. Event baseline was revalidated at `34b4efe6e64e8de317fd5ee399d165db3ea8ba5b` before modifications; implementation remains on `primitives_redesign`.

## Implemented contracts

E1 supplies Local/Serializable/Transmissible CRTP tiers, strong IDs, immutable occurrence facts, fixed aligned in-place pools, explicit pointer-sized leases, a nonwrapping sequence, one T1 lane per Type, frozen intrusive targets and a private/shared pointer FIFO capability. E2 supplies the exact 53-byte header, bounded codecs in three formats, full semantic fingerprints, caller-owned remote receipt capacity, provisional/committed admission and no remote re-egress. The family runtime validates/finalizes before its atomic admission publication. Outbound binding is a fixed downward-facing family seam; generic Adapter byte ownership and scheduling are implemented in their own dependency tranche.

Shutdown wakes blocked producers, quiesces consumers without invoking their queued callbacks and joins lane execution. Fixed capacity notifications remain valid for late lease release. The bounded Timing bridge exposes explicit diagnostic notifications only; clock reads have no Event side effects. Resource profiles report contained layout components and explicitly unknown platform control costs.

## Existing test classification, performed before execution

All six predecessor sources were read in full. None was executed as a behavioral acceptance oracle.

| Predecessor test | Disposition and preserved semantic requirement |
| --- | --- |
| broker_nonblocking | Replace manager/receiver mechanism with healthy-target fanout despite a saturated target |
| dispatch_completion | Replace observer completion with root-lane acceptance and retained lease lifetime |
| dispatch_context | Remove mutable route/context; test immutable original identity and time |
| observer | Remove dynamic registration/predicates; test fixed listener validation and fatal callback exception |
| references | Remove heap/delete-this/priority/drop modes; preserve pool/refcount/quota/backpressure intent |
| transport_packet_ownership | Remove shared dynamic packet engine; test exact wire and Type metadata here; generic byte ownership belongs to Adapters |
| RTTI-disabled observer duplicate | Replace with no-RTTI compilation across all public headers, README examples and runtime tests |

The source, examples and workflow predecessor mechanisms were removed rather than retained as aliases.

## Validation mapping

| Executable or gate | Required semantics exercised |
| --- | --- |
| pool_lifetime | Fixed alignment, reservation rollback, retained pointer leases, concurrent final references and no heap |
| type_compile_contract | Strong Type contract, detached copies, nonzero sequence and final-ID exhaustion |
| dispatch_lane | Single root lane, healthy first-pass fanout, owed-target wake and local qualified time |
| thread_capability | One mixed FIFO, exact private/shared quota, no borrowing, fixed listener, Precision composition and fatal exception |
| runtime_freeze | Frozen P1, missing/duplicate listener rejection, atomic Start and late topology rejection |
| wire_v1 | 53-byte golden offsets/endianness, truncation, exact length and all invalid reliability values |
| remote_receipts | InProgress duplicate, explicit capacity, no live eviction, abort/retry, expiry and overflow |
| remote_admission | Typed wire admission, malformed/unknown/version/max rejection, failed decode frees receipt, origin preservation and no re-egress |
| timing_bridge | Pure clock reads under heap denial, explicit bootstrap notification and sealed clock behavior |
| blocking_admission | Entry time before wait, no construction on rejection, capacity wake and shutdown baton for multiple producers |
| formats_and_contract | DirectBinary/CBOR/JSON escaped and bounded payload round trips, full fingerprint semantic equivalence/difference, source/inbound/shutdown heap denial and resource accounting |
| initialization_rollback | Second-Type signal failure rolls back an already-created first Type; retry and failed constructor return capacity |
| remote_contention | Concurrent duplicate during decode constructs once, lane saturation, pool saturation by retained leases and repaired retry |
| thread_pause_quiesce | Pause retains FIFO, quota releases before callback, next admission during callback and termination drops queued work without callbacks |
| validate_public_surface.py | Every public header, README C++ block and maintained example compiles without RTTI; invalid IDs/capacities/compositions/listeners/lease copy/unbounded transmission/missing policy fail compilation |
| check_dependency_boundaries.py | Exact production manifest DAG, forbidden reverse includes and active predecessor eradication |

Native tests use actual dependency headers with a real cooperative host-thread provider; only Arduino String declarations needed by Units are stubbed. Runtime tests compile with C++17, warnings as errors, RTTI disabled and assertions enabled. Test scheduling waits are bounded host-fixture synchronization, not production polling. Source hot-path allocation denial is exercised directly. GitHub Actions additionally compiles the maintained examples against actual ESP32/Arduino headers and working-branch dependencies.

## Dependency and migration status

Direct production dependencies are System, Primitive, Task, Threads, Timing and Serializable. The native/ESP32 fixture also checks out Units and Observable for Timing's transitive closure. The old physical Event transport engine and Thread capability-combination hierarchy are absent from Event. Downstream repositories migrate in their specified later tranches; this report does not claim platform-wide eradication until that final scan is performed.

Hardware-specific cooperative execution, durable identity provisioning and clock certification require the platform tranche and physical validation. Host passes do not certify those hardware guarantees. Any unavailable automation must be recorded from actual evidence; no quota failure has been observed during this tranche's local development.
