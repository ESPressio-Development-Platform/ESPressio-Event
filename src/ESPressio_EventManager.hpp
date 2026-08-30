#pragma once

#include <atomic>
#include <memory>

#include <ESPressio_Synchronization.hpp>
#include <ESPressio_TaskExecutor.hpp>
#include <ESPressio_Thread.hpp>

#include "ESPressio_EventDispatcher.hpp"
#include "ESPressio_EventManagerObservable.hpp"

#ifndef ESPRESSIO_EVENT_MANAGER_PRIORITY
    #define ESPRESSIO_EVENT_MANAGER_PRIORITY 2
#endif

#ifndef ESPRESSIO_EVENT_MANAGER_CORE_ID
    #define ESPRESSIO_EVENT_MANAGER_CORE_ID 0
#endif

#ifndef ESPRESSIO_EVENT_MANAGER_OBSERVER_TASK_STACK_SIZE
    #define ESPRESSIO_EVENT_MANAGER_OBSERVER_TASK_STACK_SIZE 4096
#endif

#ifndef ESPRESSIO_EVENT_MANAGER_OBSERVER_QUEUE_DEPTH
    #define ESPRESSIO_EVENT_MANAGER_OBSERVER_QUEUE_DEPTH 16
#endif

#ifndef ESPRESSIO_EVENT_MANAGER_OBSERVER_TASK_PRIORITY
    #define ESPRESSIO_EVENT_MANAGER_OBSERVER_TASK_PRIORITY 1
#endif

using namespace ESPressio::Threads;

namespace ESPressio {
namespace Event {

/// <summary>Singleton non-blocking broker that drains the global Event ingress queue and fans references out to receivers.</summary>
/// <remarks>
/// EventManager never executes observer callbacks. Dispatch observation is submitted to a bounded TaskExecutor that owns
/// one additional intrusive Event reference until observer notification completes or the work item is discarded.
/// Downstream receiver admission is non-blocking through EventDispatcher.
/// </remarks>
class EventManager : public Thread, public EventDispatcher {
private:
    struct ObserverWork {
        IEvent* Event = nullptr;
        EventDispatchMethod Method = EventDispatchMethod::Queue;
        EventPriority Priority = EventPriority::Normal;
        EventDispatchContext Context{};
    };

    static_assert(
        std::is_trivially_copyable<ObserverWork>::value,
        "EventManager observer work must remain trivially copyable"
    );

    std::unique_ptr<System::Synchronization::ISignal> _eventSignal =
        System::Synchronization::CreateBinarySignal();

    std::shared_ptr<EventManagerObservable> _observable =
        CreateEventManagerObservable();

    Task::TaskExecutor<ObserverWork> _observerExecutor;
    std::atomic<bool> _observerExecutorReady{false};

    static Task::TaskConfiguration CreateObserverTaskConfiguration() {
        Task::TaskConfiguration configuration;
        configuration.Name = "eventObservers";
        configuration.StackSize = ESPRESSIO_EVENT_MANAGER_OBSERVER_TASK_STACK_SIZE;
        configuration.Priority = ESPRESSIO_EVENT_MANAGER_OBSERVER_TASK_PRIORITY;
        configuration.Core = -1;
        configuration.QueueDepth = ESPRESSIO_EVENT_MANAGER_OBSERVER_QUEUE_DEPTH;
        configuration.OverflowPolicy = Task::TaskQueueOverflowPolicy::Reject;
        configuration.MemoryPolicy = Task::TaskMemoryPolicy::PreferExternal;
        return configuration;
    }

    void ReleaseObserverWork(const ObserverWork& work) noexcept {
        if (work.Event != nullptr) {
            work.Event->__unref();
        }
    }

    void ProcessObserverWork(const ObserverWork& work) noexcept {
        class EventReferenceGuard final {
        private:
            IEvent* _event;
        public:
            explicit EventReferenceGuard(IEvent* event) : _event(event) {}
            ~EventReferenceGuard() {
                if (_event != nullptr) _event->__unref();
            }
        } reference(work.Event);

        if (work.Event == nullptr || !_observable) return;
        try {
            _observable->EventDispatched(
                work.Event,
                work.Method,
                work.Priority,
                work.Context
            );
        } catch (...) {
            // Observation is isolated from both the broker and its executor.
        }
    }

    void SubmitObserverNotification(
        IEvent* event,
        EventDispatchMethod method,
        EventPriority priority
    ) noexcept {
        if (
            event == nullptr ||
            !_observerExecutorReady.load(std::memory_order_acquire)
        ) {
            return;
        }

        ObserverWork work;
        work.Event = event;
        work.Method = method;
        work.Priority = priority;
        work.Context = event->__getDispatchContext();

        event->__ref();
        const auto status = _observerExecutor.Submit(work);
        if (status != Task::TaskExecutionStatus::Success) {
            event->__unref();
        }
    }

    EventManager()
        : Thread(ThreadReleasePolicy::ReleaseOnTerminate),
          _observerExecutor(CreateObserverTaskConfiguration()) {
        SetPriority(ESPRESSIO_EVENT_MANAGER_PRIORITY);
        SetCoreID(ESPRESSIO_EVENT_MANAGER_CORE_ID);

        const auto observerInitialization = _observerExecutor.Initialize(
            [this](const ObserverWork& work) {
                ProcessObserverWork(work);
            },
            [this](const ObserverWork& work) {
                ReleaseObserverWork(work);
            }
        );
        if (
            observerInitialization == Task::TaskExecutionStatus::Success &&
            _observerExecutor.Start() == Task::TaskExecutionStatus::Success
        ) {
            _observerExecutorReady.store(true, std::memory_order_release);
        }

        Initialize();
        Start();
    }

protected:
    /// <summary>Waits for ingress work, fans it out, and schedules asynchronous dispatch observation.</summary>
    void OnLoop() override {
        if (_eventSignal != nullptr) {
            if (GetPendingEventCount() == 0) {
                (void)_eventSignal->Wait(System::Synchronization::WaitForever);
            } else {
                (void)_eventSignal->Wait(0);
            }
        }

        DispatchEvents(
            [this](
                IEvent* event,
                EventDispatchMethod method,
                EventPriority priority
            ) {
                SubmitObserverNotification(event, method, priority);
            }
        );
    }

    void EventAdded() override {
        if (_eventSignal != nullptr) {
            (void)_eventSignal->Give();
        }
    }

public:
    Observable::ObserverHandlePtr RegisterObserver(
        IEventManagerObserver* observer
    ) {
        return _observable->RegisterObserver(observer);
    }

    void UnregisterObserver(IEventManagerObserver* observer) {
        _observable->UnregisterObserver(observer);
    }

    /// <summary>Returns statistics for asynchronous EventManager observer delivery.</summary>
    Task::TaskExecutionStatistics GetObserverExecutionStatistics() const {
        return _observerExecutor.GetStatistics();
    }

    /// <summary>Returns whether the asynchronous observer executor is available.</summary>
    bool IsObserverExecutorReady() const noexcept {
        return _observerExecutorReady.load(std::memory_order_acquire);
    }

    static EventManager* GetInstance() {
        static EventManager* instance = new EventManager();
        return instance;
    }

    ~EventManager() override {
        _observerExecutorReady.store(false, std::memory_order_release);
        _observerExecutor.Stop();
    }
};

} // namespace Event
} // namespace ESPressio
