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

template<typename TTime = Timing::DefaultClockTime>
class Event : public IEvent {
private:
    struct DispatchState {
        bool WasDispatched = false;
        uint64_t DispatchTimeNanoseconds = 0;
    };

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
    mutable std::atomic_flag _dispatchContextGuard = ATOMIC_FLAG_INIT;
    EventDispatchContext _dispatchContext{};

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

    /// Legacy untyped Event base. Concrete routable events should inherit from
    /// TypedEvent<TDerived,TTime>; RTTI-enabled builds retain migration fallback.
    EventTypeKey __getTypeKey() const noexcept override { return nullptr; }

    void __ref() noexcept override {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

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

    void __setDispatchContext(const EventDispatchContext& context) override {
        AtomicFlagGuard lock(_dispatchContextGuard);
        _dispatchContext = context;
    }

    EventDispatchContext __getDispatchContext() const override {
        AtomicFlagGuard lock(_dispatchContextGuard);
        return _dispatchContext;
    }

    void __dispatch() override {
        const uint64_t now = GetNowNanoseconds();
        AtomicFlagGuard lock(_dispatchStateGuard);
        if (!_dispatchState.WasDispatched) {
            _dispatchState.WasDispatched = true;
            _dispatchState.DispatchTimeNanoseconds = now;
        }
    }

    void Queue(EventPriority priority = EventPriority::Normal) override {
        EventManager::GetInstance()->QueueEvent(this, priority);
    }

    void Stack(EventPriority priority = EventPriority::Normal) override {
        EventManager::GetInstance()->StackEvent(this, priority);
    }

    uint64_t GetDispatchTimeNanoseconds() const override {
        const DispatchState state = GetDispatchState();
        return state.WasDispatched ? state.DispatchTimeNanoseconds : 0;
    }

    uint64_t GetTimeSinceDispatchNanoseconds() const override {
        const DispatchState state = GetDispatchState();
        if (!state.WasDispatched) return 0;
        const uint64_t now = GetNowNanoseconds();
        return now >= state.DispatchTimeNanoseconds
            ? now - state.DispatchTimeNanoseconds
            : 0;
    }

    TTime GetDispatchTime() const {
        return CreateTime(GetDispatchTimeNanoseconds());
    }

    TTime GetTimeSinceDispatch() const {
        return CreateTime(GetTimeSinceDispatchNanoseconds());
    }
};

/// CRTP Event base that supplies concrete type identity without RTTI.
template<
    typename TDerived,
    typename TTime = Timing::DefaultClockTime
>
class TypedEvent : public Event<TTime> {
public:
    using TimeType = TTime;
    using EventBase = Event<TTime>;

    EventTypeKey __getTypeKey() const noexcept override {
        return EventTypeKeyOf<TDerived>();
    }

    virtual ~TypedEvent() = default;
};

} // namespace Event
} // namespace ESPressio
