#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

#include <ESPressio_PrecisionThread.hpp>

#include "ESPressio_EventListener.hpp"
#include "ESPressio_EventManager.hpp"
#include "ESPressio_EventReceiver.hpp"
#include "ESPressio_EventThread.hpp"

namespace ESPressio {
namespace Event {

/**
 * ESPressio Memory Audit
 * Underlying storage: 1 bytes
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
enum
class PrecisionEventProcessOrder : uint8_t {
    EventsBeforeIteration,
    EventsAfterIteration
};

/**
 * ESPressio Memory Audit
 * Underlying storage: 1 bytes
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
enum
class PrecisionEventArrivalPolicy : uint8_t {
    ProcessOnNextIteration,
    TriggerImmediateIteration,
    ProcessImmediately
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 660 bytes [PrecisionThread: Thread: _taskExited: owned object: 4 bytes; PrecisionThread: Thread: _taskStartGate: owned object: 4 bytes; PrecisionThread: Thread: _taskConfigurationMutex: _owned: owned object: 4 bytes; PrecisionThread: Thread: _taskConfigurationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: Thread: _stateTransitionMutex: _owned: owned object: 4 bytes; PrecisionThread: Thread: _stateTransitionMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: Thread: _lifecycleObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: Thread: _callbackMutex: _owned: owned object: 4 bytes; PrecisionThread: Thread: _callbackMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: Thread: _onDestroy: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onInitialize: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onStart: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onPause: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onTerminate: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onTerminated: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onInitializationFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onExecutionFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onStateChange: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: _iterationObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: _scheduleSignal: owned object: 4 bytes; PrecisionThread: _timingMutex: _owned: owned object: 4 bytes; PrecisionThread: _timingMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: _iterationSamples: implementation blocks containing N * (8 bytes) plus block-map pointers; EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventReceiver: _capacityAvailable: owned object: 4 bytes; EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; EventListener: _listeners: implementation blocks containing N * (40 bytes) plus block-map pointers; EventListener: _listeners: N elements each: Callback: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventListener: _listeners: N elements each: CustomInterest: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventListener: _listenersMutex: _owned: owned object: 4 bytes; EventListener: _listenersMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Requires Stack/Heap Preallocation
 * Members:
 * - _eventPolicyMutex (std::mutex): 4 bytes [native synchronization state may allocate platform resources lazily]
 * - _eventProcessOrder (PrecisionEventProcessOrder): 1 bytes [0 bytes dynamic allocation]
 * - _eventArrivalPolicy (PrecisionEventArrivalPolicy): 1 bytes [0 bytes dynamic allocation]
 * - _acceptingEvents (std::atomic<bool>): 1 bytes [0 bytes dynamic allocation]
 * - _lifecycleObserver (LifecycleObserver): 60 bytes [0 bytes dynamic allocation]
 * - _lifecycleObserverHandle (Observable::ObserverHandlePtr): 12 bytes [owned object: 4 bytes]
 * Total Memory: 740 bytes [PrecisionThread: Thread: _taskExited: owned object: 4 bytes; PrecisionThread: Thread: _taskStartGate: owned object: 4 bytes; PrecisionThread: Thread: _taskConfigurationMutex: _owned: owned object: 4 bytes; PrecisionThread: Thread: _taskConfigurationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: Thread: _stateTransitionMutex: _owned: owned object: 4 bytes; PrecisionThread: Thread: _stateTransitionMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: Thread: _lifecycleObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; PrecisionThread: Thread: _lifecycleObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: Thread: _callbackMutex: _owned: owned object: 4 bytes; PrecisionThread: Thread: _callbackMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: Thread: _onDestroy: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onInitialize: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onStart: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onPause: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onTerminate: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onTerminated: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onInitializationFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onExecutionFailed: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: Thread: _onStateChange: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; PrecisionThread: _iterationObservable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; PrecisionThread: _iterationObservable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: _scheduleSignal: owned object: 4 bytes; PrecisionThread: _timingMutex: _owned: owned object: 4 bytes; PrecisionThread: _timingMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; PrecisionThread: _iterationSamples: implementation blocks containing N * (8 bytes) plus block-map pointers; EventReceiver: _eventsMutex: _owned: owned object: 4 bytes; EventReceiver: _eventsMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; EventReceiver: _capacityAvailable: owned object: 4 bytes; EventReceiver: _priorityQueues: 3 elements each: Capacity * (16 bytes) element storage; EventReceiver: _priorityStacks: 3 elements each: Capacity * (16 bytes) element storage; EventListener: _listeners: implementation blocks containing N * (40 bytes) plus block-map pointers; EventListener: _listeners: N elements each: Callback: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventListener: _listeners: N elements each: CustomInterest: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 4 bytes; EventListener: _listenersMutex: _owned: owned object: 4 bytes; EventListener: _listenersMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _eventPolicyMutex: native synchronization state may allocate platform resources lazily; _lifecycleObserverHandle: owned object: 4 bytes]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
template<
    typename TTime = Timing::DefaultClockTime,
    typename TRepresentationTraits = Threads::PrecisionThreadTraits<TTime>
>
class PrecisionEventThread :
    public Threads::PrecisionThread<TTime, TRepresentationTraits>,
    public EventReceiver,
    public IEventThreadBase,
    public EventListener,
    public IEventThread {
private:
    using PrecisionThreadBase =
        Threads::PrecisionThread<TTime, TRepresentationTraits>;

    mutable std::mutex _eventPolicyMutex;
    PrecisionEventProcessOrder _eventProcessOrder =
        PrecisionEventProcessOrder::EventsBeforeIteration;
    PrecisionEventArrivalPolicy _eventArrivalPolicy =
        PrecisionEventArrivalPolicy::ProcessOnNextIteration;
    std::atomic<bool> _acceptingEvents{true};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members:
 * - _owner (PrecisionEventThread*): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 8 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class LifecycleObserver final : public Threads::IThreadObserver {
    private:
        PrecisionEventThread* _owner;
    public:
        explicit LifecycleObserver(PrecisionEventThread* owner)
            : _owner(owner) {}

        void OnThreadExecutionFailed(
            Threads::IThread*,
            std::exception_ptr
        ) override {
            _owner->StopReceivingEvents();
        }

        void OnThreadTerminated(Threads::IThread*) override {
            _owner->StopReceivingEvents();
        }
    };

    LifecycleObserver _lifecycleObserver{this};
    Observable::ObserverHandlePtr _lifecycleObserverHandle;

    void StopReceivingEvents() noexcept {
        if (!_acceptingEvents.exchange(false)) return;
        StopAcceptingEvents();
        try { UnregisterAllListeners(); } catch (...) {}
        ClearPendingEvents();
    }

    void ProcessPendingEvents() {
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

protected:
    using typename PrecisionThreadBase::IterationTime;
    using typename PrecisionThreadBase::TimeType;
    using typename PrecisionThreadBase::IterationFrequency;
    using typename PrecisionThreadBase::SignedIterationTime;

    void Iterate(
        IterationTime delta,
        IterationTime startTime,
        Threads::SkippedIterationCount skippedIterations
    ) final override {
        try {
            PrecisionEventProcessOrder processOrder;
            {
                std::lock_guard<std::mutex> lock(_eventPolicyMutex);
                processOrder = _eventProcessOrder;
            }

            if (processOrder == PrecisionEventProcessOrder::EventsBeforeIteration) {
                ProcessPendingEvents();
            }

            OnIteration(delta, startTime, skippedIterations);

            if (processOrder == PrecisionEventProcessOrder::EventsAfterIteration) {
                ProcessPendingEvents();
            }
        } catch (...) {
            StopReceivingEvents();
            throw;
        }
    }

    virtual void OnIteration(
        IterationTime delta,
        IterationTime startTime,
        Threads::SkippedIterationCount skippedIterations
    ) = 0;

    void OnWorkWake() final override {
        try {
            ProcessPendingEvents();
        } catch (...) {
            StopReceivingEvents();
            throw;
        }
    }

    void EventAdded() override {
        PrecisionEventArrivalPolicy arrivalPolicy;
        {
            std::lock_guard<std::mutex> lock(_eventPolicyMutex);
            arrivalPolicy = _eventArrivalPolicy;
        }

        if (arrivalPolicy ==
            PrecisionEventArrivalPolicy::TriggerImmediateIteration) {
            this->Bump();
        } else if (arrivalPolicy ==
            PrecisionEventArrivalPolicy::ProcessImmediately) {
            this->WakeForWork();
        }
    }

    void OnListenerRegistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->RegisterReceiver(eventType, this);
    }

    void OnListenerUnregistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->UnregisterReceiver(eventType, this);
    }

public:
    using ClockType = typename PrecisionThreadBase::ClockType;

    explicit PrecisionEventThread(
        Threads::ThreadReleasePolicy releasePolicy,
        ClockType* clock = nullptr
    ) : PrecisionThreadBase(releasePolicy, clock),
        _lifecycleObserverHandle(
            this->RegisterThreadObserver(&_lifecycleObserver)
        ) {}

    ~PrecisionEventThread() override {
        this->Shutdown();
        StopReceivingEvents();
    }

    void Terminate() override {
        StopReceivingEvents();
        PrecisionThreadBase::Terminate();
    }

    PrecisionEventProcessOrder GetEventProcessOrder() const {
        std::lock_guard<std::mutex> lock(_eventPolicyMutex);
        return _eventProcessOrder;
    }

    void SetEventProcessOrder(PrecisionEventProcessOrder processOrder) {
        std::lock_guard<std::mutex> lock(_eventPolicyMutex);
        _eventProcessOrder = processOrder;
    }

    PrecisionEventArrivalPolicy GetEventArrivalPolicy() const {
        std::lock_guard<std::mutex> lock(_eventPolicyMutex);
        return _eventArrivalPolicy;
    }

    void SetEventArrivalPolicy(PrecisionEventArrivalPolicy arrivalPolicy) {
        std::lock_guard<std::mutex> lock(_eventPolicyMutex);
        _eventArrivalPolicy = arrivalPolicy;
    }
};

} // namespace Event
} // namespace ESPressio
