#pragma once
#include <ESPressio_PrimitiveTypeDescriptor.hpp>
#include <ESPressio_ContractFingerprintBuilder.hpp>
#include "ESPressio_EventTypeRuntime.hpp"
namespace ESPressio::Event {
enum class EventTier : std::uint8_t { Local, Serializable, Transmissible };
/// Static family metadata and bootstrap thunks. P1 stores this only as an opaque
/// immutable reference; it never invokes the thunks or stores runtime occupancy.
struct EventTypeDescriptor final {
    EventTypeId TypeId{};
    EventTier Tier=EventTier::Local;
    std::size_t MaximumLiveInstances=0,MaximumPendingInstances=0;
    EventRuntimeStatus (*Initialize)(void*,Task::TaskExecutionConfiguration,const std::atomic<bool>*)=nullptr;
    bool (*ValidateStart)() noexcept=nullptr;
    void (*StartValidated)() noexcept=nullptr;
    void (*CloseAdmissions)() noexcept=nullptr;
    EventRuntimeStatus (*Shutdown)() noexcept=nullptr;
    EventRuntimeStatus (*RollbackInitialization)() noexcept=nullptr;
    std::array<std::size_t,3> MaximumCompleteWireBytes{};
    const Serializable::StaticSchemaDescriptor* Schema=nullptr;
    const Primitive::PrimitivePolicyDescriptor* DeliveryPolicy=nullptr;
    EventTypeResourceProfile Resources{};
    EventRemoteAdmissionResult (*AdmitRemote)(EventPayloadFormat,const EventWireHeader&,const std::uint8_t*,std::size_t) noexcept=nullptr;
};
inline const EventTypeDescriptor* GetEventTypeDescriptor(const Primitive::PrimitiveTypeDescriptor& common) noexcept {
    if(common.Key.Family!=EventFamilyId || !common.FamilyExtension.Data) return nullptr;
    const auto* extension=static_cast<const EventTypeDescriptor*>(common.FamilyExtension.Data);
    return extension->TypeId.Value()==common.Key.TypeValue ? extension : nullptr;
}
template<class T> struct EventDescriptorProvider final {
    static Primitive::PrimitiveTypeDescriptor Describe() noexcept {
        static_assert(Detail::ValidateEventType<T>());
        static const EventTypeDescriptor extension=[] {
          EventTypeDescriptor value{
            T::TypeId,T::IsTransmissibleEvent ? EventTier::Transmissible :
                (T::IsSerializableEvent ? EventTier::Serializable : EventTier::Local),
            T::MaximumLiveInstances,T::MaximumPendingInstances,
            [](void* owner,Task::TaskExecutionConfiguration config,const std::atomic<bool>* gate) {
                return EventTypeRuntime<T>::Get().Initialize(owner,config,nullptr,gate);
            },
            []() noexcept { return EventTypeRuntime<T>::Get().ValidateStart(); },
            []() noexcept { EventTypeRuntime<T>::Get().StartValidated(); },
            []() noexcept { EventTypeRuntime<T>::Get().CloseAdmissions(); },
            []() noexcept { return EventTypeRuntime<T>::Get().Shutdown(); },
            []() noexcept { return EventTypeRuntime<T>::Get().RollbackInitialization(); }
          };
          value.Resources=EventTypeRuntime<T>::StaticResources();
          if constexpr(T::IsSerializableEvent) {
            static_assert(Serializable::IsBoundedSerializable<T>, "P1 Serializable Event metadata requires bounded schema");
            value.Schema=&Serializable::SchemaDescriptor<T>();
            constexpr std::size_t prefix=T::IsTransmissibleEvent ? EventWireHeaderSize : 0;
            value.MaximumCompleteWireBytes={prefix+Serializable::MaximumSerializedSize<T,Serializable::DirectBinary>,
                prefix+Serializable::MaximumSerializedSize<T,Serializable::CBOR>,
                prefix+Serializable::MaximumSerializedSize<T,Serializable::JSON>};
          }
          if constexpr(T::IsTransmissibleEvent) {
            static_assert(T::ValidateTier());
            static const auto policy=Primitive::PrimitivePolicyContract<typename T::DeliveryPolicy>::Descriptor();
            value.DeliveryPolicy=&policy;
            static_assert(MaximumCompletePrimitiveWireBytes<T,Serializable::DirectBinary>>EventWireHeaderSize);
            static_assert(MaximumCompletePrimitiveWireBytes<T,Serializable::CBOR>>EventWireHeaderSize);
            static_assert(MaximumCompletePrimitiveWireBytes<T,Serializable::JSON>>EventWireHeaderSize);
            value.AdmitRemote=[](EventPayloadFormat format,const EventWireHeader& header,const std::uint8_t* payload,std::size_t size) noexcept {
                switch(format) {
                    case EventPayloadFormat::DirectBinary: return EventTypeRuntime<T>::Get().template TryAdmitRemote<Serializable::DirectBinary>(header,payload,size);
                    case EventPayloadFormat::CBOR: return EventTypeRuntime<T>::Get().template TryAdmitRemote<Serializable::CBOR>(header,payload,size);
                    case EventPayloadFormat::JSON: return EventTypeRuntime<T>::Get().template TryAdmitRemote<Serializable::JSON>(header,payload,size);
                }
                return EventRemoteAdmissionResult{EventRemoteAdmissionStatus::Invalid};
            };
          }
          return value;
        }();
        std::size_t maximum=0;
        for(auto size:extension.MaximumCompleteWireBytes) if(size>maximum) maximum=size;
        Primitive::ContractFingerprintBuilder fingerprint;
        fingerprint.Text("ESPressio.Event.Contract.v1");
        fingerprint.Integer(EventFamilyId);fingerprint.Integer(T::TypeId.Value());
        fingerprint.Byte(static_cast<std::uint8_t>(extension.Tier));fingerprint.Integer(EventProtocolVersion);
        if constexpr(T::IsSerializableEvent) Serializable::WriteCanonicalSchema<T>(fingerprint);
        if constexpr(T::IsTransmissibleEvent) {
            fingerprint.Text("Event.V1.LE.type64.message64.device128.incarnation32.time64.reliability8.length32");
            fingerprint.Integer(EventWireHeaderSize);
            fingerprint.Text("OccurrenceDeliveryPolicy.v1");
            for(auto byte:extension.DeliveryPolicy->CanonicalBytes()) fingerprint.Byte(byte);
        }
        // Capacities, diagnostic names, runtime identity, routes and selected
        // format are deployment/occurrence facts, never semantic hash inputs.
        return {{EventFamilyId,T::TypeId.Value()},T::CanonicalName,
            Primitive::PrimitiveTypeCapabilities{T::IsTransmissibleEvent ? std::uint8_t{3} : (T::IsSerializableEvent ? std::uint8_t{1} : std::uint8_t{0})},
            {1,1},fingerprint.Finish(),{maximum},{&extension}};
    }
};
}
