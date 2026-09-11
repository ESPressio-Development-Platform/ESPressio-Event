#pragma once
#include <utility>
#include <ESPressio_PrimitiveTypeDescriptor.hpp>
#include "ESPressio_EventTypes.hpp"
namespace ESPressio::Event {
template<class T> class EventTypeRuntime;
template<class T> struct EventDescriptorProvider;
template<class T, std::size_t N> class EventInstancePool;
/// Local Event tier. Define a concrete Type with a stable TypeId and explicit
/// live/pending capacities. Dispatch factories create occurrences in its pool.
/// Consumer callbacks receive const T&; facts are bound only by the pool.
template<class TDerived> class Event {
    const EventOccurrenceFacts* _facts = nullptr;
    template<class T, std::size_t N> friend class EventInstancePool;
protected:
    Event() noexcept = default;
    // Payload staging/copying never copies ownership or occurrence identity.
    Event(const Event&) noexcept {}
    Event& operator=(const Event&) noexcept { return *this; }
    ~Event() = default;
public:
    static Primitive::PrimitiveTypeDescriptor GetPrimitiveTypeDescriptor() noexcept {
        return EventDescriptorProvider<TDerived>::Describe();
    }
    static constexpr bool ValidateTier() noexcept { return true; }
    static constexpr bool IsSerializableEvent = false;
    static constexpr bool IsTransmissibleEvent = false;
    /// A detached payload is not a dispatched occurrence and has no facts.
    const EventOccurrenceFacts* TryGetOccurrenceFacts() const noexcept { return _facts; }
    ConceptualMessageId GetConceptualMessageId() const noexcept {
        return _facts ? _facts->MessageId : ConceptualMessageId{};
    }
    Timing::QualifiedTime GetOriginDispatchTime() const noexcept {
        return _facts ? _facts->OriginDispatchTime : Timing::QualifiedTime{};
    }
    template<class... Args> static EventDispatchResult Dispatch(Args&&... args) {
        return EventTypeRuntime<TDerived>::Get().template Dispatch<true>(std::forward<Args>(args)...);
    }
    template<class... Args> static EventDispatchResult TryDispatch(Args&&... args) {
        return EventTypeRuntime<TDerived>::Get().template Dispatch<false>(std::forward<Args>(args)...);
    }
};
}
