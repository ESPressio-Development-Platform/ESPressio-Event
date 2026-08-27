#pragma once

#include <memory>

#include <ESPressio_Synchronization.hpp>
#include <ESPressio_Thread.hpp>

#include "ESPressio_EventDispatcher.hpp"
#include "ESPressio_EventManagerObservable.hpp"

#ifndef ESPRESSIO_EVENT_MANAGER_PRIORITY
    #define ESPRESSIO_EVENT_MANAGER_PRIORITY 2
#endif

#ifndef ESPRESSIO_EVENT_MANAGER_CORE_ID
    #define ESPRESSIO_EVENT_MANAGER_CORE_ID 0
#endif

using namespace ESPressio::Threads;

namespace ESPressio {

    namespace Event {

        class EventManager : public Thread, public EventDispatcher {
            private:
                std::unique_ptr<System::Synchronization::ISignal>
                    _eventSignal =
                        System::Synchronization::CreateBinarySignal();

                std::shared_ptr<EventManagerObservable> _observable =
                    CreateEventManagerObservable();

            protected:
                EventManager() :
                    Thread(
                        ThreadReleasePolicy::ReleaseOnTerminate
                    ) {

                    SetPriority(
                        ESPRESSIO_EVENT_MANAGER_PRIORITY
                    );
                    SetCoreID(
                        ESPRESSIO_EVENT_MANAGER_CORE_ID
                    );
                    Initialize();
                    Start();
                }

                void OnLoop() override {
                    /*
                     * If work arrived before the worker reached this wait, the
                     * binary signal remains set. When work is already pending,
                     * consume a possibly accumulated signal without blocking
                     * and drain the queue immediately.
                     */
                    if (_eventSignal != nullptr) {
                        if (GetPendingEventCount() == 0) {
                            (void)_eventSignal->Wait(
                                System::Synchronization::WaitForever
                            );
                        } else {
                            (void)_eventSignal->Wait(0);
                        }
                    }

                    DispatchEvents();
                }

                void EventAdded() override {
                    if (_eventSignal != nullptr) {
                        (void)_eventSignal->Give();
                    }
                }

                void OnEventDispatched(
                    IEvent* event,
                    EventDispatchMethod method,
                    EventPriority priority
                ) override {
                    _observable->EventDispatched(
                        event, method, priority,
                        event->__getDispatchContext()
                    );
                }

            public:
                Observable::ObserverHandlePtr RegisterObserver(
                    IEventManagerObserver* observer
                ) {
                    return _observable->RegisterObserver(observer);
                }

                void UnregisterObserver(
                    IEventManagerObserver* observer
                ) {
                    _observable->UnregisterObserver(observer);
                }

                static EventManager* GetInstance() {
                    static EventManager* instance = new EventManager();
                    return instance;
                }

                ~EventManager() override = default;

        };

    }

}
