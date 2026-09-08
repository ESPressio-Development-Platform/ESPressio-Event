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
/// Constructing/accessing the singleton has no execution or ThreadManager-registration side effects. Initialize() creates
/// only the broker execution resources. Observer execution resources are allocated lazily when observation is actually
/// requested, so consumers that do not register EventManager observers pay no observer-task stack or queue cost. Start()
/// explicitly releases the broker and any required observer executor to run. Events may be queued before Start(); they
/// remain pending until the broker starts. EventManager never executes observer callbacks. Dispatch observation is
/// submitted to a bounded TaskExecutor only while observers are actually registered; otherwise no additional Event
/// reference or observer work item is created. Downstream receiver admission is non-blocking through EventDispatcher.
/// Local/remote provenance is retained beside queued work and never written into the Event object.
/// </remarks>
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 464 bytes [Thread: _taskExited: owned object: 4 bytes; Thread: _taskStartGate: owned object: 4 bytes; Thread: _taskConfigurationMutex: _owned: owned object: 4 bytes; Thread: _taskConfigurationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _stateTransitionMutex: _owned: owned object: 4 bytes; Thread: _stateTransitionMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _lifecycleObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _callbackMutex: _owned: owned object: 4 bytes; Thread: _callbackMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _onDestroy: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onInitialize: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onStart: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onPause: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onTerminate: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onTerminated: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onInitializationFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onExecutionFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onStateChange: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventDispatcher: EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventDispatcher: EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventDispatcher: EventReceiver: _capacityAvailable: owned object: 4 bytes; EventDispatcher: EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventDispatcher: EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; EventDispatcher: _eventReceivers: Capacity * (12 bytes) element storage; EventDispatcher: _eventReceiversMutex: _owned: owned object: 4 bytes; EventDispatcher: _eventReceiversMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Requires Stack/Heap Preallocation
 * Members:
 * - _eventSignal (std::unique_ptr<System::Synchronization::ISignal>): 4 bytes [owned object: 4 bytes]
 * - _observable (std::shared_ptr<EventManagerObservable>): 8 bytes [shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * - _observerExecutor (Task::TaskExecutor<ObserverWork>): 160 bytes [_handler: Name: Capacity + 1 bytes when capacity exceeds 15-byte SSO; _handler: LastId: Capacity + 1 bytes when capacity exceeds 15-byte SSO; _queue: owned object: 4 bytes; _startGate: owned object: 4 bytes; _lifecycleMutex: _owned: owned object: 4 bytes; _lifecycleMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * - _observerLifecycleMutex (System::Synchronization::Mutex): 20 bytes [_owned: owned object: 4 bytes; _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * - _observerExecutorInitialized (std::atomic<bool>): 1 bytes [0 bytes dynamic allocation]
 * - _observerExecutorReady (std::atomic<bool>): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 660 bytes [Thread: _taskExited: owned object: 4 bytes; Thread: _taskStartGate: owned object: 4 bytes; Thread: _taskConfigurationMutex: _owned: owned object: 4 bytes; Thread: _taskConfigurationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _stateTransitionMutex: _owned: owned object: 4 bytes; Thread: _stateTransitionMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _lifecycleObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _callbackMutex: _owned: owned object: 4 bytes; Thread: _callbackMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _onDestroy: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onInitialize: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onStart: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onPause: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onTerminate: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onTerminated: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onInitializationFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onExecutionFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onStateChange: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventDispatcher: EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventDispatcher: EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventDispatcher: EventReceiver: _capacityAvailable: owned object: 4 bytes; EventDispatcher: EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventDispatcher: EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; EventDispatcher: _eventReceivers: Capacity * (12 bytes) element storage; EventDispatcher: _eventReceiversMutex: _owned: owned object: 4 bytes; EventDispatcher: _eventReceiversMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _eventSignal: owned object: 4 bytes; _observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _observerExecutor: _handler: Name: Capacity + 1 bytes when capacity exceeds 15-byte SSO; _observerExecutor: _handler: LastId: Capacity + 1 bytes when capacity exceeds 15-byte SSO; _observerExecutor: _queue: owned object: 4 bytes; _observerExecutor: _startGate: owned object: 4 bytes; _observerExecutor: _lifecycleMutex: _owned: owned object: 4 bytes; _observerExecutor: _lifecycleMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _observerLifecycleMutex: _owned: owned object: 4 bytes; _observerLifecycleMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class EventManager : public Thread, public EventDispatcher {
private:
/**
 * ESPressio Memory Audit
 * Members:
 * - Event (IEvent*): 4 bytes [0 bytes dynamic allocation]
 * - Method (EventDispatchMethod): 4 bytes [0 bytes dynamic allocation]
 * - Priority (EventPriority): 4 bytes [0 bytes dynamic allocation]
 * - Context (EventDispatchContext): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 16 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
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
    mutable System::Synchronization::Mutex _observerLifecycleMutex;
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
/**
 * ESPressio Memory Audit
 * Members:
 * - _event (IEvent*): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
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

    bool EnsureObserverExecutorInitializedLocked() {
        if (_observerExecutorInitialized.load(std::memory_order_acquire)) return true;
        const auto status = _observerExecutor.Initialize(
            [this](const ObserverWork& work) { ProcessObserverWork(work); },
            [this](const ObserverWork& work) { ReleaseObserverWork(work); }
        );
        if (
            status != Task::TaskExecutionStatus::Success &&
            status != Task::TaskExecutionStatus::AlreadyInitialized
        ) return false;
        _observerExecutorInitialized.store(true, std::memory_order_release);
        return true;
    }

    bool EnsureObserverExecutorReadyLocked() {
        if (_observerExecutorReady.load(std::memory_order_acquire)) return true;
        if (!EnsureObserverExecutorInitializedLocked()) return false;
        const auto status = _observerExecutor.Start();
        if (
            status != Task::TaskExecutionStatus::Success &&
            status != Task::TaskExecutionStatus::AlreadyStarted
        ) return false;
        _observerExecutorReady.store(true, std::memory_order_release);
        return true;
    }

    void ReleaseObserverExecutorLocked() noexcept {
        _observerExecutorReady.store(false, std::memory_order_release);
        _observerExecutor.Stop();
        _observerExecutorInitialized.store(false, std::memory_order_release);
    }

    void SubmitObserverNotification(
        IEvent* event,
        EventDispatchMethod method,
        EventPriority priority,
        const EventDispatchContext& context
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
        work.Context = context;

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
                EventPriority priority,
                const EventDispatchContext& context
            ) {
                SubmitObserverNotification(event, method, priority, context);
            }
        );
    }

    void EventAdded() override {
        if (_eventSignal != nullptr) (void)_eventSignal->Give();
    }

public:
    /// <summary>Initializes the broker and any observer executor already required by registered observers.</summary>
    ThreadInitializationStatus Initialize() override {
        if (_observable && _observable->HasObservers()) {
            std::lock_guard<System::Synchronization::Mutex> observerLifecycle(
                _observerLifecycleMutex
            );
            if (!EnsureObserverExecutorInitializedLocked()) {
                return ThreadInitializationStatus::TaskCreationFailed;
            }
        }

        const auto threadInitialization = Thread::Initialize();
        if (
            threadInitialization != ThreadInitializationStatus::Success &&
            threadInitialization != ThreadInitializationStatus::AlreadyInitialized
        ) {
            std::lock_guard<System::Synchronization::Mutex> observerLifecycle(
                _observerLifecycleMutex
            );
            ReleaseObserverExecutorLocked();
        }
        return threadInitialization;
    }

    /// <summary>Starts asynchronous observer execution when required, then releases the broker Thread.</summary>
    ThreadInitializationStatus Start() override {
        if (GetThreadState() == ThreadState::Uninitialized) {
            const auto initialization = Initialize();
            if (
                initialization != ThreadInitializationStatus::Success &&
                initialization != ThreadInitializationStatus::AlreadyInitialized
            ) return initialization;
        }

        if (_observable && _observable->HasObservers()) {
            std::lock_guard<System::Synchronization::Mutex> observerLifecycle(
                _observerLifecycleMutex
            );
            if (!EnsureObserverExecutorReadyLocked()) {
                return ThreadInitializationStatus::TaskCreationFailed;
            }
        }

        const auto threadStart = Thread::Start();
        if (
            threadStart != ThreadInitializationStatus::Success &&
            threadStart != ThreadInitializationStatus::AlreadyInitialized
        ) {
            std::lock_guard<System::Synchronization::Mutex> observerLifecycle(
                _observerLifecycleMutex
            );
            ReleaseObserverExecutorLocked();
        }
        return threadStart;
    }

    Observable::ObserverHandlePtr RegisterObserver(
        IEventManagerObserver* observer
    ) {
        if (_observable == nullptr || observer == nullptr) return {};

        std::lock_guard<System::Synchronization::Mutex> observerLifecycle(
            _observerLifecycleMutex
        );
        auto handle = _observable->RegisterObserver(observer);
        if (!handle) return handle;

        const auto state = GetThreadState();
        if (
            state == ThreadState::Initialized ||
            state == ThreadState::Running ||
            state == ThreadState::Paused
        ) {
            if (!EnsureObserverExecutorInitializedLocked()) {
                _observable->UnregisterObserver(observer);
                return {};
            }
            if (state == ThreadState::Running && !EnsureObserverExecutorReadyLocked()) {
                _observable->UnregisterObserver(observer);
                return {};
            }
        }
        return handle;
    }

    void UnregisterObserver(IEventManagerObserver* observer) {
        if (_observable != nullptr) _observable->UnregisterObserver(observer);
    }

    /// <summary>Returns statistics for asynchronous EventManager observer delivery.</summary>
    Task::TaskExecutionStatistics GetObserverExecutionStatistics() const {
        return _observerExecutor.GetStatistics();
    }

    /// <summary>Returns whether observer execution resources have been allocated.</summary>
    bool IsObserverExecutorInitialized() const noexcept {
        return _observerExecutorInitialized.load(std::memory_order_acquire);
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
        std::lock_guard<System::Synchronization::Mutex> observerLifecycle(
            _observerLifecycleMutex
        );
        ReleaseObserverExecutorLocked();
    }
};

} // namespace Event
} // namespace ESPressio
