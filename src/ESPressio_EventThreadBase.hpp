#pragma once

#include <memory>
#include <mutex>

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
                    auto* eventSignal =
                        EnsureEventSignal();

                    if (eventSignal != nullptr) {
                        if (GetPendingEventCount() == 0) {
                            (void)eventSignal->Wait(
                                System::Synchronization::WaitForever
                            );
                        } else {
                            (void)eventSignal->Wait(0);
                        }
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
                /// <remarks>If the worker has not yet materialized its runtime signal, no wake is required: its first loop observes the retained pending-event count directly.</remarks>
                void EventAdded() override {
                    std::lock_guard<std::mutex>
                        lock(_eventSignalMutex);

                    if (_eventSignal != nullptr) {
                        (void)_eventSignal->Give();
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
