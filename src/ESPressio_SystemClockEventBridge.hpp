#pragma once
#include <atomic>
#include <utility>
#include <ESPressio_ISystemClockObserver.hpp>
#include <ESPressio_SystemClock.hpp>
#include "ESPressio_EventRuntime.hpp"
#include "ESPressio_TimingEvents.hpp"
namespace ESPressio::Event {
/// Caller-owned optional bridge. Initialize only after selected diagnostic Types
/// are registered/started. Every notification uses TryDispatch and may be missed
/// under bounded pressure; it never waits for Event consumers or services time.
class SystemClockEventBridge final : public Timing::ISystemClockObserver<> {
    using Clock=Timing::SystemClock<>;
    decltype(std::declval<Clock&>().RegisterObserver(static_cast<Timing::ISystemClockObserver<>*>(nullptr))) _handle;
    std::atomic<std::uint64_t> _unavailable{0};
    template<class T,class... Args> void Emit(Args&&... args) noexcept {
        try { if(!T::TryDispatch(std::forward<Args>(args)...)) ++_unavailable; }
        catch(...) { ++_unavailable; }
    }
public:
    SystemClockEventBridge() noexcept = default;
    SystemClockEventBridge(const SystemClockEventBridge&)=delete;
    SystemClockEventBridge& operator=(const SystemClockEventBridge&)=delete;
    ~SystemClockEventBridge() override { Shutdown(); }
    bool Initialize(Clock& clock=Clock::GetInstance()) {
        if(_handle) return true;
        _handle=clock.RegisterObserver(this);return bool(_handle);
    }
    void Shutdown() noexcept { _handle.reset(); }
    bool IsInitialized() const noexcept { return bool(_handle); }
    std::uint64_t UnavailableOccurrences() const noexcept { return _unavailable.load(); }
    void OnSystemClockTimeSet(Timing::ClockTick before,Timing::ClockTick after,std::int64_t difference) override {
        Emit<SystemClockTimeChangedEvent>(before,after,difference);
    }
    void OnSystemClockSynchronizationSampleAccepted(Timing::ClockTick,Timing::ClockTick,std::int64_t,
        const Timing::ClockSynchronizationResult& result,const Timing::ClockSynchronizationStatus& status) override {
        Emit<SynchronizationSampleAcceptedEvent>(ClockObservationDiagnostic{result},ClockStatusDiagnostic{status});
    }
    void OnSystemClockSynchronizationSampleRejected(const Timing::ClockSynchronizationResult& result,
        const Timing::ClockSynchronizationStatus& status) override {
        Emit<SynchronizationSampleRejectedEvent>(ClockObservationDiagnostic{result},ClockStatusDiagnostic{status});
    }
    void OnSystemClockSynchronizationStateChanged(Timing::TimeReliability before,Timing::TimeReliability after,
        const Timing::ClockSynchronizationStatus& status) override {
        Emit<SynchronizationStateChangedEvent>(static_cast<std::uint8_t>(before),static_cast<std::uint8_t>(after),ClockStatusDiagnostic{status});
    }
    void OnSystemClockSynchronizationReset(const Timing::ClockSynchronizationStatus& before,
        const Timing::ClockSynchronizationStatus& after) override {
        Emit<SynchronizationResetEvent>(ClockStatusDiagnostic{before},ClockStatusDiagnostic{after});
    }
    void OnSystemClockSynchronizationConfigurationChanged(const Timing::ClockSynchronizationProfile& before,
        const Timing::ClockSynchronizationProfile& after) override {
        Emit<SynchronizationConfigurationChangedEvent>(ClockProfileDiagnostic{before},ClockProfileDiagnostic{after});
    }
    void OnSystemClockCallbackScheduled(Timing::ClockTick scheduled) override { Emit<SystemClockCallbackScheduledEvent>(scheduled); }
    void OnSystemClockCallbackScheduleFailed(Timing::ClockTick scheduled) override { Emit<SystemClockCallbackScheduleFailedEvent>(scheduled); }
    void OnSystemClockCallbackExecuted(Timing::ClockTick scheduled,Timing::ClockTick actual,std::int64_t difference) override {
        Emit<SystemClockCallbackExecutedEvent>(scheduled,actual,difference);
    }
    void OnSystemClockCallbackExecutionFailed(Timing::ClockTick scheduled,Timing::ClockTick actual,std::int64_t difference,std::exception_ptr cause) override {
        Emit<SystemClockCallbackExecutionFailedEvent>(scheduled,actual,difference,bool(cause));
    }
    void OnSystemClockCallbacksCleared(std::size_t count) override { Emit<SystemClockCallbacksClearedEvent>(static_cast<std::uint64_t>(count)); }
};
}
