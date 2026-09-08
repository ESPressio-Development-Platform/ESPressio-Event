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
 * - TypeID (EventTypeId): 8 bytes [0 bytes dynamic allocation]
 * - TypeName (std::string): 24 bytes [Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * - SchemaVersion (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - DefaultDirection (EventTransportDirection): 1 bytes [0 bytes dynamic allocation]
 * - Properties (std::vector<Serializable::PropertySchemaInfo>): 12 bytes [Capacity * (64 bytes) element storage; N live elements each: Name: Capacity + 1 bytes when capacity exceeds 15-byte SSO; N live elements each: Aliases: Capacity * (24 bytes) element storage; N live elements each: Aliases: N live elements each: Capacity + 1 bytes when capacity exceeds 15-byte SSO; N live elements each: Type: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * - CanConstruct (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 56 bytes [TypeName: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Properties: Capacity * (64 bytes) element storage; Properties: N live elements each: Name: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Properties: N live elements each: Aliases: Capacity * (24 bytes) element storage; Properties: N live elements each: Aliases: N live elements each: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Properties: N live elements each: Type: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
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
 * - Deserialization (Serializable::DeserializationResult): 12 bytes [_issues: Capacity * (52 bytes) element storage; _issues: N live elements each: Path: Capacity + 1 bytes when capacity exceeds 15-byte SSO; _issues: N live elements each: Message: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * - TypeRegistered (bool): 1 bytes [0 bytes dynamic allocation]
 * - Constructible (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 20 bytes [Event: owned object: 4 bytes; Deserialization: _issues: Capacity * (52 bytes) element storage; Deserialization: _issues: N live elements each: Path: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Deserialization: _issues: N live elements each: Message: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
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
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
enum
class RuntimeEventDispatchResult : uint8_t {
    Dispatched,
    NullEvent,
    UnsupportedMethod
};

} // namespace ESPressio::Event
