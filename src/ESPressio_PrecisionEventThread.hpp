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
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
enum
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 1 bytes [0 bytes dynamic allocation]
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class PrecisionEventProcessOrder : uint8_t {
    EventsBeforeIteration,
    EventsAfterIteration
};

/**
 * ESPressio Memory Audit
 * Underlying storage: 1 bytes
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
enum
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 1 bytes [0 bytes dynamic allocation]
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class PrecisionEventArrivalPolicy : uint8_t {
    ProcessOnNextIteration,
    TriggerImmediateIteration,
    ProcessImmediately
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: sizeof(Threads::PrecisionThread<TTime, TRepresentationTraits>) + 4 bytes known bases + 59 bytes known members + sizeof(EventCollection) + sizeof(EventCollection) + 4 bytes known bases + 5 bytes known members + sizeof(ListenerStorage) + sizeof(System::Synchronization::RecursiveMutex) [0 bytes dynamic allocation]
 * Requires Stack/Heap Preallocation
 * Members:
 * - _eventPolicyMutex (std::mutex): sizeof(std::mutex) [0 bytes dynamic allocation]
 * - _eventProcessOrder (PrecisionEventProcessOrder): 1 bytes [0 bytes dynamic allocation]
 * - _eventArrivalPolicy (PrecisionEventArrivalPolicy): 1 bytes [0 bytes dynamic allocation]
 * - _lifecycleObserverHandle (Observable::ObserverHandlePtr): sizeof(Observable::ObserverHandlePtr) [0 bytes dynamic allocation]
 * Total Memory: sizeof(Threads::PrecisionThread<TTime, TRepresentationTraits>) + 4 bytes known bases + 59 bytes known members + sizeof(EventCollection) + sizeof(EventCollection) + 4 bytes known bases + 5 bytes known members + sizeof(ListenerStorage) + sizeof(System::Synchronization::RecursiveMutex) + 2 bytes known members + sizeof(std::mutex) + sizeof(Observable::ObserverHandlePtr) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
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
 * Inherited Memory Total: sizeof(Threads::IThreadObserver) [0 bytes dynamic allocation]
 * Members:
 * - _owner (PrecisionEventThread*): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: sizeof(Threads::IThreadObserver) + 4 bytes known members [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
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
