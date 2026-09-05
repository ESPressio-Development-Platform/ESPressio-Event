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
enum class RuntimeEventDispatchResult : uint8_t {
    Dispatched,
    NullEvent,
    UnsupportedMethod
};

} // namespace ESPressio::Event
