#pragma once
#include <limits>
#include <ESPressio_BoundedDeserializer.hpp>
#include <ESPressio_BoundedCborArchive.hpp>
#include <ESPressio_BoundedJsonArchive.hpp>
#include "ESPressio_TransmissibleEvent.hpp"
namespace ESPressio::Event {
/// Format belongs to a frozen binding, never to per-occurrence facts or bytes.
enum class EventPayloadFormat : std::uint8_t { DirectBinary, CBOR, JSON };
enum class EventWireStatus : std::uint8_t {
    Success, InvalidHeader, UnsupportedProtocol, UnknownType, InvalidLength,
    PayloadTooLarge, SchemaOrDecodeFailure, InsufficientOutput
};
struct EventWireResult final {
    EventWireStatus Status=EventWireStatus::InvalidHeader;
    std::size_t Bytes=0;
    explicit operator bool() const noexcept { return Status==EventWireStatus::Success; }
};
struct EventWireHeader final {
    EventOccurrenceKey Key{};
    Timing::QualifiedTime OriginDispatchTime{};
    std::uint32_t PayloadLength=0;
};
namespace Detail {
inline void WriteEventLE(std::uint8_t* out,std::uint64_t value,std::size_t bytes) noexcept {
    for(std::size_t i=0;i<bytes;++i) { out[i]=static_cast<std::uint8_t>(value); value>>=8; }
}
inline std::uint64_t ReadEventLE(const std::uint8_t* data,std::size_t bytes) noexcept {
    std::uint64_t value=0; for(std::size_t i=0;i<bytes;++i) value|=std::uint64_t(data[i])<<(8*i); return value;
}
template<class T,class Format> constexpr std::size_t EventMaximum() noexcept {
    static_assert(T::IsTransmissibleEvent,"Event family wire requires the Transmissible tier");
    static_assert(Serializable::IsBoundedSerializable<T>,"Transmissible Event requires bounded P3 schema");
    static_assert(Primitive::IsOccurrenceDeliveryPolicy<typename T::DeliveryPolicy>::value,"Transmissible Event requires a valid DeliveryPolicy");
    constexpr auto payload=Serializable::MaximumSerializedSize<T,Format>;
    static_assert(payload<=UINT32_MAX && payload<=SIZE_MAX-EventWireHeaderSize,"Event payload length exceeds V1 bounds");
    return EventWireHeaderSize+payload;
}
}
template<class T,class Format> inline constexpr std::size_t MaximumCompletePrimitiveWireBytes=Detail::EventMaximum<T,Format>();
/// Exact V1 offsets, independent of native alignment or compiler struct layout.
inline EventWireResult EncodeEventWireHeader(const EventWireHeader& header,std::uint8_t* output,std::size_t capacity) noexcept {
    if(!header.Key.IsValid() || !Timing::IsValidTimeReliability(header.OriginDispatchTime.Reliability)) return {};
    if(!output || capacity<EventWireHeaderSize) return {EventWireStatus::InsufficientOutput,0};
    Detail::WriteEventLE(output,EventFamilyId,2); Detail::WriteEventLE(output+2,EventProtocolVersion,2);
    Detail::WriteEventLE(output+4,header.Key.TypeId.Value(),8); Detail::WriteEventLE(output+12,header.Key.MessageId.Value(),8);
    for(std::size_t i=0;i<16;++i) output[20+i]=header.Key.Origin.Device.Bytes()[i];
    Detail::WriteEventLE(output+36,header.Key.Origin.Incarnation.Value(),4);
    Detail::WriteEventLE(output+40,header.OriginDispatchTime.Nanoseconds,8);
    output[48]=static_cast<std::uint8_t>(header.OriginDispatchTime.Reliability);
    Detail::WriteEventLE(output+49,header.PayloadLength,4);
    return {EventWireStatus::Success,EventWireHeaderSize};
}
/// Validates complete-frame length and immutable identity before touching any
/// receipt or Type pool. Failure leaves the caller's header unchanged.
inline EventWireResult DecodeEventWireHeader(const std::uint8_t* data,std::size_t size,EventWireHeader& output) noexcept {
    if(!data || size<EventWireHeaderSize || Detail::ReadEventLE(data,2)!=EventFamilyId) return {};
    if(Detail::ReadEventLE(data+2,2)!=EventProtocolVersion) return {EventWireStatus::UnsupportedProtocol,0};
    EventWireHeader header;
    header.Key.TypeId=EventTypeId{Detail::ReadEventLE(data+4,8)};
    header.Key.MessageId=ConceptualMessageId{Detail::ReadEventLE(data+12,8)};
    System::DeviceIdentifier::Storage device{};
    for(std::size_t i=0;i<16;++i) device[i]=data[20+i];
    header.Key.Origin={System::DeviceIdentifier{device},System::RuntimeIncarnationId{static_cast<std::uint32_t>(Detail::ReadEventLE(data+36,4))}};
    header.OriginDispatchTime={Detail::ReadEventLE(data+40,8),static_cast<Timing::TimeReliability>(data[48])};
    header.PayloadLength=static_cast<std::uint32_t>(Detail::ReadEventLE(data+49,4));
    if(!header.Key.IsValid() || !Timing::IsValidTimeReliability(header.OriginDispatchTime.Reliability)) return {};
    if(size-EventWireHeaderSize!=header.PayloadLength) return {EventWireStatus::InvalidLength,0};
    output=header; return {EventWireStatus::Success,size};
}
template<class T,class Format> EventWireResult EncodeEventWire(const T& event,std::uint8_t* output,std::size_t capacity) {
    static_assert(MaximumCompletePrimitiveWireBytes<T,Format>>EventWireHeaderSize);
    if(!event.TryGetOccurrenceFacts() || !event.GetOriginDeviceRuntimeIdentity()) return {};
    if(!output || capacity<EventWireHeaderSize) return {EventWireStatus::InsufficientOutput,0};
    Serializable::BoundedSerializationResult payload;
    if constexpr(std::is_same_v<Format,Serializable::DirectBinary>)
        payload=Serializable::SerializeDirectBinary(event,output+EventWireHeaderSize,capacity-EventWireHeaderSize);
    else if constexpr(std::is_same_v<Format,Serializable::CBOR>)
        payload=Serializable::SerializeBoundedCbor(event,output+EventWireHeaderSize,capacity-EventWireHeaderSize);
    else {
        static_assert(std::is_same_v<Format,Serializable::JSON>,"Unsupported Event binding format");
        payload=Serializable::SerializeBoundedJson(event,output+EventWireHeaderSize,capacity-EventWireHeaderSize);
    }
    if(!payload) return {EventWireStatus::SchemaOrDecodeFailure,0};
    const auto result=EncodeEventWireHeader({{T::TypeId,event.GetOriginDeviceRuntimeIdentity(),event.GetConceptualMessageId()},
        event.GetOriginDispatchTime(),static_cast<std::uint32_t>(payload.Bytes)},output,capacity);
    if(!result) return result;
    return {EventWireStatus::Success,EventWireHeaderSize+payload.Bytes};
}
}
