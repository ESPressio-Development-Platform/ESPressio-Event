#include <Arduino.h>

#include <ESPressio_Event.hpp>
#include <ESPressio_EventThread.hpp>

#include <ESPressio_ThreadEventBridges.hpp>
#include <ESPressio_ThreadEvents.hpp>

using namespace ESPressio;

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 532 bytes [EventThread: EventThreadBase: Thread: _taskExited: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _taskStartGate: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _taskConfigurationMutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _taskConfigurationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: Thread: _stateTransitionMutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _stateTransitionMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: Thread: _lifecycleObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: Thread: _callbackMutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _callbackMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: Thread: _onDestroy: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onInitialize: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onStart: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onPause: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onTerminate: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onTerminated: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onInitializationFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onExecutionFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onStateChange: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: EventReceiver: _capacityAvailable: owned object: 4 bytes; EventThread: EventThreadBase: EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventThread: EventThreadBase: EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; EventThread: EventThreadBase: _eventSignal: owned object: 4 bytes; EventThread: EventThreadBase: _eventSignalMutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: _eventSignalMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventListener: _listeners: implementation blocks containing N * (40 bytes) plus block-map pointers; EventThread: EventListener: _listeners: N elements each: Callback: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventListener: _listeners: N elements each: CustomInterest: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventListener: _listenersMutex: _owned: owned object: 4 bytes; EventThread: EventListener: _listenersMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Requires Stack/Heap Preallocation
 * Members:
 * - _registeredHandle (Event::EventListenerHandlePtr): 12 bytes [owned object: 4 bytes]
 * - _cleanupHandle (Event::EventListenerHandlePtr): 12 bytes [owned object: 4 bytes]
 * - _terminationHandle (Event::EventListenerHandlePtr): 12 bytes [owned object: 4 bytes]
 * Total Memory: 568 bytes [EventThread: EventThreadBase: Thread: _taskExited: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _taskStartGate: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _taskConfigurationMutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _taskConfigurationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: Thread: _stateTransitionMutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _stateTransitionMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: Thread: _lifecycleObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: Thread: _callbackMutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: Thread: _callbackMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: Thread: _onDestroy: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onInitialize: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onStart: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onPause: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onTerminate: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onTerminated: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onInitializationFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onExecutionFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: Thread: _onStateChange: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventThreadBase: EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventThreadBase: EventReceiver: _capacityAvailable: owned object: 4 bytes; EventThread: EventThreadBase: EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventThread: EventThreadBase: EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; EventThread: EventThreadBase: _eventSignal: owned object: 4 bytes; EventThread: EventThreadBase: _eventSignalMutex: _owned: owned object: 4 bytes; EventThread: EventThreadBase: _eventSignalMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventThread: EventListener: _listeners: implementation blocks containing N * (40 bytes) plus block-map pointers; EventThread: EventListener: _listeners: N elements each: Callback: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventListener: _listeners: N elements each: CustomInterest: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventThread: EventListener: _listenersMutex: _owned: owned object: 4 bytes; EventThread: EventListener: _listenersMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _registeredHandle: owned object: 4 bytes; _cleanupHandle: owned object: 4 bytes; _terminationHandle: owned object: 4 bytes]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class InfrastructureEventThread final :
    public Event::EventThread {

private:
    Event::EventListenerHandlePtr
        _registeredHandle;

    Event::EventListenerHandlePtr
        _cleanupHandle;

    Event::EventListenerHandlePtr
        _terminationHandle;

public:
    InfrastructureEventThread() :
        Event::EventThread(
            Threads::ThreadReleasePolicy::ExplicitRelease
        ) {

        _registeredHandle =
            RegisterListener<
                Event::ThreadRegisteredEvent
            >(
                [](
                    Event::ThreadRegisteredEvent* event,
                    Event::EventDispatchMethod,
                    Event::EventPriority,
                    const Event::EventDispatchContext&
                ) {
                    Serial.printf(
                        "registered id=%u core=%d\n",
                        event->Snapshot.ThreadID,
                        event->Snapshot.CoreID
                    );
                }
            );

        _cleanupHandle =
            RegisterListener<
                Event::ThreadCleanupCompletedEvent
            >(
                [](
                    Event::ThreadCleanupCompletedEvent* event,
                    Event::EventDispatchMethod,
                    Event::EventPriority,
                    const Event::EventDispatchContext&
                ) {
                    Serial.printf(
                        "cleanup deleted=%u\n",
                        static_cast<unsigned int>(
                            event->Result.ThreadsDeleted
                        )
                    );
                }
            );

        _terminationHandle =
            RegisterListener<
                Event::ThreadTerminationDispatchCompletedEvent
            >(
                [](
                    Event::ThreadTerminationDispatchCompletedEvent* event,
                    Event::EventDispatchMethod,
                    Event::EventPriority,
                    const Event::EventDispatchContext&
                ) {
                    Serial.printf(
                        "termination dispatched id=%u\n",
                        event->Snapshot.ThreadID
                    );
                }
            );
    }
};

InfrastructureEventThread infrastructureEvents;

void setup() {
    Serial.begin(115200);

    Event::ThreadManagerEventBridge::
        GetInstance().
        Initialize();

    Event::ThreadTerminationDispatcherEventBridge::
        GetInstance().
        Initialize();

    Threads::ThreadManager::
        GetInstance()->
        Initialize();
}

void loop() {
    delay(1000);
}
