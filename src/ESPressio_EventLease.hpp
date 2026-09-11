#pragma once
#include <atomic>
#include <cstdint>
#include <exception>
#include <limits>
#include <utility>
#include "ESPressio_EventTypes.hpp"

namespace ESPressio::Event {
template<class T, std::size_t N> class EventInstancePool;
namespace Detail {
/// Pool-resident control block. An outstanding reference prevents slot reuse, so
/// a FIFO needs exactly one pointer, with no copied generation or callback data.
struct EventSlotControl final {
    std::atomic<std::size_t> References{0};
    EventOccurrenceFacts Facts{};
    const void* Payload = nullptr;
    void* Pool = nullptr;
    void (*DestroyAndRelease)(EventSlotControl*) noexcept = nullptr;
    std::uint64_t Generation = 0;
    std::size_t Index = 0;
    void Retain() noexcept {
        auto n = References.load(std::memory_order_relaxed);
        do {
            if (n == 0 || n == std::numeric_limits<std::size_t>::max()) std::terminate();
        } while (!References.compare_exchange_weak(n, n + 1, std::memory_order_relaxed));
    }
    void Release() noexcept {
        if (References.fetch_sub(1, std::memory_order_acq_rel) == 1) DestroyAndRelease(this);
    }
};
}
/// Move-only ownership of one pool-resident occurrence. No payload allocation or
/// copy occurs when a lane hands a retained reference to a consumer.
class EventLease final {
    Detail::EventSlotControl* _slot = nullptr;
    explicit EventLease(Detail::EventSlotControl* p) noexcept : _slot(p) {}
    template<class T, std::size_t N> friend class EventInstancePool;
public:
    EventLease() noexcept = default;
    EventLease(const EventLease&) = delete;
    EventLease& operator=(const EventLease&) = delete;
    EventLease(EventLease&& b) noexcept : _slot(std::exchange(b._slot, nullptr)) {}
    EventLease& operator=(EventLease&& b) noexcept {
        if (this != &b) { Reset(); _slot = std::exchange(b._slot, nullptr); }
        return *this;
    }
    ~EventLease() { Reset(); }
    void Reset() noexcept { auto* p = std::exchange(_slot, nullptr); if (p) p->Release(); }
    explicit operator bool() const noexcept { return _slot != nullptr; }
    /// Caller must already own a live reference. The returned lease adds one.
    EventLease Retain() const noexcept {
        if (_slot) _slot->Retain();
        return EventLease(_slot);
    }
    const EventOccurrenceFacts& Facts() const noexcept { return _slot->Facts; }
    template<class T> const T& Get() const noexcept {
        if (!_slot || _slot->Facts.TypeId != T::TypeId) std::terminate();
        return *static_cast<const T*>(_slot->Payload);
    }
    /// Internal one-pointer FIFO ownership transfer. Adopt consumes that exact
    /// live reference; it must never be used with a stale/non-owning pointer.
    Detail::EventSlotControl* ReleaseToInbox() noexcept { return std::exchange(_slot, nullptr); }
    static EventLease AdoptFromInbox(Detail::EventSlotControl* p) noexcept { return EventLease(p); }
};
static_assert(sizeof(EventLease) == sizeof(void*), "Event inbox ownership remains exactly one pointer");
}
