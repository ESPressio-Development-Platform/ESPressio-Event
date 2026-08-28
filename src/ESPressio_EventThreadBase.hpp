#pragma once

#include <memory>

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
                    _eventSignal =
                        System::Synchronization::CreateBinarySignal();

            protected:
                /// <summary>Waits for pending events and drains them through <see cref="OnEvent"/>.</summary>
                void OnLoop() override {
                    if (_eventSignal != nullptr) {
                        if (GetPendingEventCount() == 0) {
                            (void)_eventSignal->Wait(
                                System::Synchronization::WaitForever
                            );
                        } else {
                            (void)_eventSignal->Wait(0);
                        }
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
                void EventAdded() override {
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
