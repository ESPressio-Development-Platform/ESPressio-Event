#pragma once
#include <array>
#include <atomic>
#include <limits>
#include <mutex>
#include <new>
#include <utility>
#include <ESPressio_Synchronization.hpp>
#include "ESPressio_EventLease.hpp"

namespace ESPressio::Event {
template<class T> class Event;
/// Fixed placement storage. Reservations count against capacity before T is
/// constructed. A failed constructor releases the reservation without publishing
/// an occurrence. Destruction finishes before the slot becomes reusable.
template<class T, std::size_t N> class EventInstancePool final {
    static_assert(N > 0, "Event pool requires finite positive capacity");
    static_assert(std::is_nothrow_destructible_v<T>, "Event destruction must not throw");
    struct Slot final {
        Detail::EventSlotControl Control{};
        alignas(T) std::byte Storage[sizeof(T)];
        bool Reserved = false;
    };
    std::array<Slot, N> _slots{};
    System::Synchronization::Mutex _mutex;
    std::atomic<std::size_t> _occupied{0};
    void* _wakeOwner = nullptr;
    void (*_capacityChanged)(void*) noexcept = nullptr;
    void Return(std::size_t index) noexcept {
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            auto& slot = _slots[index];
            slot.Control.Payload = nullptr;
            slot.Reserved = false;
            --_occupied;
        }
        if (_capacityChanged) _capacityChanged(_wakeOwner);
    }
    static void Destroy(Detail::EventSlotControl* control) noexcept {
        auto& pool = *static_cast<EventInstancePool*>(control->Pool);
        auto& slot = pool._slots[control->Index];
        std::launder(reinterpret_cast<T*>(slot.Storage))->~T();
        pool.Return(control->Index);
    }
public:
    class Reservation final {
        EventInstancePool* _pool = nullptr;
        std::size_t _index = 0;
        friend class EventInstancePool;
        Reservation(EventInstancePool* pool, std::size_t index) noexcept : _pool(pool), _index(index) {}
    public:
        Reservation() noexcept = default;
        Reservation(const Reservation&) = delete;
        Reservation& operator=(const Reservation&) = delete;
        Reservation(Reservation&& b) noexcept : _pool(std::exchange(b._pool, nullptr)), _index(b._index) {}
        Reservation& operator=(Reservation&&) = delete;
        ~Reservation() { if (_pool) _pool->Return(_index); }
        explicit operator bool() const noexcept { return _pool != nullptr; }
        /// Publishes the root reference only after successful placement construction.
        template<class... Args> EventLease Construct(const EventOccurrenceFacts& facts, Args&&... args) {
            if (!_pool) std::terminate();
            auto& slot = _pool->_slots[_index];
            auto* value = new (slot.Storage) T(std::forward<Args>(args)...);
            slot.Control.Facts = facts;
            slot.Control.Payload = value;
            if constexpr (std::is_base_of_v<Event<T>, T>)
                static_cast<Event<T>&>(*value)._facts = &slot.Control.Facts;
            slot.Control.References.store(1, std::memory_order_release);
            _pool = nullptr;
            return EventLease(&slot.Control);
        }
    };
    EventInstancePool() noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            auto& c = _slots[i].Control;
            c.Pool = this; c.Index = i; c.DestroyAndRelease = &Destroy;
        }
    }
    EventInstancePool(const EventInstancePool&) = delete;
    EventInstancePool& operator=(const EventInstancePool&) = delete;
    ~EventInstancePool() { if (_occupied != 0) std::terminate(); }
    /// Bootstrap only; the owner and callback outlive every reservation/reference.
    void BindCapacityWake(void* owner, void (*wake)(void*) noexcept) noexcept {
        // Resolve platform synchronization at bootstrap, never at first Dispatch.
        std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
        _wakeOwner = owner; _capacityChanged = wake;
    }
    /// Non-blocking admission, no spill or allocation. Exhausted slot generations
    /// remain permanently unavailable instead of becoming stale-handle aliases.
    Reservation TryReserve() noexcept {
        std::unique_lock<System::Synchronization::Mutex> lock(_mutex, std::try_to_lock);
        if (!lock.owns_lock()) return {};
        for (std::size_t i = 0; i < N; ++i) {
            auto& slot = _slots[i];
            if (slot.Reserved || slot.Control.Generation == std::numeric_limits<std::uint64_t>::max()) continue;
            slot.Reserved = true; ++slot.Control.Generation; ++_occupied;
            return Reservation(this, i);
        }
        return {};
    }
    std::size_t Occupied() const noexcept { return _occupied.load(std::memory_order_acquire); }
    static constexpr std::size_t Capacity = N;
    static constexpr std::size_t SlotBytes = sizeof(Slot);
    static constexpr std::size_t StorageAlignment = alignof(Slot);
};
}
