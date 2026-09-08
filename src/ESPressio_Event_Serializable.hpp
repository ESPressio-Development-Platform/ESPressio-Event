#pragma once

#if !__has_include(<ESPressio_Serializable.hpp>)
    #error "ESPressio_Event_Serializable.hpp requires ESPressio-Serializable. Add https://github.com/ESPressio-Development-Platform/ESPressio-Serializable.git#structural_realignment to the consuming project."
#endif

#include <ESPressio_Serializable.hpp>
#include <ESPressio_Time_Serializable.hpp>

#include "ESPressio_Event.hpp"

namespace ESPressio {
namespace Event {

/// Serializable Event base. TDerived supplies both the serialization schema and
/// the compiler-backed Event routing identity, so SerializableEvent requires no RTTI.
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members: none; polymorphic interface/object includes vptr storage where not supplied by a base.
 * Total Memory: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
template<
    typename TDerived,
    typename TTime = Units::SerializableNanoSeconds<uint64_t>
>
class SerializableEvent :
    public TypedEvent<TDerived, TTime>,
    public Serializable::SerializableBase<TDerived> {
public:
    using TimeType = TTime;
    using EventBase = TypedEvent<TDerived, TTime>;
    virtual ~SerializableEvent() = default;
};

} // namespace Event
} // namespace ESPressio
