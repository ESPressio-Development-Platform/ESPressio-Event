#pragma once

#include <utility>

#include "ESPressio_EventTransportTypes.hpp"

namespace ESPressio::Event {

class IEventTransport;

/// <summary>Receives complete ownership-bearing inbound wire packets from a registered Event transport.</summary>
class IEventTransportReceiver {
public:
    virtual ~IEventTransportReceiver() = default;

    /// <summary>Transfers a complete inbound packet into the Event transport-management layer.</summary>
    /// <remarks>The packet's immutable backing storage remains valid across asynchronous processing without another byte copy.</remarks>
    virtual void ReceiveEventTransportPacket(
        IEventTransport* transport,
        EventTransportPacket packet
    ) = 0;
};

/// <summary>Transport-neutral ownership-bearing byte-delivery contract used by Event transport routing.</summary>
/// <remarks>
/// Event owns serialization and routing metadata. A transport receives independent shared ownership of immutable serialized
/// bytes and may move that packet into its own asynchronous execution context without copying the payload.
/// </remarks>
class IEventTransport {
public:
    virtual ~IEventTransport() = default;

    /// <summary>Attempts to accept an owned serialized Event packet for delivery.</summary>
    /// <returns>True when the transport accepts the packet for delivery.</returns>
    virtual bool Send(EventTransportPacket packet) = 0;

    /// <summary>Sets the receiver that should consume complete inbound Event wire packets.</summary>
    virtual void SetReceiver(IEventTransportReceiver* receiver) = 0;
};

}
