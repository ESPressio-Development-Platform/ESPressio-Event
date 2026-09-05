# Event Transport

Event owns the transport-neutral machinery required to move Serializable Events between local Event infrastructure and concrete transports.

```text
Serializable Event
       |
       v
EventTransportManager
       |
       v
IEventTransport
       |
       +--> concrete transport supplied downstream
```

Concrete media/protocol implementations belong to downstream transport libraries such as ESPressio ESP-Now, Serial or Sockets rather than to Event itself.

## `IEventTransport`

A transport implementation adapts its medium to Event's transport-neutral contract. Event must not gain Wi-Fi, MAC-address, socket, UART or other medium-specific concepts merely to support a new transport.

## Registration metadata

Runtime transport registrations are immutable after construction and shared by queued work where lifetime must cross manager lock boundaries. The current memory-efficient implementation retains shared references to registration metadata instead of deep-copying complete registration records into each queued operation.

## Routing identity

Local runtime Event matching uses `EventTypeKey`. There is no RTTI routing fallback.

Serializable wire identity is an explicit stable non-zero 64-bit `EventTypeId` declared by the Event contract. Diagnostic names do not derive identity, and local compiler identity must never be substituted for the wire value.

Outbound conceptual message identifiers come from a non-wrapping monotonic generator whose authenticated source identity and source-incarnation scope is owned by composition. A continuing incarnation restores its non-regressing persisted high-water mark before the manager starts; a newly authenticated incarnation explicitly resets issuance to begin at `1`. One identifier is retained across serialization and physical-transport fan-out.

## Memory placement

The current 1.0.0 baseline uses ESPressio-System `ExternalPreferred` storage for ESPressio-owned variable-size Event Transport state, including registration maps, route maps, registered-transport storage and inbound/outbound work queues where appropriate.

Inbound packet backing and bulk-operation/target-transport scratch are likewise kept away from worker-stack/default-heap storage when ownership must cross a manager lock.

This policy does **not** authorize a transport implementation to move native driver/RTOS/DMA memory into external RAM where its platform requires internal or capability-specific storage.

## Extension rule

A new transport should implement the Event contract and keep medium-specific discovery, addressing, framing, retransmission and driver behaviour in its owning transport library. Event should remain representation- and medium-neutral.
