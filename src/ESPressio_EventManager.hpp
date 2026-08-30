#pragma once

#include <atomic>
#include <memory>
#include <type_traits>

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
/// Constructing/accessing the singleton has no execution or ThreadManager-registration side effects. Initialize() registers
/// and creates the broker and observer execution resources, and Start() explicitly releases them to run. Events may be queued
/// before Start(); they remain pending until the broker starts. EventManager never executes observer callbacks. Dispatch
/// observation is submitted to a bounded TaskExecutor only while observers are actually registered; otherwise no additional
/// Event reference or observer work item is created. Downstream receiver admission is non-blocking through EventDispatcher.
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
    std::atomic<bool> _observerExecutorInitialized{false};
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
        if (work.Event != nullptr) work.Event->__unref();
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

        if (work.Event == nullptr || !_observable || !_observable->HasObservers()) return;
        try {
            _observable->EventDispatched(
                work.Event,
                work.Method,
                work.Priority,
                work.Context
            );
        } catch (...) {}
    }

    void SubmitObserverNotification(
        IEvent* event,
        EventDispatchMethod method,
        EventPriority priority
    ) noexcept {
        if (
            event == nullptr ||
            !_observerExecutorReady.load(std::memory_order_acquire) ||
            !_observable ||
            !_observable->HasObservers()
        ) return;

        ObserverWork work;
        work.Event = event;
        work.Method = method;
        work.Priority = priority;
        work.Context = event->__getDispatchContext();

        event->__ref();
        const auto status = _observerExecutor.Submit(work);
        if (status != Task::TaskExecutionStatus::Success) event->__unref();
    }

    EventManager()
        : Thread(
              ThreadReleasePolicy::ReleaseOnTerminate,
              ThreadRegistrationPolicy::DeferredUntilInitialize
          ),
          _observerExecutor(CreateObserverTaskConfiguration()) {
        SetPriority(ESPRESSIO_EVENT_MANAGER_PRIORITY);
        SetCoreID(ESPRESSIO_EVENT_MANAGER_CORE_ID);
        SetStartOnInitialize(false);
    }

protected:
    /// <summary>Waits for ingress work, fans it out, and schedules asynchronous dispatch observation when required.</summary>
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
        if (_eventSignal != nullptr) (void)_eventSignal->Give();
    }

public:
    /// <summary>Initializes broker and asynchronous observer execution resources without starting Event dispatch.</summary>
    ThreadInitializationStatus Initialize() override {
        if (!_observerExecutorInitialized.load(std::memory_order_acquire)) {
            const auto observerInitialization = _observerExecutor.Initialize(
                [this](const ObserverWork& work) { ProcessObserverWork(work); },
                [this](const ObserverWork& work) { ReleaseObserverWork(work); }
            );
            if (
                observerInitialization != Task::TaskExecutionStatus::Success &&
                observerInitialization != Task::TaskExecutionStatus::AlreadyInitialized
            ) {
                return ThreadInitializationStatus::TaskCreationFailed;
            }
            _observerExecutorInitialized.store(true, std::memory_order_release);
        }

        const auto threadInitialization = Thread::Initialize();
        if (
            threadInitialization != ThreadInitializationStatus::Success &&
            threadInitialization != ThreadInitializationStatus::AlreadyInitialized
        ) {
            _observerExecutor.Stop();
            _observerExecutorInitialized.store(false, std::memory_order_release);
            _observerExecutorReady.store(false, std::memory_order_release);
        }
        return threadInitialization;
    }

    /// <summary>Starts asynchronous observer execution and then releases the broker Thread to dispatch pending Events.</summary>
    ThreadInitializationStatus Start() override {
        if (!_observerExecutorInitialized.load(std::memory_order_acquire)) {
            const auto initialization = Initialize();
            if (
                initialization != ThreadInitializationStatus::Success &&
                initialization != ThreadInitializationStatus::AlreadyInitialized
            ) return initialization;
        }

        if (!_observerExecutorReady.load(std::memory_order_acquire)) {
            const auto observerStart = _observerExecutor.Start();
            if (
                observerStart != Task::TaskExecutionStatus::Success &&
                observerStart != Task::TaskExecutionStatus::AlreadyStarted
            ) {
                return ThreadInitializationStatus::TaskCreationFailed;
            }
            _observerExecutorReady.store(true, std::memory_order_release);
        }

        const auto threadStart = Thread::Start();
        if (
            threadStart != ThreadInitializationStatus::Success &&
            threadStart != ThreadInitializationStatus::AlreadyInitialized
        ) {
            _observerExecutorReady.store(false, std::memory_order_release);
            _observerExecutor.Stop();
            _observerExecutorInitialized.store(false, std::memory_order_release);
        }
        return threadStart;
    }

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

    /// <summary>Returns whether any EventManager observer is currently registered.</summary>
    bool HasObservers() const noexcept {
        return _observable && _observable->HasObservers();
    }

    static EventManager* GetInstance() {
        static EventManager* instance = new EventManager();
        return instance;
    }

    ~EventManager() override {
        _observerExecutorReady.store(false, std::memory_order_release);
        _observerExecutor.Stop();
        _observerExecutorInitialized.store(false, std::memory_order_release);
    }
};

} // namespace Event
} // namespace ESPressio
