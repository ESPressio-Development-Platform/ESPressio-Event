#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <ESPressio_PrimitiveTypeId.hpp>
#include <ESPressio_PrimitiveTypes.hpp>
#include <ESPressio_DeviceRuntimeIdentity.hpp>
#include <ESPressio_TimeReliability.hpp>

namespace ESPressio::Event {
/// Stable family-qualified semantic Type identity; never a process-local address.
using EventTypeId = Primitive::EventTypeId;
using ConceptualMessageId = Primitive::ConceptualMessageId;
inline constexpr Primitive::PrimitiveFamilyId EventFamilyId = Primitive::FamilyIds::Event;
inline constexpr Primitive::PrimitiveProtocolVersion EventProtocolVersion = 1;
inline constexpr std::size_t EventWireHeaderSize = 53;

/// Source admission is complete when the single Type lane owns the occurrence.
enum class EventDispatchStatus : std::uint8_t {
    Accepted, NotInitialized, Stopping, CapacityUnavailable,
    IdentityUnavailable, IdentifierExhausted
};
struct EventDispatchResult final {
    EventDispatchStatus Status = EventDispatchStatus::NotInitialized;
    ConceptualMessageId MessageId{};
    constexpr explicit operator bool() const noexcept { return Status == EventDispatchStatus::Accepted; }
};
enum class EventRuntimeStatus : std::uint8_t {
    Success, AlreadyInitialized, NotInitialized, InvalidConfiguration,
    InvalidDirectory, TypeConflict, InvalidTopology, Frozen, StorageUnavailable,
    TaskCreationFailed, Stopping, JoinFailed
};
/// These facts are published once, before any consumer can acquire the occurrence.
/// Local/Serializable occurrences carry no redundant origin or route metadata.
struct EventOccurrenceFacts final {
    EventTypeId TypeId{};
    ConceptualMessageId MessageId{};
    Timing::QualifiedTime OriginDispatchTime{};
};
/// Distributed receipt identity excludes transport routes and previous-hop identity.
struct EventOccurrenceKey final {
    EventTypeId TypeId{};
    System::DeviceRuntimeIdentity Origin{};
    ConceptualMessageId MessageId{};
    constexpr bool IsValid() const noexcept { return bool(TypeId) && bool(Origin) && bool(MessageId); }
    constexpr bool operator==(const EventOccurrenceKey& b) const noexcept {
        return TypeId == b.TypeId && Origin == b.Origin && MessageId == b.MessageId;
    }
};
namespace Detail {
template<class T> constexpr bool ValidateEventType() noexcept {
    static_assert(std::is_same_v<std::remove_cv_t<decltype(T::TypeId)>, EventTypeId>,
                  "Event TypeId must be a strong EventTypeId");
    static_assert(bool(T::TypeId), "Event TypeId must be nonzero");
    static_assert(T::MaximumLiveInstances > 0, "Event MaximumLiveInstances must be positive");
    static_assert(T::MaximumPendingInstances >= 0, "Event pending capacity cannot be negative");
    static_assert(std::is_nothrow_destructible_v<T>, "Event destruction must not throw");
    return true;
}
}
}
