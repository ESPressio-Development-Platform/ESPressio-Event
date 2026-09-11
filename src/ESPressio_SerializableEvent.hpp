#pragma once
#include <ESPressio_SerializableBase.hpp>
#include <ESPressio_SerializationMacros.hpp>
#include <ESPressio_SerializationTraits.hpp>
#include "ESPressio_EventBase.hpp"
namespace ESPressio::Event {
/// One Event Type with one payload schema, not a separately registered wrapper.
/// Wire/persistence use additionally instantiates the P3 bounded-schema gate.
template<class TDerived> class SerializableEvent : public Event<TDerived>, public Serializable::SerializableBase<TDerived> {
public:
    static constexpr bool IsSerializableEvent=true;
};
}
