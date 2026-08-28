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

        /// <summary>Marker interface shared by event-processing thread implementations.</summary>
        class IEventThreadBase {

        };

        /// <summary>Thread/receiver base that waits for queued events and forwards them to a derived event handler.</summary>
        class EventThreadBase : public Thread, public EventReceiver, public IEventThreadBase {
            private:
                std::unique_ptr<System::Synchronization::ISignal>
                    _eventSignal;

                mutable std::mutex
                    _eventSignalMutex;

#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
                std::atomic<Task::TaskHandle>
                    _eventWorkerTask{
                        System::Execution::InvalidExecutionHandle
                    };

                std::atomic<uint32_t>
                    _eventSignalCycle{0};

                void TraceSignal(
                    const char* phase,
                    System::Synchronization::ISignal* signal,
                    uint32_t cycle,
                    std::size_t pending,
                    bool resultKnown = false,
                    bool result = false
                ) const noexcept {
                    const auto worker =
                        _eventWorkerTask.load(std::memory_order_acquire);
                    const auto current =
                        Task::TaskRuntime::Current();

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
                    if (resultKnown) {
                        std::printf(" result=%s", result ? "success" : "failure");
                    }
                    std::printf("\n");
                    std::fflush(stdout);
                }
#endif

                System::Synchronization::ISignal*
                EnsureEventSignal() noexcept {
                    if (_eventSignal != nullptr) {
                        return _eventSignal.get();
                    }

                    std::lock_guard<std::mutex>
                        lock(_eventSignalMutex);

                    if (_eventSignal == nullptr) {
                        try {
                            _eventSignal =
                                System::Synchronization::
                                    CreateBinarySignal();
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
                /// <summary>Waits for pending events and drains them through <see cref="OnEvent"/>.</summary>
                /// <remarks>The receiver signal is materialized lazily from the active System synchronization provider when the worker first runs. This avoids process-lifetime EventThread instances retaining deferred pre-provider synchronization wrappers.</remarks>
                void OnLoop() override {
#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
                    _eventWorkerTask.store(
                        Task::TaskRuntime::Current(),
                        std::memory_order_release
                    );
#endif

                    auto* eventSignal =
                        EnsureEventSignal();

                    if (eventSignal != nullptr) {
                        const auto pending = GetPendingEventCount();
#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
                        const uint32_t cycle =
                            _eventSignalCycle.fetch_add(
                                1,
                                std::memory_order_acq_rel
                            ) + 1;
                        TraceSignal(
                            "wait-begin",
                            eventSignal,
                            cycle,
                            pending
                        );
#endif
                        const auto waitResult =
                            pending == 0
                                ? eventSignal->Wait(
                                    System::Synchronization::WaitForever
                                )
                                : eventSignal->Wait(0);
#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
                        TraceSignal(
                            "wait-end",
                            eventSignal,
                            cycle,
                            GetPendingEventCount(),
                            true,
                            static_cast<bool>(waitResult)
                        );
#else
                        (void)waitResult;
#endif
                    } else {
                        // A missing platform signal must never turn an EventThread
                        // into a tight busy loop. Pending events remain retained and
                        // will be processed once the provider becomes available.
                        Task::TaskRuntime::SleepMilliseconds(1);
                    }

                    WithEvents(
                        [&](
                            IEvent* event,
                            EventDispatchMethod dispatchMethod,
                            EventPriority priority
                        ) {
                            OnEvent(
                                event,
                                dispatchMethod,
                                priority
                            );
                        }
                    );
                }

                /// <summary>Handles one event removed from the thread's receiver queue.</summary>
                virtual void OnEvent(
                    IEvent* event,
                    EventDispatchMethod dispatchMethod,
                    EventPriority priority
                ) = 0;

                /// <summary>Signals the worker when new receiver work becomes available.</summary>
                /// <remarks>If the worker has not yet materialized its runtime signal, no wake is required: its first loop observes the retained pending-event count directly. When <c>ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS</c> is enabled, the receiver, signal, worker task, dispatching task and wake cycle are emitted immediately before and after the signal operation.</remarks>
                void EventAdded() override {
                    std::lock_guard<std::mutex>
                        lock(_eventSignalMutex);

                    if (_eventSignal != nullptr) {
#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
                        const uint32_t cycle =
                            _eventSignalCycle.load(std::memory_order_acquire);
                        TraceSignal(
                            "give-begin",
                            _eventSignal.get(),
                            cycle,
                            GetPendingEventCount()
                        );
#endif
                        const auto giveResult =
                            _eventSignal->Give();
#ifdef ESPRESSIO_EVENT_SIGNAL_DIAGNOSTICS
                        TraceSignal(
                            "give-end",
                            _eventSignal.get(),
                            cycle,
                            GetPendingEventCount(),
                            true,
                            static_cast<bool>(giveResult)
                        );
#else
                        (void)giveResult;
#endif
                    }
                }

            public:
                /// <summary>Constructs an event-processing thread with the configured release policy and default scheduling parameters.</summary>
                explicit EventThreadBase(ThreadReleasePolicy releasePolicy) :
                    Thread(releasePolicy) {
                    SetPriority(
                        ESPRESSIO_EVENT_THREAD_DEFAULT_PRIORITY
                    );
                    SetCoreID(
                        ESPRESSIO_EVENT_THREAD_DEFAULT_CORE_ID
                    );
                }

                ~EventThreadBase() override = default;
        };

    }

}
