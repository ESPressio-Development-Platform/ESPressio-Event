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

        /// <summary>Singleton worker that drains the global event queue and performs typed event dispatch.</summary>
        /// <remarks>The manager owns a dedicated ESPressio Thread and is signalled whenever new event work is added.</remarks>
        class EventManager : public Thread, public EventDispatcher {
            private:
                std::unique_ptr<System::Synchronization::ISignal>
                    _eventSignal =
                        System::Synchronization::CreateBinarySignal();

                std::shared_ptr<EventManagerObservable> _observable =
                    CreateEventManagerObservable();

            protected:
                /// <summary>Constructs and starts the singleton event-dispatch worker.</summary>
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

                /// <summary>Waits for queued work and drains available events through the dispatcher.</summary>
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

                /// <summary>Signals the worker whenever event work is added to the receiver queue.</summary>
                void EventAdded() override {
                    if (_eventSignal != nullptr) {
                        (void)_eventSignal->Give();
                    }
                }

                /// <summary>Publishes manager-level notification after an event has entered dispatch.</summary>
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
                /// <summary>Registers an observer for manager-level event dispatch notifications.</summary>
                Observable::ObserverHandlePtr RegisterObserver(
                    IEventManagerObserver* observer
                ) {
                    return _observable->RegisterObserver(observer);
                }

                /// <summary>Unregisters a manager-level event observer.</summary>
                void UnregisterObserver(
                    IEventManagerObserver* observer
                ) {
                    _observable->UnregisterObserver(observer);
                }

                /// <summary>Returns the process-wide event manager singleton.</summary>
                static EventManager* GetInstance() {
                    static EventManager* instance = new EventManager();
                    return instance;
                }

                ~EventManager() override = default;

        };

    }

}
