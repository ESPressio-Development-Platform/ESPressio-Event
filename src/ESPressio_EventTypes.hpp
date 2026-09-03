#pragma once

#include <cstdint>

#include <ESPressio_Primitive.hpp>

namespace ESPressio {
namespace Event {

/// <summary>Stable semantic identifier of an Event type within the Event family.</summary>
using EventTypeId = std::uint64_t;

/// <summary>Schema revision used to interpret one Event type's payload.</summary>
using EventSchemaVersion = std::uint32_t;

/// <summary>Conceptual identity of one Event occurrence.</summary>
using EventMessageId = Primitive::ConceptualMessageId;

/// <summary>Optional causal/workflow correlation shared with other conceptual messages.</summary>
using EventCorrelationId = Primitive::CorrelationId;

/// <summary>Primitive-family protocol revision used by Event family adapters.</summary>
using EventProtocolVersion = Primitive::PrimitiveProtocolVersion;

/// <summary>Provenance of an Event at the current local dispatch boundary.</summary>
enum class EventOrigin : std::uint8_t {
    Local,
    Remote
};

/// <summary>Transport-independent provenance accompanying one local Event dispatch.</summary>
/// <remarks>
/// This context belongs to the dispatch operation rather than to the Event object.
/// Transport-local message identifiers, routes and hop counts are deliberately absent.
/// </remarks>
struct EventDispatchContext final {
    EventOrigin Origin = EventOrigin::Local;

    constexpr bool operator==(const EventDispatchContext& other) const noexcept {
        return Origin == other.Origin;
    }

    constexpr bool operator!=(const EventDispatchContext& other) const noexcept {
        return !(*this == other);
    }
};

/// <summary>Intrinsic transport-independent metadata of one Event occurrence.</summary>
/// <remarks>
/// Transport origin, route, hop count and transport-local message identity are
/// deliberately absent. Those values belong to receive/dispatch context owned by
/// the integration layer rather than to the Event occurrence itself.
/// </remarks>
struct EventMetadata final {
    /// <summary>Stable semantic Event type identifier.</summary>
    EventTypeId TypeId = 0;

    /// <summary>Payload schema version for this Event type.</summary>
    EventSchemaVersion SchemaVersion = 1;

    /// <summary>Conceptual identity of this occurrence. Zero means Unspecified until assigned.</summary>
    EventMessageId MessageId{};

    /// <summary>Optional correlation with other independent conceptual messages.</summary>
    EventCorrelationId Correlation{};
};

} // namespace Event
} // namespace ESPressio
