#include <ESPressio_Event.hpp>
#include <ESPressio_PrecisionEventThread.hpp>
#include <ESPressio_ThreadManager.hpp>

using namespace ESPressio;

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - _setpoint (int): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 28 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SetpointEvent final :
    public Event::TypedEvent<SetpointEvent> {

    private:
        const int _setpoint;

    public:
        explicit SetpointEvent(
            int setpoint
        ) :
            _setpoint(setpoint) {
        }

        int GetSetpoint() const {
            return _setpoint;
        }
};


/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 740 bytes [PrecisionEventThread: PrecisionThread: Thread: _taskExited: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _taskStartGate: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _taskConfigurationMutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _taskConfigurationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: Thread: _stateTransitionMutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _stateTransitionMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: Thread: _callbackMutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _callbackMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: Thread: _onDestroy: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onInitialize: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onStart: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onPause: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onTerminate: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onTerminated: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onInitializationFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onExecutionFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onStateChange: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: _iterationObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: _scheduleSignal: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: _timingMutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: _timingMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: _iterationSamples: implementation blocks containing N * (8 bytes) plus block-map pointers; PrecisionEventThread: EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; PrecisionEventThread: EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: EventReceiver: _capacityAvailable: owned object: 4 bytes; PrecisionEventThread: EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; PrecisionEventThread: EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; PrecisionEventThread: EventListener: _listeners: implementation blocks containing N * (40 bytes) plus block-map pointers; PrecisionEventThread: EventListener: _listeners: N elements each: Callback: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: EventListener: _listeners: N elements each: CustomInterest: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: EventListener: _listenersMutex: _owned: owned object: 4 bytes; PrecisionEventThread: EventListener: _listenersMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: _eventPolicyMutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: _lifecycleObserverHandle: owned object: 4 bytes]
 * Requires Stack/Heap Preallocation
 * Members:
 * - _setpoint (int): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 744 bytes [PrecisionEventThread: PrecisionThread: Thread: _taskExited: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _taskStartGate: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _taskConfigurationMutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _taskConfigurationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: Thread: _stateTransitionMutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _stateTransitionMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: Thread: _callbackMutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _callbackMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: Thread: _onDestroy: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onInitialize: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onStart: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onPause: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onTerminate: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onTerminated: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onInitializationFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onExecutionFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: Thread: _onStateChange: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: PrecisionThread: _iterationObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: _scheduleSignal: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: _timingMutex: _owned: owned object: 4 bytes; PrecisionEventThread: PrecisionThread: _timingMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: PrecisionThread: _iterationSamples: implementation blocks containing N * (8 bytes) plus block-map pointers; PrecisionEventThread: EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; PrecisionEventThread: EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: EventReceiver: _capacityAvailable: owned object: 4 bytes; PrecisionEventThread: EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; PrecisionEventThread: EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; PrecisionEventThread: EventListener: _listeners: implementation blocks containing N * (40 bytes) plus block-map pointers; PrecisionEventThread: EventListener: _listeners: N elements each: Callback: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: EventListener: _listeners: N elements each: CustomInterest: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionEventThread: EventListener: _listenersMutex: _owned: owned object: 4 bytes; PrecisionEventThread: EventListener: _listenersMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: _eventPolicyMutex: native synchronization state may allocate platform resources lazily; PrecisionEventThread: _lifecycleObserverHandle: owned object: 4 bytes]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class ControlThread final :
    public Event::PrecisionEventThread<> {

    private:
        int _setpoint = 0;

    protected:
        void OnIteration(
            IterationTime delta,
            IterationTime startTime,
            Threads::
                SkippedIterationCount
                    skippedIterations
        ) override {
            (void)delta;
            (void)startTime;

            Serial.printf(
                "control setpoint=%d skipped=%llu\n",
                _setpoint,
                static_cast<
                    unsigned long long
                >(
                    skippedIterations
                )
            );
        }

    public:
        ControlThread() :
            Event::PrecisionEventThread<>(
                Threads::ThreadReleasePolicy::ExplicitRelease
            ) {
        }

        void ApplySetpoint(
            SetpointEvent* event
        ) {
            _setpoint =
                event->GetSetpoint();
        }
};


ControlThread controlThread;

Event::EventListenerHandlePtr
    setpointListener;


void setup() {
    Serial.begin(115200);

    controlThread.
        SetIterationPeriod(
            Units::
                MilliSeconds<
                    uint64_t
                >(10)
        );

    controlThread.
        SetEventProcessOrder(
            Event::
                PrecisionEventProcessOrder::
                    EventsBeforeIteration
        );

    controlThread.
        SetEventArrivalPolicy(
            Event::
                PrecisionEventArrivalPolicy::
                    ProcessImmediately
        );

    setpointListener =
        controlThread.
            RegisterListener<
                SetpointEvent
            >(
                [](
                    SetpointEvent* event,
                    Event::
                        EventDispatchMethod,
                    Event::
                        EventPriority,
                    const Event::
                        EventDispatchContext&
                ) {
                    controlThread.
                        ApplySetpoint(
                            event
                        );
                }
            );

    Threads::ThreadManager::
        GetInstance()->
        Initialize();
}


void loop() {
    static int
        nextSetpoint = 1;

    (
        new SetpointEvent(
            nextSetpoint++
        )
    )->Queue();

    delay(1000);
}
