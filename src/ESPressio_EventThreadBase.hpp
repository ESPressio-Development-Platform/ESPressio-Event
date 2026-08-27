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

        class IEventThreadBase {

        };

        class EventThreadBase : public Thread, public EventReceiver, public IEventThreadBase {
            private:
                std::unique_ptr<System::Synchronization::ISignal>
                    _eventSignal =
                        System::Synchronization::CreateBinarySignal();

            protected:
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

                virtual void OnEvent(
                    IEvent* event,
                    EventDispatchMethod dispatchMethod,
                    EventPriority priority
                ) = 0;

                void EventAdded() override {
                    if (_eventSignal != nullptr) {
                        (void)_eventSignal->Give();
                    }
                }

            public:
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
