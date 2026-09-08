#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>

#include <ESPressio_Memory.hpp>
#include <ESPressio_Primitive.hpp>

#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventTypes.hpp"

namespace ESPressio::Event {

/// <summary>Stable ESPressio primitive-family identifier for Event occurrences.</summary>
inline constexpr Primitive::PrimitiveFamilyId EventFamilyId = Primitive::FamilyIds::Event;

/**
 * ESPressio Memory Audit
 * Underlying storage: 1 bytes
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
enum
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 1 bytes [0 bytes dynamic allocation]
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class EventTransportDirection : uint8_t {
    None = 0,
    Inbound = 1u << 0,
    Outbound = 1u << 1,
    Bidirectional = (1u << 0) | (1u << 1)
};

constexpr EventTransportDirection operator|(EventTransportDirection a, EventTransportDirection b) noexcept {
    return static_cast<EventTransportDirection>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr EventTransportDirection operator&(EventTransportDirection a, EventTransportDirection b) noexcept {
    return static_cast<EventTransportDirection>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
constexpr EventTransportDirection operator~(EventTransportDirection a) noexcept {
    return static_cast<EventTransportDirection>(~static_cast<uint8_t>(a));
}
constexpr EventTransportDirection RemoveDirection(EventTransportDirection value, EventTransportDirection remove) noexcept {
    return static_cast<EventTransportDirection>(static_cast<uint8_t>(value) & ~static_cast<uint8_t>(remove));
}
constexpr bool HasDirection(EventTransportDirection value, EventTransportDirection test) noexcept {
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(test)) == static_cast<uint8_t>(test);
}

/**
 * ESPressio Memory Audit
 * Underlying storage: 1 bytes
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
enum
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 1 bytes [0 bytes dynamic allocation]
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class EventTransportPendingAction : uint8_t { Complete, Discard };

/**
 * ESPressio Memory Audit
 * Members:
 * - PendingOutbound (EventTransportPendingAction): 1 bytes [0 bytes dynamic allocation]
 * - PendingInbound (EventTransportPendingAction): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 2 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
struct EventTransportUnregistrationOptions {
    EventTransportPendingAction PendingOutbound = EventTransportPendingAction::Complete;
    EventTransportPendingAction PendingInbound = EventTransportPendingAction::Complete;
};

#pragma pack(push, 1)
/**
 * ESPressio Memory Audit
 * Members:
 * - Magic (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - Version (uint8_t): 1 bytes [0 bytes dynamic allocation]
 * - Reserved (uint8_t): 1 bytes [0 bytes dynamic allocation]
 * - EventTypeID (EventTypeId): sizeof(EventTypeId) [0 bytes dynamic allocation]
 * - SchemaVersion (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - MessageID (uint64_t): 8 bytes [0 bytes dynamic allocation]
 * - PayloadLength (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 22 bytes known members + sizeof(EventTypeId) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
struct EventTransportEnvelope {
    static constexpr uint32_t MagicValue = 0x45565454u; // EVTT
    static constexpr uint8_t CurrentVersion = 2;

    uint32_t Magic = MagicValue;
    uint8_t Version = CurrentVersion;
    uint8_t DispatchMethod = static_cast<uint8_t>(EventDispatchMethod::Queue);
    uint8_t Priority = static_cast<uint8_t>(EventPriority::Normal);
    uint8_t Reserved = 0;
    EventTypeId EventTypeID = 0;
    uint32_t SchemaVersion = 1;
    uint64_t MessageID = 0;
    uint32_t PayloadLength = 0;
};
#pragma pack(pop)

static_assert(
    sizeof(EventTransportEnvelope) == 32,
    "EventTransportEnvelope wire layout changed; increment protocol version deliberately."
);

/// <summary>Externally preferred byte storage used for complete serialized Event transport packets.</summary>
using EventTransportBuffer = System::Memory::ByteVector<
    System::Memory::MemoryPolicy::ExternalPreferred
>;

/// <summary>Shared immutable ownership of one complete serialized Event packet.</summary>
using EventTransportBufferPtr = std::shared_ptr<const EventTransportBuffer>;

/// <summary>Ownership-bearing serialized Event packet passed across transport execution boundaries.</summary>
/// <remarks>
/// The immutable backing buffer normally resides in PSRAM when available. Copying this packet copies only shared ownership;
/// the serialized bytes are not duplicated, allowing one serialization to fan out to multiple physical transports.
/// </remarks>
/**
 * ESPressio Memory Audit
 * Members:
 * - _buffer (EventTransportBufferPtr): sizeof(EventTransportBufferPtr) [0 bytes dynamic allocation]
 * Total Memory: sizeof(EventTransportBufferPtr) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class EventTransportPacket {
private:
    EventTransportBufferPtr _buffer;
    EventMessageId _messageID{};

public:
    EventTransportPacket() = default;

    explicit EventTransportPacket(
        EventTransportBuffer buffer,
        EventMessageId messageID = {}
    ) :
        _buffer(System::Memory::MakeShared<
            EventTransportBuffer,
            System::Memory::MemoryPolicy::ExternalPreferred
        >(std::move(buffer))),
        _messageID(messageID) {}

    EventTransportPacket(
        EventTransportBufferPtr buffer,
        EventMessageId messageID = {}
    ) : _buffer(std::move(buffer)), _messageID(messageID) {}

    const uint8_t* Data() const noexcept {
        return _buffer && !_buffer->empty() ? _buffer->data() : nullptr;
    }

    std::size_t Size() const noexcept {
        return _buffer ? _buffer->size() : 0;
    }

    EventMessageId MessageID() const noexcept { return _messageID; }
    const EventTransportBufferPtr& Buffer() const noexcept { return _buffer; }
    explicit operator bool() const noexcept { return Data() != nullptr && Size() != 0; }
};

/**
 * ESPressio Memory Audit
 * Underlying storage: 1 bytes
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
enum
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 1 bytes [0 bytes dynamic allocation]
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class EventTransportRegistrationResult : uint8_t {
    Registered,
    Updated,
    AlreadyRegistered,
    TypeConflict,
    InvalidTransport
};

/**
 * ESPressio Memory Audit
 * Underlying storage: 1 bytes
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
enum
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 1 bytes [0 bytes dynamic allocation]
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class EventTransportUnregistrationResult : uint8_t {
    Updated,
    Removed,
    NotRegistered,
    InvalidTransport
};

/**
 * ESPressio Memory Audit
 * Members:
 * - Requested (std::size_t): 4 bytes [0 bytes dynamic allocation]
 * - Changed (std::size_t): 4 bytes [0 bytes dynamic allocation]
 * - Unchanged (std::size_t): 4 bytes [0 bytes dynamic allocation]
 * - Failed (std::size_t): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 16 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
struct EventTransportBulkOperationResult {
    std::size_t Requested = 0;
    std::size_t Changed = 0;
    std::size_t Unchanged = 0;
    std::size_t Failed = 0;
};

class IEvent;
class IEventTransport;

/**
 * ESPressio Memory Audit
 * Underlying storage: 1 bytes
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
enum
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 1 bytes [0 bytes dynamic allocation]
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class EventTransportTransactionStage : uint8_t {
    OutboundAccepted,
    OutboundSerialized,
    OutboundHandedToTransport,
    InboundAccepted,
    InboundRejected,
    InboundDeserialized,
    InboundDispatched,
    Failed
};

/// <summary>Diagnostic snapshot of one Event transport-manager stage.</summary>
/// <remarks>Origin is dispatch provenance; route/hop state is deliberately absent because Event transport does not forward a received Event onward.</remarks>
/**
 * ESPressio Memory Audit
 * Members:
 * - Stage (EventTransportTransactionStage): 1 bytes [0 bytes dynamic allocation]
 * - Direction (EventTransportDirection): 1 bytes [0 bytes dynamic allocation]
 * - EventTypeID (EventTypeId): sizeof(EventTypeId) [0 bytes dynamic allocation]
 * - SchemaVersion (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - Transport (IEventTransport*): 4 bytes [0 bytes dynamic allocation]
 * - Event (IEvent*): 4 bytes [0 bytes dynamic allocation]
 * - Payload (uint8_t*): 4 bytes [0 bytes dynamic allocation]
 * - PayloadSize (std::size_t): 4 bytes [0 bytes dynamic allocation]
 * - DispatchMethod (EventDispatchMethod): 4 bytes [0 bytes dynamic allocation]
 * - Priority (EventPriority): 4 bytes [0 bytes dynamic allocation]
 * - Origin (EventOrigin): 1 bytes [0 bytes dynamic allocation]
 * - TransportAccepted (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 32 bytes known members + sizeof(EventTypeId) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
struct EventTransportTransaction {
    EventTransportTransactionStage Stage = EventTransportTransactionStage::Failed;
    EventTransportDirection Direction = EventTransportDirection::None;
    EventTypeId EventTypeID = 0;
    std::string_view EventTypeName{};
    uint32_t SchemaVersion = 0;
    EventMessageId MessageID{};
    IEventTransport* Transport = nullptr;
    const IEvent* Event = nullptr;
    const uint8_t* Payload = nullptr;
    std::size_t PayloadSize = 0;
    EventDispatchMethod DispatchMethod = EventDispatchMethod::Queue;
    EventPriority Priority = EventPriority::Normal;
    EventOrigin Origin = EventOrigin::Local;
    bool TransportAccepted = false;
};

/**
 * ESPressio Memory Audit
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 0 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
template<typename TEvent>
struct EventTransportTypeTraits {
    static constexpr EventTypeId Id = 0;
    static constexpr std::string_view Name{};
};

template<typename TEvent>
constexpr EventTypeId EventTransportTypeID() noexcept {
    return EventTransportTypeTraits<TEvent>::Id;
}

}

#define ESPRESSIO_EVENT_TRANSPORT_TYPE(Type, StableId, DiagnosticName) \
    namespace ESPressio::Event { \
        template<> struct EventTransportTypeTraits<Type> { \
            static constexpr EventTypeId Id = StableId; \
            static constexpr std::string_view Name = DiagnosticName; \
        }; \
    }
