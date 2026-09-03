#pragma once

#include <memory>
#include <mutex>

#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
    #include <atomic>
    #include <cstdint>
    #include <cstdio>
#endif

#include <ESPressio_Synchronization.hpp>
#include <ESPressio_Thread.hpp>

#include "ESPressio_EventReceiver.hpp"

#ifndef ESPRESSIO_EVENT_THREAD_DEFAULT_PRIORITY
    #define ESPRESSIO_EVENT_THREAD_DEFAULT_PRIORITY 2
#endif

#ifndef ESPRESSIO_EVENT_THREAD_DEFAULT_CORE_ID
    #define ESPRESSIO_EVENT_THREAD_DEFAULT_CORE_ID 0
#endif

using namespace ESPressio::Threads;

namespace ESPressio {
namespace Event {

class IEventThreadBase {};

/// <summary>Thread/receiver base that waits for queued Events and forwards them with dispatch provenance to a derived handler.</summary>
class EventThreadBase : public Thread, public EventReceiver, public IEventThreadBase {
private:
    std::unique_ptr<System::Synchronization::ISignal> _eventSignal;
    mutable System::Synchronization::Mutex _eventSignalMutex;

#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
    std::atomic<Task::TaskHandle> _eventWorkerTask{
        System::Execution::InvalidExecutionHandle
    };
    std::atomic<uint32_t> _eventSignalCycle{0};

    void TraceSignal(
        const char* phase,
        System::Synchronization::ISignal* signal,
        uint32_t cycle,
        std::size_t pending,
        bool resultKnown = false,
        bool result = false
    ) noexcept {
        const auto worker = _eventWorkerTask.load(std::memory_order_acquire);
        const auto current = Task::TaskRuntime::Current();
        std::printf(
            "[ESPressio Event][Signal] phase=%s receiver=%p thread=%u signal=%p cycle=%lu pending=%lu worker=0x%llX current=0x%llX",
            phase != nullptr ? phase : "unknown",
            static_cast<const void*>(this),
            static_cast<unsigned int>(GetThreadID()),
            static_cast<void*>(signal),
            static_cast<unsigned long>(cycle),
            static_cast<unsigned long>(pending),
            static_cast<unsigned long long>(worker),
            static_cast<unsigned long long>(current)
        );
        if (resultKnown) std::printf(" result=%s", result ? "success" : "failure");
        std::printf("\n");
        std::fflush(stdout);
    }
#endif

    System::Synchronization::ISignal* EnsureEventSignal() noexcept {
        if (_eventSignal != nullptr) return _eventSignal.get();

        std::lock_guard<System::Synchronization::Mutex> lock(_eventSignalMutex);
        if (_eventSignal == nullptr) {
            try {
                _eventSignal = System::Synchronization::CreateBinarySignal();
#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
                TraceSignal(
                    "created",
                    _eventSignal.get(),
                    _eventSignalCycle.load(std::memory_order_acquire),
                    GetPendingEventCount()
                );
#endif
            } catch (...) {
                return nullptr;
            }
        }
        return _eventSignal.get();
    }

protected:
    /// <summary>Waits for pending Events and drains them through <see cref="OnEvent"/>.</summary>
    void OnLoop() override {
#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
        _eventWorkerTask.store(Task::TaskRuntime::Current(), std::memory_order_release);
#endif
        auto* eventSignal = EnsureEventSignal();
        if (eventSignal != nullptr) {
            const auto pending = GetPendingEventCount();
#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
            const uint32_t cycle = _eventSignalCycle.fetch_add(1, std::memory_order_acq_rel) + 1;
            TraceSignal("wait-begin", eventSignal, cycle, pending);
#endif
            const auto waitResult = pending == 0
                ? eventSignal->Wait(System::Synchronization::WaitForever)
                : eventSignal->Wait(0);
#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
            TraceSignal(
                "wait-end", eventSignal, cycle, GetPendingEventCount(),
                true, static_cast<bool>(waitResult)
            );
#else
            (void)waitResult;
#endif
        } else {
            Task::TaskRuntime::SleepMilliseconds(1);
        }

        WithEvents(
            [&](
                IEvent* event,
                EventDispatchMethod dispatchMethod,
                EventPriority priority,
                const EventDispatchContext& context
            ) {
                OnEvent(event, dispatchMethod, priority, context);
            }
        );
    }

    virtual void OnEvent(
        IEvent* event,
        EventDispatchMethod dispatchMethod,
        EventPriority priority,
        const EventDispatchContext& context
    ) = 0;

    /// <summary>Signals the worker when new receiver work becomes available.</summary>
    void EventAdded() override {
        std::lock_guard<System::Synchronization::Mutex> lock(_eventSignalMutex);
        if (_eventSignal != nullptr) {
#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
            const uint32_t cycle = _eventSignalCycle.load(std::memory_order_acquire);
            TraceSignal("give-begin", _eventSignal.get(), cycle, GetPendingEventCount());
#endif
            const auto giveResult = _eventSignal->Give();
#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
            TraceSignal(
                "give-end", _eventSignal.get(), cycle, GetPendingEventCount(),
                true, static_cast<bool>(giveResult)
            );
#else
            (void)giveResult;
#endif
        }
    }

public:
    explicit EventThreadBase(ThreadReleasePolicy releasePolicy) : Thread(releasePolicy) {
        SetPriority(ESPRESSIO_EVENT_THREAD_DEFAULT_PRIORITY);
        SetCoreID(ESPRESSIO_EVENT_THREAD_DEFAULT_CORE_ID);
    }

    ~EventThreadBase() override = default;
};

} // namespace Event
} // namespace ESPressio
