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
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members: none; polymorphic/virtual-base object metadata is included in the total.
 * Total Memory: 24 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
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
