#pragma once

#include "ESPressio_EventTransportTypes.hpp"

namespace ESPressio::Event {

class IEventTransport;

/// <summary>Receives complete inbound wire packets from a registered Event transport.</summary>
class IEventTransportReceiver {
public:
    virtual ~IEventTransportReceiver() = default;

    /// <summary>Hands an inbound packet to the Event transport-management layer.</summary>
    /// <param name="transport">Transport that received the packet.</param>
    /// <param name="data">Non-owning pointer to the complete wire packet.</param>
    /// <param name="size">Packet length in bytes.</param>
    virtual void ReceiveEventTransportPacket(
        IEventTransport* transport,
        const uint8_t* data,
        std::size_t size
    ) = 0;
};

/// <summary>Transport-neutral byte-delivery contract used by the Event transport manager.</summary>
/// <remarks>Implementations own their physical/network delivery mechanism; Event owns serialization, routing metadata, and inbound dispatch.</remarks>
class IEventTransport {
public:
    virtual ~IEventTransport() = default;

    /// <summary>Attempts to hand an outbound serialized Event packet to the transport.</summary>
    /// <param name="packet">Non-owning packet view valid for the duration of the call.</param>
    /// <returns>True when the transport accepts the packet for delivery.</returns>
    virtual bool Send(const EventTransportPacket& packet) = 0;

    /// <summary>Sets the receiver that should consume complete inbound Event wire packets.</summary>
    /// <param name="receiver">Non-owning receiver pointer, or null to detach inbound delivery.</param>
    virtual void SetReceiver(IEventTransportReceiver* receiver) = 0;
};

}
