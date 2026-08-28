#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "ESPressio_EventEnums.hpp"

namespace ESPressio::Event {

/// <summary>Bit flags describing the directions in which an Event transport is registered.</summary>
enum class EventTransportDirection : uint8_t {
    None = 0,
    Inbound = 1u << 0,
    Outbound = 1u << 1,
    Bidirectional = (1u << 0) | (1u << 1)
};

constexpr EventTransportDirection operator|(
    EventTransportDirection a,
    EventTransportDirection b
) noexcept {
    return static_cast<EventTransportDirection>(
        static_cast<uint8_t>(a) |
        static_cast<uint8_t>(b)
    );
}

constexpr EventTransportDirection operator&(
    EventTransportDirection a,
    EventTransportDirection b
) noexcept {
    return static_cast<EventTransportDirection>(
        static_cast<uint8_t>(a) &
        static_cast<uint8_t>(b)
    );
}

constexpr EventTransportDirection operator~(
    EventTransportDirection a
) noexcept {
    return static_cast<EventTransportDirection>(
        ~static_cast<uint8_t>(a)
    );
}

/// <summary>Returns a direction mask with the requested direction bits removed.</summary>
constexpr EventTransportDirection RemoveDirection(
    EventTransportDirection value,
    EventTransportDirection remove
) noexcept {
    return static_cast<EventTransportDirection>(
        static_cast<uint8_t>(value) &
        ~static_cast<uint8_t>(remove)
    );
}

/// <summary>Tests whether every requested direction bit is present in a direction mask.</summary>
constexpr bool HasDirection(
    EventTransportDirection value,
    EventTransportDirection test
) noexcept {
    return (
        static_cast<uint8_t>(value) &
        static_cast<uint8_t>(test)
    ) == static_cast<uint8_t>(test);
}

/// <summary>Specifies whether pending work is completed or discarded when a transport direction is unregistered.</summary>
enum class EventTransportPendingAction : uint8_t {
    Complete,
    Discard
};

/// <summary>Controls treatment of pending inbound and outbound work during transport unregistration.</summary>
struct EventTransportUnregistrationOptions {
    EventTransportPendingAction PendingOutbound =
        EventTransportPendingAction::Complete;

    EventTransportPendingAction PendingInbound =
        EventTransportPendingAction::Complete;
};

/// <summary>Identifies whether an Event originated locally or was received from a remote transport.</summary>
enum class EventOrigin : uint8_t {
    Local,
    Remote
};

/// <summary>Routing metadata carried with an Event while it moves between local and remote dispatch paths.</summary>
struct EventDispatchContext {
    EventOrigin Origin = EventOrigin::Local;
    uint64_t TransportMessageID = 0;
    uint8_t HopCount = 0;

    constexpr bool operator==(
        const EventDispatchContext& other
    ) const noexcept {
        return
            Origin == other.Origin &&
            TransportMessageID ==
                other.TransportMessageID &&
            HopCount == other.HopCount;
    }

    constexpr bool operator!=(
        const EventDispatchContext& other
    ) const noexcept {
        return !(*this == other);
    }
};

#pragma pack(push, 1)
/// <summary>Fixed 32-byte wire envelope preceding a serialized transported Event payload.</summary>
struct EventTransportEnvelope {
    static constexpr uint32_t MagicValue =
        0x45565454u; // EVTT

    static constexpr uint8_t CurrentVersion = 1;

    uint32_t Magic = MagicValue;
    uint8_t Version = CurrentVersion;
    uint8_t DispatchMethod =
        static_cast<uint8_t>(
            EventDispatchMethod::Queue
        );

    uint8_t Priority =
        static_cast<uint8_t>(
            EventPriority::Normal
        );

    uint8_t HopCount = 0;
    uint64_t EventTypeID = 0;
    uint32_t SchemaVersion = 1;
    uint64_t MessageID = 0;
    uint32_t PayloadLength = 0;
};
#pragma pack(pop)

static_assert(
    sizeof(EventTransportEnvelope) == 32,
    "EventTransportEnvelope wire layout changed; increment protocol version deliberately."
);

/// <summary>Non-owning packet view handed to an Event transport for outbound delivery.</summary>
struct EventTransportPacket {
    const uint8_t* Data = nullptr;
    std::size_t Size = 0;
    uint64_t MessageID = 0;
};

/// <summary>Outcome of registering or updating one Event transport.</summary>
enum class EventTransportRegistrationResult : uint8_t {
    Registered,
    Updated,
    AlreadyRegistered,
    TypeConflict,
    InvalidTransport
};

/// <summary>Outcome of removing one or more directions from a registered Event transport.</summary>
enum class EventTransportUnregistrationResult : uint8_t {
    Updated,
    Removed,
    NotRegistered,
    InvalidTransport
};

/// <summary>Counts requested, changed, unchanged, and failed entries from a bulk transport operation.</summary>
struct EventTransportBulkOperationResult {
    std::size_t Requested = 0;
    std::size_t Changed = 0;
    std::size_t Unchanged = 0;
    std::size_t Failed = 0;
};

class IEvent;
class IEventTransport;

/// <summary>Lifecycle stage reported for an Event transport transaction.</summary>
enum class EventTransportTransactionStage : uint8_t {
    OutboundAccepted,
    OutboundSerialized,
    OutboundHandedToTransport,
    InboundAccepted,
    InboundRejected,
    InboundDeserialized,
    InboundDispatched,
    Failed
};

/// <summary>Diagnostic snapshot describing one stage of an inbound or outbound Event transport transaction.</summary>
/// <remarks>Event, transport, and payload pointers are non-owning views valid for the duration of the notification.</remarks>
struct EventTransportTransaction {
    EventTransportTransactionStage Stage =
        EventTransportTransactionStage::Failed;

    EventTransportDirection Direction =
        EventTransportDirection::None;

    uint64_t EventTypeID = 0;
    std::string_view EventTypeName{};
    uint32_t SchemaVersion = 0;
    uint64_t MessageID = 0;

    IEventTransport* Transport = nullptr;
    const IEvent* Event = nullptr;

    const uint8_t* Payload = nullptr;
    std::size_t PayloadSize = 0;

    EventDispatchMethod DispatchMethod =
        EventDispatchMethod::Queue;

    EventPriority Priority =
        EventPriority::Normal;

    EventOrigin Origin = EventOrigin::Local;
    uint8_t HopCount = 0;

    bool TransportAccepted = false;
};

/// <summary>Customization point supplying the stable wire name for a concrete transported Event type.</summary>
template<typename TEvent>
struct EventTransportTypeTraits {
    static constexpr std::string_view Name{};
};

/// <summary>Computes the stable 64-bit FNV-1a identifier for a transport type name.</summary>
constexpr uint64_t EventTransportTypeHash(
    std::string_view value
) noexcept {
    uint64_t hash =
        14695981039346656037ull;

    for (char c : value) {
        hash ^=
            static_cast<uint8_t>(c);

        hash *=
            1099511628211ull;
    }

    return hash;
}

/// <summary>Returns the stable wire type identifier for a transported Event type.</summary>
template<typename TEvent>
constexpr uint64_t EventTransportTypeID()
    noexcept {
    return
        EventTransportTypeHash(
            EventTransportTypeTraits<
                TEvent
            >::Name
        );
}

}

#define ESPRESSIO_EVENT_TRANSPORT_TYPE(Type, StableName) \
    namespace ESPressio::Event { \
        template<> struct EventTransportTypeTraits<Type> { \
            static constexpr std::string_view Name = StableName; \
        }; \
    }
