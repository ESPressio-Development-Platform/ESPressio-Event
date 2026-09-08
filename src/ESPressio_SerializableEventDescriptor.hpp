#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <ESPressio_Serializable.hpp>

#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventTransportTypes.hpp"
#include "ESPressio_IEvent.hpp"

namespace ESPressio::Event {

/// <summary>Runtime schema and transport metadata for a registered serializable Event type.</summary>
/**
 * ESPressio Memory Audit
 * Members:
 * - TypeID (EventTypeId): sizeof(EventTypeId) [0 bytes dynamic allocation]
 * - TypeName (std::string): 24 bytes [Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * - SchemaVersion (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - DefaultDirection (EventTransportDirection): 1 bytes [0 bytes dynamic allocation]
 * - Properties (std::vector<Serializable::PropertySchemaInfo>): 12 bytes [Capacity * 64 bytes; N elements each may add: Name: Capacity + 1 bytes when capacity exceeds 15-byte SSO + Aliases: Capacity * 24 bytes + Aliases: N elements each may add: Capacity + 1 bytes when capacity exceeds 15-byte SSO + Type: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * - CanConstruct (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 42 bytes known members + sizeof(EventTypeId) [TypeName: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Properties: Capacity * 64 bytes; Properties: N elements each may add: Name: Capacity + 1 bytes when capacity exceeds 15-byte SSO + Aliases: Capacity * 24 bytes + Aliases: N elements each may add: Capacity + 1 bytes when capacity exceeds 15-byte SSO + Type: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
struct SerializableEventDescriptor {
    /// <summary>Stable wire type identifier.</summary>
    EventTypeId TypeID = 0;
    /// <summary>Stable wire type name.</summary>
    std::string TypeName;
    /// <summary>Current Serializable schema version.</summary>
    uint32_t SchemaVersion = 1;
    /// <summary>Default transport direction registered for the type.</summary>
    EventTransportDirection DefaultDirection = EventTransportDirection::None;
    /// <summary>Serializable property schema exposed for runtime inspection.</summary>
    std::vector<Serializable::PropertySchemaInfo> Properties;
    /// <summary>Indicates whether the registered type can be constructed from a serialization node at runtime.</summary>
    bool CanConstruct = false;
};

/// <summary>Detailed result from constructing a registered Event dynamically from serialized node data.</summary>
/**
 * ESPressio Memory Audit
 * Members:
 * - Event (std::unique_ptr<IEvent>): 4 bytes [owned object: 4 bytes]
 * - Deserialization (Serializable::DeserializationResult): 12 bytes [_issues: Capacity * 52 bytes; _issues: N elements each may add: Path: Capacity + 1 bytes when capacity exceeds 15-byte SSO + Message: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * - TypeRegistered (bool): 1 bytes [0 bytes dynamic allocation]
 * - Constructible (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 20 bytes [Event: owned object: 4 bytes; Deserialization: _issues: Capacity * 52 bytes; Deserialization: _issues: N elements each may add: Path: Capacity + 1 bytes when capacity exceeds 15-byte SSO + Message: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
struct SerializableEventConstructionResult {
    /// <summary>Constructed Event when deserialization succeeds.</summary>
    std::unique_ptr<IEvent> Event;
    /// <summary>Detailed Serializable deserialization result.</summary>
    Serializable::DeserializationResult Deserialization;
    /// <summary>Indicates whether the requested runtime Event type is registered.</summary>
    bool TypeRegistered = false;
    /// <summary>Indicates whether the registered type supports runtime construction.</summary>
    bool Constructible = false;

    /// <summary>Reports whether type lookup, construction, and deserialization all succeeded.</summary>
    bool Success() const noexcept {
        return TypeRegistered && Constructible && Event != nullptr && Deserialization.Success();
    }
    /// <summary>Converts the result to its overall success state.</summary>
    explicit operator bool() const noexcept { return Success(); }
};

/// <summary>Outcome from dispatching a dynamically constructed Event through the local Event manager.</summary>
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
class RuntimeEventDispatchResult : uint8_t {
    Dispatched,
    NullEvent,
    UnsupportedMethod
};

} // namespace ESPressio::Event
