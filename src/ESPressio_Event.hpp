#pragma once

#include <atomic>
#include <cstdint>

#include <ESPressio_SystemClock.hpp>
#include <ESPressio_TimeTraits.hpp>

#include "ESPressio_IEvent.hpp"
#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventObserver.hpp"
#include "ESPressio_EventManager.hpp"

namespace ESPressio {
namespace Event {

/// <summary>Default Event implementation providing intrusive lifetime, dispatch timing, and manager queueing.</summary>
/// <typeparam name="TTime">Public time representation returned by typed dispatch-time accessors.</typeparam>
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members:
 * - _dispatchStateGuard (std::atomic_flag): 1 bytes [0 bytes dynamic allocation]
 * - _dispatchState (DispatchState): 12 bytes [0 bytes dynamic allocation]
 * - _refCount (std::atomic<uint32_t>): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 24 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
template<typename TTime = Timing::DefaultClockTime>
class Event : public IEvent {
private:
/**
 * ESPressio Memory Audit
 * Members:
 * - WasDispatched (bool): 1 bytes [0 bytes dynamic allocation]
 * - DispatchTimeNanoseconds (uint64_t): 8 bytes [0 bytes dynamic allocation]
 * Total Memory: 12 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
struct DispatchState {
        bool WasDispatched = false;
        uint64_t DispatchTimeNanoseconds = 0;
    };

/**
 * ESPressio Memory Audit
 * Members:
 * - _flag (std::atomic_flag&): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class AtomicFlagGuard {
    private:
        std::atomic_flag& _flag;
    public:
        explicit AtomicFlagGuard(std::atomic_flag& flag) noexcept : _flag(flag) {
            while (_flag.test_and_set(std::memory_order_acquire)) {}
        }
        ~AtomicFlagGuard() { _flag.clear(std::memory_order_release); }
        AtomicFlagGuard(const AtomicFlagGuard&) = delete;
        AtomicFlagGuard& operator=(const AtomicFlagGuard&) = delete;
    };

    mutable std::atomic_flag _dispatchStateGuard = ATOMIC_FLAG_INIT;
    DispatchState _dispatchState{};
    std::atomic<uint32_t> _refCount{0};

    static uint64_t GetResolutionNanoseconds() {
        auto& clock = Timing::SystemClock<TTime>::GetInstance();
        uint64_t resolution = Timing::TimeTraits<TTime>::template ToNanoseconds<uint64_t>(
            clock.GetResolution()
        );
        return resolution == 0 ? 1 : resolution;
    }

    static uint64_t GetNowNanoseconds() {
        auto& clock = Timing::SystemClock<TTime>::GetInstance();
        return Timing::TimeTraits<TTime>::template ToNanoseconds<uint64_t>(
            clock.GetTime()
        );
    }

    static TTime CreateTime(uint64_t nanoseconds) {
        return Timing::TimeTraits<TTime>::template FromNanoseconds<uint64_t>(
            nanoseconds,
            GetResolutionNanoseconds()
        );
    }

    DispatchState GetDispatchState() const noexcept {
        AtomicFlagGuard lock(_dispatchStateGuard);
        return _dispatchState;
    }

public:
    using TimeType = TTime;
    virtual ~Event() = default;

    /// <inheritdoc/>
    void __ref() noexcept override {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    /// <inheritdoc/>
    void __unref() noexcept override {
        uint32_t current = _refCount.load(std::memory_order_acquire);
        while (current != 0) {
            if (_refCount.compare_exchange_weak(
                    current,
                    current - 1,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                if (current == 1) delete this;
                return;
            }
        }
    }

    /// <inheritdoc/>
    void __dispatch() override {
        const uint64_t now = GetNowNanoseconds();
        AtomicFlagGuard lock(_dispatchStateGuard);
        if (!_dispatchState.WasDispatched) {
            _dispatchState.WasDispatched = true;
            _dispatchState.DispatchTimeNanoseconds = now;
        }
    }

    /// <inheritdoc/>
    void Queue(EventPriority priority = EventPriority::Normal) override {
        EventManager::GetInstance()->QueueEvent(this, priority);
    }

    /// <inheritdoc/>
    void Stack(EventPriority priority = EventPriority::Normal) override {
        EventManager::GetInstance()->StackEvent(this, priority);
    }

    /// <inheritdoc/>
    uint64_t GetDispatchTimeNanoseconds() const override {
        const DispatchState state = GetDispatchState();
        return state.WasDispatched ? state.DispatchTimeNanoseconds : 0;
    }

    /// <inheritdoc/>
    uint64_t GetTimeSinceDispatchNanoseconds() const override {
        const DispatchState state = GetDispatchState();
        if (!state.WasDispatched) return 0;
        const uint64_t now = GetNowNanoseconds();
        return now >= state.DispatchTimeNanoseconds
            ? now - state.DispatchTimeNanoseconds
            : 0;
    }

    /// <summary>Returns the Event's dispatch timestamp in the configured public time representation.</summary>
    TTime GetDispatchTime() const {
        return CreateTime(GetDispatchTimeNanoseconds());
    }

    /// <summary>Returns elapsed time since dispatch in the configured public time representation.</summary>
    TTime GetTimeSinceDispatch() const {
        return CreateTime(GetTimeSinceDispatchNanoseconds());
    }
};

/// <summary>CRTP Event base that supplies a stable RTTI-free local type identity for a concrete Event type.</summary>
/// <typeparam name="TDerived">Concrete Event type whose identity is exposed.</typeparam>
/// <typeparam name="TTime">Public time representation used by the Event.</typeparam>
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
    typename TTime = Timing::DefaultClockTime
>
class TypedEvent : public Event<TTime> {
public:
    using TimeType = TTime;
    using EventBase = Event<TTime>;

    /// <summary>Returns the stable compiler-backed local type key for <typeparamref name="TDerived"/>.</summary>
    EventTypeKey __getTypeKey() const noexcept override {
        return EventTypeKeyOf<TDerived>();
    }

    virtual ~TypedEvent() = default;
};

} // namespace Event
} // namespace ESPressio
