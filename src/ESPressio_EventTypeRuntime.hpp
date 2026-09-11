#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <ESPressio_IdleWorkerTask.hpp>
#include <ESPressio_SystemClock.hpp>
#include <ESPressio_RuntimeIdentity.hpp>
#include "ESPressio_EventBase.hpp"
#include "ESPressio_EventInstancePool.hpp"
#include "ESPressio_EventMessageSequence.hpp"
#include "ESPressio_EventTarget.hpp"
#include "ESPressio_EventResources.hpp"
#include "ESPressio_EventWire.hpp"
#include "ESPressio_EventRemoteReceipts.hpp"

namespace ESPressio::Event {
/// One process-lifetime pool, sequence and single-slot lane for one concrete
/// Type. The family Runtime owns bootstrap; no registry lookup occurs at Dispatch.
template<class T> class EventTypeRuntime final {
    static_assert(Detail::ValidateEventType<T>() && T::ValidateTier());
    enum class Phase : std::uint8_t { Uninitialized, Prepared, Running, Stopping };
    EventInstancePool<T,T::MaximumLiveInstances> _pool;
    Detail::EventMessageSequence<> _sequence;
    Task::IdleWorkerTask<EventLease> _lane;
    System::Synchronization::Mutex _admission, _topology;
    // The producer latch and target-progress latch are distinct: a sleeping
    // producer cannot consume the lane's target-capacity notification.
    std::unique_ptr<System::Synchronization::ISignal> _producerChanged, _targetChanged;
    std::atomic<Phase> _phase{Phase::Uninitialized};
    EventTargetNode* _targets = nullptr;
    bool _frozen = false;
    // Owner-side reservation is published only after T1's release hook runs
    // outside its slot lock. This prevents construction followed by a transient
    // T1 Busy while its prior descriptor is still being released.
    bool _laneAvailable = false, _laneExhausted = false;
    void* _runtimeOwner = nullptr;
    const std::atomic<bool>* _familyRunning = nullptr;
    Timing::QualifiedTime (*_captureTime)() = nullptr;
    EventTypeRuntime() noexcept = default;
    static Timing::QualifiedTime CaptureSystemTime() {
        return Timing::SystemClock<>::GetInstance().CaptureQualifiedTime();
    }
    void WakeProducers() noexcept { if (_producerChanged) (void)_producerChanged->Give(); }
    void WakeTargets() noexcept { if (_targetChanged) (void)_targetChanged->Give(); }
    void OnLaneReleased(Task::IdleWorkerTask<EventLease>&) noexcept {
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_admission);
            _laneAvailable = !_laneExhausted;
        }
        WakeProducers();
    }
    void ExecuteLane(EventLease& occurrence) noexcept {
        bool first = true;
        for (;;) {
            bool pending = false;
            {
                std::lock_guard<System::Synchronization::Mutex> lock(_topology);
                if (_phase != Phase::Running || (_familyRunning && !_familyRunning->load(std::memory_order_acquire))) return;
                for (auto* target = _targets; target; target = target->Next) {
                    if (first) {
                        target->Owed = true;
                        if constexpr (T::IsTransmissibleEvent) {
                            if (target->Class == EventTargetClass::ExternalAdapter &&
                                occurrence.template Get<T>().IsOriginRemote()) target->Owed = false;
                        }
                    }
                    if (!target->Owed) continue;
                    switch (target->TryAdmit(target->Owner, occurrence)) {
                        case EventTargetAdmission::Accepted:
                        case EventTargetAdmission::Quiesced: target->Owed = false; break;
                        case EventTargetAdmission::CapacityUnavailable: pending = true; break;
                    }
                }
            }
            if (!pending) return;
            first = false;
            // Every healthy target was attempted before waiting. A capacity or
            // quiesce publication always precedes Give; stale gives are harmless.
            (void)_targetChanged->Wait();
        }
    }
public:
    static EventTypeRuntime& Get() noexcept { static EventTypeRuntime instance; return instance; }
    EventTypeRuntime(const EventTypeRuntime&) = delete;
    EventTypeRuntime& operator=(const EventTypeRuntime&) = delete;
    /// Family bootstrap only; providers and coordinator outlive all references.
    EventRuntimeStatus Initialize(void* owner, Task::TaskExecutionConfiguration config,
                                  Timing::QualifiedTime (*capture)() = nullptr,
                                  const std::atomic<bool>* familyRunning = nullptr) {
        std::lock_guard<System::Synchronization::Mutex> lock(_admission);
        if (_phase != Phase::Uninitialized) return EventRuntimeStatus::AlreadyInitialized;
        if (!owner || !config.StackSize) return EventRuntimeStatus::InvalidConfiguration;
        if (!capture) capture = &CaptureSystemTime;
        try { (void)capture(); } // Resolve the qualified clock facade during bootstrap.
        catch (...) { return EventRuntimeStatus::StorageUnavailable; }
        auto* signals = System::Synchronization::Provider();
        if (!signals) return EventRuntimeStatus::StorageUnavailable;
        try {
            _producerChanged = signals->CreateBinarySignal(false);
            _targetChanged = signals->CreateBinarySignal(false);
        } catch (...) { _producerChanged.reset(); _targetChanged.reset(); return EventRuntimeStatus::StorageUnavailable; }
        if (!_producerChanged || !_targetChanged) {
            _producerChanged.reset(); _targetChanged.reset(); return EventRuntimeStatus::StorageUnavailable;
        }
        { std::lock_guard<System::Synchronization::Mutex> warm(_topology); }
        _pool.BindCapacityWake(this, [](void* p) noexcept { static_cast<EventTypeRuntime*>(p)->WakeProducers(); });
        const auto result = _lane.template Initialize<EventTypeRuntime,&EventTypeRuntime::ExecuteLane,
            &EventTypeRuntime::OnLaneReleased>(*this, config);
        if (result != Task::TaskExecutionStatus::Success) {
            _producerChanged.reset(); _targetChanged.reset(); return EventRuntimeStatus::TaskCreationFailed;
        }
        _captureTime = capture; _runtimeOwner = owner; _familyRunning = familyRunning; _laneAvailable = true;
        _phase = Phase::Prepared;
        return EventRuntimeStatus::Success;
    }
    EventRuntimeStatus StageTarget(EventTargetNode& target) noexcept {
        std::lock_guard<System::Synchronization::Mutex> lock(_topology);
        if (_frozen) return EventRuntimeStatus::Frozen;
        if (_phase != Phase::Prepared || target.Runtime.load() || !target.Owner || !target.TryAdmit || !target.Validate || !target.Quiesce)
            return EventRuntimeStatus::InvalidTopology;
        if (target.Class == EventTargetClass::ExternalAdapter && !T::IsTransmissibleEvent)
            return EventRuntimeStatus::InvalidTopology;
        target.CapacityChanged = [](void* p) noexcept { static_cast<EventTypeRuntime*>(p)->WakeTargets(); };
        target.Runtime.store(this,std::memory_order_release);
        target.Next = _targets; _targets = &target;
        return EventRuntimeStatus::Success;
    }
    /// Teardown removes a node under the same gate used by lane traversal, so its
    /// owner can be destroyed afterward without leaving a dangling frozen link.
    void RemoveTarget(EventTargetNode& target) noexcept {
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_topology);
            auto** link = &_targets;
            while (*link && *link != &target) link = &(*link)->Next;
            if (*link) *link = target.Next;
            target.Next = nullptr; target.Runtime.store(nullptr,std::memory_order_release); target.Owed = false;
        }
        WakeTargets();
    }
    bool ValidateStart() noexcept {
        std::lock_guard<System::Synchronization::Mutex> lock(_topology);
        if (_phase != Phase::Prepared) return false;
        for (auto* target = _targets; target; target = target->Next) if (!target->Validate(target->Owner)) return false;
        return true;
    }
    /// The coordinator invokes this only after every Type validates successfully.
    void StartValidated() noexcept {
        std::lock_guard<System::Synchronization::Mutex> lock(_admission);
        { std::lock_guard<System::Synchronization::Mutex> topology(_topology); _frozen = true; }
        _phase = Phase::Running;
    }
    void CloseAdmissions() noexcept {
        { std::lock_guard<System::Synchronization::Mutex> lock(_admission); if (_phase != Phase::Uninitialized) _phase = Phase::Stopping; }
        WakeProducers(); WakeTargets();
    }
    EventRuntimeStatus Shutdown() noexcept {
        CloseAdmissions();
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_topology);
            for (auto* target = _targets; target; target = target->Next) target->Quiesce(target->Owner);
        }
        if (_lane.Shutdown() != Task::TaskExecutionStatus::Success) return EventRuntimeStatus::JoinFailed;
        // Pool releases and blocked producers may still publish wakes. The two
        // fixed latches stay alive for the process lifetime once initialized.
        return EventRuntimeStatus::Success;
    }
    EventRuntimeStatus RollbackInitialization() noexcept {
        std::lock_guard<System::Synchronization::Mutex> lock(_admission);
        if (_phase != Phase::Prepared || _targets || _pool.Occupied()) return EventRuntimeStatus::InvalidTopology;
        if (_lane.Shutdown() != Task::TaskExecutionStatus::Success) return EventRuntimeStatus::JoinFailed;
        _phase=Phase::Uninitialized; _runtimeOwner=nullptr; _familyRunning=nullptr;
        _captureTime=nullptr; _laneAvailable=false;
        _producerChanged.reset(); _targetChanged.reset();
        return EventRuntimeStatus::Success;
    }
    template<bool Blocking, class... Args> EventDispatchResult Dispatch(Args&&... args) {
        const auto phase = _phase.load(std::memory_order_acquire);
        if (phase != Phase::Running) return {phase == Phase::Stopping ? EventDispatchStatus::Stopping : EventDispatchStatus::NotInitialized,{}};
        if (_familyRunning && !_familyRunning->load(std::memory_order_acquire)) return {EventDispatchStatus::NotInitialized,{}};
        const auto time = _captureTime(); // Immutable origin observation before any wait.
        if constexpr (T::IsTransmissibleEvent)
            if (!System::RuntimeIdentity::IsInstalled()) return {EventDispatchStatus::IdentityUnavailable,{}};
        for (;;) {
            std::unique_lock<System::Synchronization::Mutex> lock(_admission,std::defer_lock);
            if constexpr (Blocking) lock.lock();
            else if (!lock.try_lock()) return {EventDispatchStatus::CapacityUnavailable,{}};
            if (_phase != Phase::Running || (_familyRunning && !_familyRunning->load(std::memory_order_acquire))) {
                lock.unlock(); WakeProducers(); // Stop baton for every blocked producer.
                return {EventDispatchStatus::Stopping,{}};
            }
            if (_sequence.IsExhausted() || _laneExhausted) {
                lock.unlock(); WakeProducers();
                return {EventDispatchStatus::IdentifierExhausted,{}};
            }
            if (_laneAvailable) {
                auto reservation = _pool.TryReserve();
                if (reservation) {
                    _laneAvailable = false;
                    ConceptualMessageId id;
                    (void)_sequence.TryIssue(id);
                    try {
                        auto lease = reservation.Construct({T::TypeId,id,time},std::forward<Args>(args)...);
                        if constexpr (T::IsTransmissibleEvent)
                            const_cast<T&>(lease.template Get<T>())._origin=*System::RuntimeIdentity::TryGet();
                        const auto admitted = _lane.TryAssign(std::move(lease));
                        // This is the sole assigner; owner-side availability comes
                        // from T1's post-release hook, never an IsIdle snapshot.
                        if (admitted.Status==Task::TaskExecutionStatus::GenerationExhausted) {
                            _laneExhausted=true; lock.unlock(); WakeProducers();
                            return {EventDispatchStatus::IdentifierExhausted,{}};
                        }
                        if (!admitted) std::terminate();
                    } catch (...) { _laneAvailable = true; lock.unlock(); WakeProducers(); throw; }
                    lock.unlock();
                    WakeProducers(); // Baton lets other sleepers observe stop/exhaustion.
                    return {EventDispatchStatus::Accepted,id};
                }
            }
            if constexpr (!Blocking) return {EventDispatchStatus::CapacityUnavailable,{}};
            lock.unlock();
            (void)_producerChanged->Wait();
        }
    }
    /// Called only after the family validates the complete header/binding and
    /// owns a provisional receipt. No consumer-inbox wait or application callback.
    template<class Format> EventRemoteAdmissionResult TryAdmitRemote(
        const EventWireHeader& header,const std::uint8_t* payload,std::size_t size) noexcept {
        static_assert(T::IsTransmissibleEvent && T::ValidateTier());
        std::unique_lock<System::Synchronization::Mutex> lock(_admission,std::try_to_lock);
        if(lock.owns_lock() && _laneExhausted) return {EventRemoteAdmissionStatus::ResourceUnavailable};
        if(!lock.owns_lock() || _phase!=Phase::Running ||
           (_familyRunning && !_familyRunning->load(std::memory_order_acquire)) || !_laneAvailable)
            return {EventRemoteAdmissionStatus::TemporarilyUnavailable};
        if(header.Key.TypeId!=T::TypeId || size>Serializable::MaximumSerializedSize<T,Format>)
            return {EventRemoteAdmissionStatus::Invalid};
        auto reservation=_pool.TryReserve();
        if(!reservation) return {EventRemoteAdmissionStatus::TemporarilyUnavailable};
        _laneAvailable=false;
        try {
            T candidate{};
            Serializable::BoundedSerializationResult decoded;
            if constexpr(std::is_same_v<Format,Serializable::DirectBinary>)
                decoded=Serializable::DeserializeBoundedDirectBinary(payload,size,candidate);
            else if constexpr(std::is_same_v<Format,Serializable::CBOR>)
                decoded=Serializable::DeserializeBoundedCbor(payload,size,candidate);
            else {
                static_assert(std::is_same_v<Format,Serializable::JSON>);
                decoded=Serializable::DeserializeBoundedJson(payload,size,candidate);
            }
            if(!decoded) { _laneAvailable=true; return {EventRemoteAdmissionStatus::SchemaOrDecodeFailure}; }
            auto lease=reservation.Construct({T::TypeId,header.Key.MessageId,header.OriginDispatchTime},std::move(candidate));
            const_cast<T&>(lease.template Get<T>())._origin=header.Key.Origin;
            const auto admitted=_lane.TryAssign(std::move(lease));
            if(admitted.Status==Task::TaskExecutionStatus::GenerationExhausted) {
                _laneExhausted=true; lock.unlock(); WakeProducers();
                return {EventRemoteAdmissionStatus::ResourceUnavailable};
            }
            if(!admitted) std::terminate();
            return {EventRemoteAdmissionStatus::Admitted};
        } catch(...) { _laneAvailable=true; return {EventRemoteAdmissionStatus::SchemaOrDecodeFailure}; }
    }
    static constexpr EventTypeResourceProfile StaticResources() noexcept {
        return {sizeof(EventTypeRuntime),alignof(EventTypeRuntime),sizeof(EventInstancePool<T,T::MaximumLiveInstances>),
            EventInstancePool<T,T::MaximumLiveInstances>::SlotBytes,T::MaximumLiveInstances,sizeof(EventTargetNode)};
    }
    EventTypeResourceProfile GetResourceProfile() const noexcept {
        auto profile=StaticResources();profile.LaneStackBytes=_lane.GetStatistics().ConfiguredStackSize;return profile;
    }
    std::size_t LiveInstances() const noexcept { return _pool.Occupied(); }
    ConceptualMessageId MessageHighWater() noexcept {
        std::lock_guard<System::Synchronization::Mutex> lock(_admission); return _sequence.HighWater();
    }
};
}
