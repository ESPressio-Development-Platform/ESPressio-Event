# Event dependency boundary

| Direct production dependency | Purpose |
| --- | --- |
| System | Identity, synchronization, provider execution contracts |
| Primitive | Frozen metadata, fingerprints, IDs and delivery policy |
| Task | One bounded joinable dispatch lane per Type |
| Threads | Composable consumer capability and common work wake |
| Timing | Qualified origin observations and monotonic receipt expiry |
| Serializable | Bounded schemas and selected-format typed codecs |

Units and Observable are reached by Timing and the host validation fixture, not declared Event production dependencies. Generic Adapters and concrete transports depend on the Event binding seam. Event never imports those implementations. Run `python tools/check_dependency_boundaries.py` for the enforced boundary.
