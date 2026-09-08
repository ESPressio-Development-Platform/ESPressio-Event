#pragma once

#include <atomic>

#include <ESPressio_Thread.hpp>
#include <ESPressio_EventThreadBase.hpp>

#include "ESPressio_EventListener.hpp"
#include "ESPressio_EventManager.hpp"

namespace ESPressio {
namespace Event {

/// <summary>Marker interface shared by concrete Event-thread variants.</summary>
/**
 * ESPressio Memory Audit
 * Members: none (standalone empty object occupies 1 byte; an eligible empty base may be optimized to 0 bytes).
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class IEventThread {};

/// <summary>Dedicated Event-processing thread that combines EventThreadBase with typed listener registration.</summary>
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 528 bytes [EventThreadBase: Thread: _taskExited: owned object: 4 bytes; EventThreadBase: Thread: _taskStartGate: owned object: 4 bytes; EventThreadBase: Thread: _taskConfigurationMutex: _owned: owned object: 4 bytes; EventThreadBase: Thread: _taskConfigurationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: Thread: _stateTransitionMutex: _owned: owned object: 4 bytes; EventThreadBase: Thread: _stateTransitionMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: Thread: _lifecycleObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: Thread: _callbackMutex: _owned: owned object: 4 bytes; EventThreadBase: Thread: _callbackMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: Thread: _onDestroy: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onInitialize: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onStart: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onPause: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onTerminate: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onTerminated: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onInitializationFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onExecutionFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onStateChange: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventThreadBase: EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: EventReceiver: _capacityAvailable: owned object: 4 bytes; EventThreadBase: EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventThreadBase: EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; EventThreadBase: _eventSignal: owned object: 4 bytes; EventThreadBase: _eventSignalMutex: _owned: owned object: 4 bytes; EventThreadBase: _eventSignalMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventListener: _listeners: implementation blocks containing N * (40 bytes) plus block-map pointers; EventListener: _listeners: N elements each: Callback: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventListener: _listeners: N elements each: CustomInterest: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventListener: _listenersMutex: _owned: owned object: 4 bytes; EventListener: _listenersMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Requires Stack/Heap Preallocation
 * Members:
 * - _acceptingEvents (std::atomic<bool>): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 532 bytes [EventThreadBase: Thread: _taskExited: owned object: 4 bytes; EventThreadBase: Thread: _taskStartGate: owned object: 4 bytes; EventThreadBase: Thread: _taskConfigurationMutex: _owned: owned object: 4 bytes; EventThreadBase: Thread: _taskConfigurationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: Thread: _stateTransitionMutex: _owned: owned object: 4 bytes; EventThreadBase: Thread: _stateTransitionMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: Thread: _lifecycleObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: Thread: _callbackMutex: _owned: owned object: 4 bytes; EventThreadBase: Thread: _callbackMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: Thread: _onDestroy: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onInitialize: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onStart: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onPause: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onTerminate: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onTerminated: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onInitializationFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onExecutionFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: Thread: _onStateChange: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThreadBase: EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventThreadBase: EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThreadBase: EventReceiver: _capacityAvailable: owned object: 4 bytes; EventThreadBase: EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventThreadBase: EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; EventThreadBase: _eventSignal: owned object: 4 bytes; EventThreadBase: _eventSignalMutex: _owned: owned object: 4 bytes; EventThreadBase: _eventSignalMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventListener: _listeners: implementation blocks containing N * (40 bytes) plus block-map pointers; EventListener: _listeners: N elements each: Callback: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventListener: _listeners: N elements each: CustomInterest: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventListener: _listenersMutex: _owned: owned object: 4 bytes; EventListener: _listenersMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class EventThread :
    public EventThreadBase,
    public EventListener,
    public IEventThread {
private:
    std::atomic<bool> _acceptingEvents{true};

    void StopReceivingEvents() noexcept {
        if (!_acceptingEvents.exchange(false)) return;
        StopAcceptingEvents();
        try { UnregisterAllListeners(); } catch (...) {}
        ClearPendingEvents();
    }

protected:
    /// <summary>Processes one received Event through the registered listener set with its dispatch provenance.</summary>
    void OnEvent(
        IEvent* event,
        EventDispatchMethod dispatchMethod,
        EventPriority priority,
        const EventDispatchContext& context
    ) override {
        try {
            ProcessEvent(event, dispatchMethod, priority, context);
        } catch (...) {
            StopReceivingEvents();
            throw;
        }
    }

    /// <summary>Connects a newly registered Event type to the global EventManager dispatcher.</summary>
    void OnListenerRegistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->RegisterReceiver(eventType, this);
    }

    /// <summary>Disconnects an Event type from the global EventManager dispatcher.</summary>
    void OnListenerUnregistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->UnregisterReceiver(eventType, this);
    }

public:
    /// <summary>Constructs a dedicated Event thread with the specified release policy.</summary>
    explicit EventThread(Threads::ThreadReleasePolicy releasePolicy)
        : EventThreadBase(releasePolicy) {}

    ~EventThread() override {
        Shutdown();
        StopReceivingEvents();
    }

    /// <summary>Stops accepting Events before requesting thread termination.</summary>
    void Terminate() override {
        StopReceivingEvents();
        EventThreadBase::Terminate();
    }
};

/// <summary>Selects whether queued Events are processed before or after each custom thread-loop iteration.</summary>
/**
 * ESPressio Memory Audit
 * Underlying storage: 4 bytes
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
enum EventThreadProcessOrder {
    /// <summary>Drain queued Events before invoking the custom loop body.</summary>
    EventsBeforeLoop,
    /// <summary>Invoke the custom loop body before draining queued Events.</summary>
    EventsAfterLoop
};

/// <summary>Thread variant that combines ordinary loop work with Event reception and typed listener dispatch.</summary>
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 496 bytes [Thread: _taskExited: owned object: 4 bytes; Thread: _taskStartGate: owned object: 4 bytes; Thread: _taskConfigurationMutex: _owned: owned object: 4 bytes; Thread: _taskConfigurationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _stateTransitionMutex: _owned: owned object: 4 bytes; Thread: _stateTransitionMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _lifecycleObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _callbackMutex: _owned: owned object: 4 bytes; Thread: _callbackMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _onDestroy: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onInitialize: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onStart: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onPause: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onTerminate: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onTerminated: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onInitializationFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onExecutionFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onStateChange: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventReceiver: _capacityAvailable: owned object: 4 bytes; EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; EventListener: _listeners: implementation blocks containing N * (40 bytes) plus block-map pointers; EventListener: _listeners: N elements each: Callback: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventListener: _listeners: N elements each: CustomInterest: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventListener: _listenersMutex: _owned: owned object: 4 bytes; EventListener: _listenersMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Requires Stack/Heap Preallocation
 * Members:
 * - _processOrder (EventThreadProcessOrder): 4 bytes [0 bytes dynamic allocation]
 * - _acceptingEvents (std::atomic<bool>): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 504 bytes [Thread: _taskExited: owned object: 4 bytes; Thread: _taskStartGate: owned object: 4 bytes; Thread: _taskConfigurationMutex: _owned: owned object: 4 bytes; Thread: _taskConfigurationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _stateTransitionMutex: _owned: owned object: 4 bytes; Thread: _stateTransitionMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _lifecycleObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _callbackMutex: _owned: owned object: 4 bytes; Thread: _callbackMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; Thread: _onDestroy: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onInitialize: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onStart: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onPause: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onTerminate: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onTerminated: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onInitializationFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onExecutionFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; Thread: _onStateChange: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventReceiver: _capacityAvailable: owned object: 4 bytes; EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; EventListener: _listeners: implementation blocks containing N * (40 bytes) plus block-map pointers; EventListener: _listeners: N elements each: Callback: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventListener: _listeners: N elements each: CustomInterest: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventListener: _listenersMutex: _owned: owned object: 4 bytes; EventListener: _listenersMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class EventThreadWithLoop :
    public Threads::Thread,
    public EventReceiver,
    public IEventThreadBase,
    public EventListener,
    public IEventThread {
private:
    EventThreadProcessOrder _processOrder = EventsBeforeLoop;
    std::atomic<bool> _acceptingEvents{true};

    void StopReceivingEvents() noexcept {
        if (!_acceptingEvents.exchange(false)) return;
        StopAcceptingEvents();
        try { UnregisterAllListeners(); } catch (...) {}
        ClearPendingEvents();
    }

protected:
    /// <summary>Runs one loop iteration and drains Events according to the configured processing order.</summary>
    void OnLoop() override {
        try {
            if (_processOrder == EventsBeforeLoop) {
                WithEvents(
                    [&](
                        IEvent* event,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority,
                        const EventDispatchContext& context
                    ) {
                        ProcessEvent(event, dispatchMethod, priority, context);
                    }
                );
            }

            OnThreadLoop();

            if (_processOrder == EventsAfterLoop) {
                WithEvents(
                    [&](
                        IEvent* event,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority,
                        const EventDispatchContext& context
                    ) {
                        ProcessEvent(event, dispatchMethod, priority, context);
                    }
                );
            }
        } catch (...) {
            StopReceivingEvents();
            throw;
        }
    }

    /// <summary>Executes application-specific non-Event work for one thread-loop iteration.</summary>
    virtual void OnThreadLoop() = 0;

    /// <summary>Connects a newly registered Event type to the global EventManager dispatcher.</summary>
    void OnListenerRegistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->RegisterReceiver(eventType, this);
    }

    /// <summary>Disconnects an Event type from the global EventManager dispatcher.</summary>
    void OnListenerUnregistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->UnregisterReceiver(eventType, this);
    }

public:
    /// <summary>Constructs a combined loop/Event thread with the specified release policy.</summary>
    explicit EventThreadWithLoop(Threads::ThreadReleasePolicy releasePolicy)
        : Threads::Thread(releasePolicy) {
        SetPriority(ESPRESSIO_EVENT_THREAD_DEFAULT_PRIORITY);
        SetCoreID(ESPRESSIO_EVENT_THREAD_DEFAULT_CORE_ID);
    }

    ~EventThreadWithLoop() override {
        Shutdown();
        StopReceivingEvents();
    }

    /// <summary>Stops accepting Events before requesting thread termination.</summary>
    void Terminate() override {
        StopReceivingEvents();
        Threads::Thread::Terminate();
    }

    /// <summary>Returns whether Events are processed before or after the custom loop body.</summary>
    EventThreadProcessOrder GetProcessOrder() const {
        return _processOrder;
    }

    /// <summary>Sets whether Events are processed before or after the custom loop body.</summary>
    void SetProcessOrder(EventThreadProcessOrder processOrder) {
        _processOrder = processOrder;
    }
};

} // namespace Event
} // namespace ESPressio
