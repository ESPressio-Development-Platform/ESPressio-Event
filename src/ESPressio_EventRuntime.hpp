#pragma once
#include <ESPressio_TypeDirectory.hpp>
#include "ESPressio_EventTypeDescriptor.hpp"
namespace ESPressio::Event {
struct RuntimeConfiguration final {
    Task::TaskExecutionConfiguration DispatchLane{};
    RemoteAdmissionReceipt* RemoteAdmissionReceipts=nullptr;
    std::size_t RemoteAdmissionReceiptCapacity=0;
};
class Runtime;
/// Immutable typed inbound binding prepared before family Start. No selected
/// format or transport pointer becomes an occurrence fact.
class EventInboundBinding final {
    friend class Runtime;
    const Runtime* _runtime=nullptr;
    const EventTypeDescriptor* _type=nullptr;
    EventPayloadFormat _format=EventPayloadFormat::DirectBinary;
public:
    explicit operator bool() const noexcept { return _runtime && _type; }
};
/// Caller-owned family bootstrap over a frozen P1 view. It is not a dispatch
/// registry. All Types are prepared/frozen before one atomic admission publish.
class Runtime final {
    RuntimeConfiguration _configuration;
    Primitive::TypeDirectoryView _directory;
    std::atomic<bool> _running{false};
    bool _initialized=false,_stopping=false;
    std::size_t _preparedTypes=0;
    EventRemoteReceiptTable _receipts;
    System::Synchronization::Mutex _remoteGate;
    std::unique_ptr<System::Synchronization::ISignal> _remoteDrained;
    std::size_t _activeRemote=0;
    const System::Clock::IMonotonicClock* _monotonic=nullptr;
    struct RemoteCall final {
        Runtime* Owner=nullptr;
        ~RemoteCall() {
            if(!Owner) return;
            std::lock_guard<System::Synchronization::Mutex> lock(Owner->_remoteGate);
            if(--Owner->_activeRemote==0) (void)Owner->_remoteDrained->Give();
        }
    };
public:
    explicit Runtime(RuntimeConfiguration configuration={}) noexcept : _configuration(configuration) {}
    Runtime(const Runtime&)=delete;
    Runtime& operator=(const Runtime&)=delete;
    ~Runtime() { if(Shutdown()!=EventRuntimeStatus::Success) std::terminate(); }
    EventRuntimeStatus Initialize(Primitive::TypeDirectoryView directory) {
        if(_initialized) return EventRuntimeStatus::AlreadyInitialized;
        if(_stopping) return EventRuntimeStatus::Stopping;
        if(!directory.IsFrozen()) return EventRuntimeStatus::InvalidDirectory;
        // Validate the complete immutable extension set before creating any task.
        for(const auto& common:directory) if(common.Key.Family==EventFamilyId) {
            const auto* descriptor=GetEventTypeDescriptor(common);
            if(!descriptor || !descriptor->Initialize || !descriptor->ValidateStart || !descriptor->StartValidated ||
               !descriptor->CloseAdmissions || !descriptor->Shutdown || !descriptor->RollbackInitialization)
                return EventRuntimeStatus::InvalidDirectory;
        }
        if(_configuration.RemoteAdmissionReceiptCapacity>SIZE_MAX/sizeof(RemoteAdmissionReceipt))
            return EventRuntimeStatus::InvalidConfiguration;
        if(!_receipts.Initialize(_configuration.RemoteAdmissionReceipts,_configuration.RemoteAdmissionReceiptCapacity))
            return EventRuntimeStatus::InvalidConfiguration;
        { std::lock_guard<System::Synchronization::Mutex> warm(_remoteGate); }
        _monotonic=&System::Clock::Monotonic();
        if(_configuration.RemoteAdmissionReceiptCapacity && !_remoteDrained) {
            auto* provider=System::Synchronization::Provider();
            if(!provider) return EventRuntimeStatus::StorageUnavailable;
            try { _remoteDrained=provider->CreateBinarySignal(false); } catch(...) { return EventRuntimeStatus::StorageUnavailable; }
            if(!_remoteDrained) return EventRuntimeStatus::StorageUnavailable;
        }
        _directory=directory; _preparedTypes=0;
        for(const auto& common:directory) if(common.Key.Family==EventFamilyId) {
            const auto* descriptor=GetEventTypeDescriptor(common);
            const auto status=descriptor->Initialize(this,_configuration.DispatchLane,&_running);
            if(status!=EventRuntimeStatus::Success) {
                for(const auto& earlier:directory) {
                    if(&earlier==&common) break;
                    if(earlier.Key.Family==EventFamilyId &&
                       GetEventTypeDescriptor(earlier)->RollbackInitialization()!=EventRuntimeStatus::Success)
                    { _initialized=true; _stopping=true; return EventRuntimeStatus::JoinFailed; }
                }
                _preparedTypes=0; return status;
            }
            ++_preparedTypes;
        }
        _directory=directory; _initialized=true;
        return EventRuntimeStatus::Success;
    }
    EventRuntimeStatus Start() noexcept {
        if(!_initialized) return EventRuntimeStatus::NotInitialized;
        if(_stopping) return EventRuntimeStatus::Stopping;
        if(_running) return EventRuntimeStatus::Frozen;
        for(const auto& common:_directory) if(common.Key.Family==EventFamilyId)
            if(!GetEventTypeDescriptor(common)->ValidateStart()) return EventRuntimeStatus::InvalidTopology;
        for(const auto& common:_directory) if(common.Key.Family==EventFamilyId)
            GetEventTypeDescriptor(common)->StartValidated();
        _running.store(true,std::memory_order_release);
        return EventRuntimeStatus::Success;
    }
    EventRuntimeStatus Shutdown() noexcept {
        if(!_initialized) return EventRuntimeStatus::Success;
        _running.store(false,std::memory_order_release); _stopping=true;
        for(;;) {
            { std::lock_guard<System::Synchronization::Mutex> lock(_remoteGate); if(_activeRemote==0) break; }
            (void)_remoteDrained->Wait();
        }
        std::size_t visited=0;
        for(const auto& common:_directory) if(common.Key.Family==EventFamilyId) {
            if(visited++==_preparedTypes) break;
            GetEventTypeDescriptor(common)->CloseAdmissions();
        }
        bool joined=true; visited=0;
        for(const auto& common:_directory) if(common.Key.Family==EventFamilyId) {
            if(visited++==_preparedTypes) break;
            if(GetEventTypeDescriptor(common)->Shutdown()!=EventRuntimeStatus::Success) joined=false;
        }
        if(!joined) return EventRuntimeStatus::JoinFailed;
        _initialized=false;
        return EventRuntimeStatus::Success;
    }
    template<class T,class Format> EventInboundBinding BindInbound() const noexcept {
        static_assert(T::IsTransmissibleEvent && T::ValidateTier());
        EventInboundBinding binding;
        if(!_initialized || _running || _stopping || !_configuration.RemoteAdmissionReceiptCapacity) return binding;
        const auto* common=_directory.Find({EventFamilyId,T::TypeId.Value()});
        if(!common) return binding;
        binding._type=GetEventTypeDescriptor(*common);
        if(!binding._type || !binding._type->AdmitRemote) return {};
        if constexpr(std::is_same_v<Format,Serializable::DirectBinary>) binding._format=EventPayloadFormat::DirectBinary;
        else if constexpr(std::is_same_v<Format,Serializable::CBOR>) binding._format=EventPayloadFormat::CBOR;
        else { static_assert(std::is_same_v<Format,Serializable::JSON>); binding._format=EventPayloadFormat::JSON; }
        binding._runtime=this; return binding;
    }
    EventRemoteAdmissionResult TryAdmitRemote(const EventInboundBinding& binding,const std::uint8_t* data,std::size_t size) noexcept {
        if(binding._runtime!=this || !binding._type) return {EventRemoteAdmissionStatus::UnknownType};
        RemoteCall call;
        {
            std::unique_lock<System::Synchronization::Mutex> lock(_remoteGate,std::try_to_lock);
            if(!lock.owns_lock() || !_running || _activeRemote==_configuration.RemoteAdmissionReceiptCapacity)
                return {EventRemoteAdmissionStatus::TemporarilyUnavailable};
            ++_activeRemote; call.Owner=this;
        }
        EventWireHeader header;
        const auto parsed=DecodeEventWireHeader(data,size,header);
        if(!parsed) return {parsed.Status==EventWireStatus::UnsupportedProtocol ?
            EventRemoteAdmissionStatus::UnsupportedProtocol : EventRemoteAdmissionStatus::Invalid};
        if(header.Key.TypeId!=binding._type->TypeId) return {EventRemoteAdmissionStatus::UnknownType};
        if(size>binding._type->MaximumCompleteWireBytes[static_cast<std::size_t>(binding._format)])
            return {EventRemoteAdmissionStatus::Invalid};
        // A source's own device occurrence must never be replayed through remote
        // admission and thereby acquire a second local fanout/outbound campaign.
        if(const auto* local=System::RuntimeIdentity::TryGet(); local && header.Key.Origin.Device==local->Device)
            return {EventRemoteAdmissionStatus::Rejected};
        auto receipt=_receipts.TryReserve(header.Key,_monotonic->NowNanoseconds());
        if(!receipt) return {receipt.Status};
        const auto admitted=binding._type->AdmitRemote(binding._format,header,data+EventWireHeaderSize,header.PayloadLength);
        if(admitted.Status!=EventRemoteAdmissionStatus::Admitted) return admitted;
        receipt.Commit(_monotonic->NowNanoseconds(),binding._type->DeliveryPolicy->MaximumResidenceNanoseconds);
        return admitted; // Only now may the adapter emit destination-admission evidence.
    }
    EventRuntimeResourceProfile GetResourceProfile() const noexcept {
        EventRuntimeResourceProfile profile;
        profile.CoordinatorBytes=sizeof(Runtime);profile.CoordinatorAlignment=alignof(Runtime);
        profile.ReceiptCapacity=_configuration.RemoteAdmissionReceiptCapacity;
        profile.ReceiptBytes=profile.ReceiptCapacity*sizeof(RemoteAdmissionReceipt);
        profile.ReceiptAlignment=alignof(RemoteAdmissionReceipt);
        profile.StackBytesPerType=_configuration.DispatchLane.StackSize;
        profile.Signals=profile.ReceiptCapacity ? 1 : 0;
        for(const auto& common:_directory) if(common.Key.Family==EventFamilyId) {
            const auto* descriptor=GetEventTypeDescriptor(common);
            ++profile.TypeCount;profile.TypeResidentBytes+=descriptor->Resources.ResidentBytes;
            profile.ExecutionContexts+=descriptor->Resources.ExecutionContexts;
            profile.Signals+=descriptor->Resources.Signals;
        }
        return profile;
    }
    bool IsRunning() const noexcept { return _running.load(std::memory_order_acquire); }
};
}
