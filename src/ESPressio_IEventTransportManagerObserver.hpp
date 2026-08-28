#pragma once

#include <ESPressio_IObserver.hpp>

#include "ESPressio_EventTransportTypes.hpp"
#include "ESPressio_IEventTransport.hpp"

namespace ESPressio::Event {

/// <summary>Observes Event transport registration, routing, and inbound/outbound transaction activity.</summary>
/// <remarks>Callbacks have default no-op implementations so observers may subscribe only to the transport activity they require.</remarks>
class IEventTransportManagerObserver :
    public virtual Observable::IObserver {

public:
    virtual ~IEventTransportManagerObserver() = default;

    /// <summary>Called when a transport first becomes registered with the manager.</summary>
    virtual void OnEventTransportRegistered(
        IEventTransport*
    ) {}

    /// <summary>Called when a transport is completely removed from the manager.</summary>
    virtual void OnEventTransportUnregistered(
        IEventTransport*
    ) {}

    /// <summary>Called when a transported Event type is first registered.</summary>
    virtual void OnEventTransportTypeRegistered(
        uint64_t,
        EventTransportDirection
    ) {}

    /// <summary>Called when the aggregate registered direction for a transported Event type changes.</summary>
    virtual void OnEventTransportTypeRegistrationChanged(
        uint64_t,
        EventTransportDirection,
        EventTransportDirection
    ) {}

    /// <summary>Called when directions are removed from a transported Event type registration.</summary>
    virtual void OnEventTransportTypeUnregistered(
        uint64_t,
        EventTransportDirection,
        EventTransportDirection
    ) {}

    /// <summary>Called when a concrete transport route is first registered for an Event type.</summary>
    virtual void OnEventTransportTypeRouteRegistered(
        uint64_t,
        IEventTransport*,
        EventTransportDirection
    ) {}

    /// <summary>Called when the registered direction for one concrete transport route changes.</summary>
    virtual void OnEventTransportTypeRouteChanged(
        uint64_t,
        IEventTransport*,
        EventTransportDirection,
        EventTransportDirection
    ) {}

    /// <summary>Called when directions are removed from one concrete transport route.</summary>
    virtual void OnEventTransportTypeRouteUnregistered(
        uint64_t,
        IEventTransport*,
        EventTransportDirection,
        EventTransportDirection
    ) {}

    /// <summary>Called when an outbound Event is accepted for transport routing.</summary>
    virtual void OnOutboundEventAccepted(
        uint64_t,
        uint64_t
    ) {}

    /// <summary>Called when an outbound Event is accepted for delivery by a specific registered transport route.</summary>
    virtual void OnOutboundEventAcceptedForTransport(
        uint64_t,
        uint64_t,
        IEventTransport*
    ) {}

    /// <summary>Called after a serialized outbound Event is handed to a concrete transport.</summary>
    virtual void OnOutboundEventHandedToTransport(
        uint64_t,
        uint64_t,
        IEventTransport*,
        bool
    ) {}

    /// <summary>Called when an inbound wire packet passes transport-envelope and route validation.</summary>
    virtual void OnInboundPacketAccepted(
        uint64_t,
        uint64_t,
        IEventTransport*
    ) {}

    /// <summary>Called when an inbound wire packet is rejected before Event deserialization/dispatch.</summary>
    virtual void OnInboundPacketRejected(
        uint64_t,
        uint64_t,
        IEventTransport*
    ) {}

    /// <summary>Called after an inbound packet has been deserialized into its concrete Event type.</summary>
    virtual void OnInboundEventDeserialized(
        uint64_t,
        uint64_t
    ) {}

    /// <summary>Called after an inbound Event has been queued or stacked into local Event dispatch.</summary>
    virtual void OnInboundEventDispatched(
        uint64_t,
        uint64_t
    ) {}

    /// <summary>Receives the unified immutable snapshot for an Event transport transaction stage.</summary>
    /// <remarks>Event, transport, and payload pointers in the snapshot are borrowed and valid only for the duration of this callback. Copy any data that must outlive the notification.</remarks>
    virtual void OnEventTransportTransaction(
        const EventTransportTransaction&
    ) {}
};

}
