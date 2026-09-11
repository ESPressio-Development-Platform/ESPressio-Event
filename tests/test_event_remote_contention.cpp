#include <ESPressio_Event.hpp>
#include <HostRuntime.hpp>
#include <cassert>
using namespace ESPressio;
namespace E=ESPressio::Event;
struct Policy {
    using PolicyCategory=Primitive::OccurrenceDeliveryPolicyTag;
    using RequiredEvidence=Primitive::DestinationPrimitiveAdmission;
    using TerminalDisposition=Primitive::DiagnosticOnlyAfterBudget;
    static constexpr std::uint64_t MaximumResidenceNanoseconds=10000000000ULL,MaximumAdapterAdmissionWaitNanoseconds=1000;
    static constexpr std::uint16_t MaximumAttempts=2;
    static constexpr std::uint64_t MinimumRetrySpacingNanoseconds=1000,MaximumRetrySpacingNanoseconds=10000;
};
std::atomic<bool> blockConstruction{false},entered{false},releaseConstruction{false};
std::atomic<unsigned> constructions{0};
struct Message final:E::TransmissibleEvent<Message> {
    static constexpr E::EventTypeId TypeId{88};
    static constexpr std::string_view CanonicalName="test.contention";
    static constexpr std::size_t MaximumLiveInstances=2,MaximumPendingInstances=1;
    using DeliveryPolicy=Policy;
    std::uint32_t Value=0;
    Message() {++constructions;if(blockConstruction){entered=true;while(!releaseConstruction) std::this_thread::yield();}}
    ESPRESSIO_SERIALIZABLE_TYPE(Message)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("value",Value))
};
template<class P> void Await(P p){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(3);while(!p()){assert(std::chrono::steady_clock::now()<end);std::this_thread::yield();}}
struct Target {
    E::EventTargetNode Node;
    std::atomic<bool> Blocked{true};
    std::atomic<unsigned> Calls{0};
    std::array<E::EventLease,3> Held;
    Target(){Node.Owner=this;Node.Validate=[](void*)noexcept{return true;};Node.Quiesce=[](void*)noexcept{};
        Node.TryAdmit=[](void* p,const E::EventLease& lease)noexcept{
            auto& t=*static_cast<Target*>(p);if(t.Blocked)return E::EventTargetAdmission::CapacityUnavailable;
            const auto& m=lease.Get<Message>();assert(m.IsOriginRemote());
            assert(m.GetOriginDispatchTime().Nanoseconds==123 && m.GetOriginDeviceRuntimeIdentity().Incarnation.Value()==7);
            t.Held[t.Calls.load()]=lease.Retain();++t.Calls;return E::EventTargetAdmission::Accepted;
        };}
};
int main(){
    HostRuntime host;
    Primitive::TypeDirectory<1> directory;assert(directory.Register<Message>()==Primitive::TypeDirectoryRegistrationStatus::Success);
    assert(directory.Initialize()==Primitive::TypeDirectoryInitializationStatus::Success);
    std::array<E::RemoteAdmissionReceipt,8> receipts;
    E::Runtime runtime({{},receipts.data(),receipts.size()});assert(runtime.Initialize(directory.View())==E::EventRuntimeStatus::Success);
    auto inbound=runtime.BindInbound<Message,Serializable::DirectBinary>();
    auto& type=E::EventTypeRuntime<Message>::Get();Target target;assert(type.StageTarget(target.Node)==E::EventRuntimeStatus::Success);
    assert(runtime.Start()==E::EventRuntimeStatus::Success);
    Message payload;payload.Value=91;
    std::array<std::uint8_t,E::MaximumCompletePrimitiveWireBytes<Message,Serializable::DirectBinary>> bytes{};
    auto encoded=Serializable::SerializeDirectBinary(payload,bytes.data()+53,bytes.size()-53);assert(encoded);
    System::DeviceIdentifier::Storage device{};device[0]=9;
    E::EventWireHeader header{{Message::TypeId,{System::DeviceIdentifier{device},System::RuntimeIncarnationId{7}},E::ConceptualMessageId{1}},
        {123,Timing::TimeReliability::Acquiring},static_cast<std::uint32_t>(encoded.Bytes)};
    assert(E::EncodeEventWireHeader(header,bytes.data(),bytes.size()));const auto size=53+encoded.Bytes;
    const auto before=constructions.load();blockConstruction=true;
    E::EventRemoteAdmissionResult first;
    std::thread worker([&]{first=runtime.TryAdmitRemote(inbound,bytes.data(),size);});
    Await([]{return entered.load();});
    assert(runtime.TryAdmitRemote(inbound,bytes.data(),size).Status==E::EventRemoteAdmissionStatus::TemporarilyUnavailable);
    assert(constructions==before+1);
    blockConstruction=false;releaseConstruction=true;worker.join();assert(first.Status==E::EventRemoteAdmissionStatus::Admitted);
    assert(runtime.TryAdmitRemote(inbound,bytes.data(),size).Status==E::EventRemoteAdmissionStatus::AlreadyAdmitted);
    header.Key.MessageId=E::ConceptualMessageId{2};assert(E::EncodeEventWireHeader(header,bytes.data(),bytes.size()));
    assert(runtime.TryAdmitRemote(inbound,bytes.data(),size).Status==E::EventRemoteAdmissionStatus::TemporarilyUnavailable); // Lane busy; one pool slot is free.
    target.Blocked=false;target.Node.NotifyCapacityChanged();Await([&]{return target.Calls==1;});
    E::EventRemoteAdmissionResult second;Await([&]{second=runtime.TryAdmitRemote(inbound,bytes.data(),size);return second.Status!=E::EventRemoteAdmissionStatus::TemporarilyUnavailable;});
    assert(second.Status==E::EventRemoteAdmissionStatus::Admitted);Await([&]{return target.Calls==2;});
    header.Key.MessageId=E::ConceptualMessageId{3};assert(E::EncodeEventWireHeader(header,bytes.data(),bytes.size()));
    assert(runtime.TryAdmitRemote(inbound,bytes.data(),size).Status==E::EventRemoteAdmissionStatus::TemporarilyUnavailable); // Retained references exhaust the pool.
    target.Held[0].Reset();E::EventRemoteAdmissionResult third;
    Await([&]{third=runtime.TryAdmitRemote(inbound,bytes.data(),size);return third.Status!=E::EventRemoteAdmissionStatus::TemporarilyUnavailable;});
    assert(third.Status==E::EventRemoteAdmissionStatus::Admitted);Await([&]{return target.Calls==3;});
    assert(runtime.Shutdown()==E::EventRuntimeStatus::Success);type.RemoveTarget(target.Node);
    for(auto& lease:target.Held) lease.Reset();
    assert(type.LiveInstances()==0);
}
