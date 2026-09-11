#pragma once
#include <ESPressio_RuntimeIdentity.hpp>
#include <ESPressio_PrimitivePolicy.hpp>
#include "ESPressio_SerializableEvent.hpp"
namespace ESPressio::Event {
/// Transmissible tier adds immutable source identity. P3 serializes only the
/// declared payload; the family codec carries origin exactly once in its header.
template<class TDerived> class TransmissibleEvent : public SerializableEvent<TDerived> {
    System::DeviceRuntimeIdentity _origin{};
    template<class T> friend class EventTypeRuntime;
public:
    static constexpr bool IsTransmissibleEvent=true;
    static constexpr bool ValidateTier() noexcept {
        static_assert(Serializable::IsBoundedSerializable<TDerived>, "Transmissible Event requires a bounded P3 schema");
        static_assert(Primitive::IsOccurrenceDeliveryPolicy<typename TDerived::DeliveryPolicy>::value,
                      "Transmissible Event requires a valid DeliveryPolicy");
        return true;
    }
    const System::DeviceRuntimeIdentity& GetOriginDeviceRuntimeIdentity() const noexcept { return _origin; }
    bool IsOriginLocal() const noexcept {
        const auto* local=System::RuntimeIdentity::TryGet();
        return local && _origin.Device==local->Device;
    }
    bool IsOriginRemote() const noexcept { return !IsOriginLocal(); }
};
}
