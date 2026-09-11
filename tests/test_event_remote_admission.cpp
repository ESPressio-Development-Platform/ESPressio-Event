#include <ESPressio_EventRuntime.hpp>
#include <ESPressio_EventThreadCapability.hpp>
#include <ESPressio_ThreadWith.hpp>
#include <HostRuntime.hpp>
#include <array>
#include <cassert>
#include <thread>
#include <chrono>
using namespace ESPressio;
namespace E=ESPressio::Event;
struct Delivery final {
    using PolicyCategory=Primitive::OccurrenceDeliveryPolicyTag;
    using RequiredEvidence=Primitive::DestinationPrimitiveAdmission;
    using TerminalDisposition=Primitive::ReportTerminalFailureToFamily;
    static constexpr std::uint64_t MaximumResidenceNanoseconds=10000000000ULL;
    static constexpr std::uint16_t MaximumAttempts=3;
    static constexpr std::uint64_t MaximumAdapterAdmissionWaitNanoseconds=1000000;
    static constexpr std::uint64_t MinimumRetrySpacingNanoseconds=1000000,MaximumRetrySpacingNanoseconds=10000000;
};
struct Remote final : E::TransmissibleEvent<Remote> {
    static constexpr E::EventTypeId TypeId{0x2030405060708090ULL};
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=2;
    static constexpr std::string_view CanonicalName="test.remote";
    using DeliveryPolicy=Delivery;
    std::uint32_t Value=0;
    ESPRESSIO_SERIALIZABLE_TYPE(Remote)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("value",Value))
};
using Inbox=E::ThreadCapability<E::SharedPendingCapacity<0>,Remote>;
struct Consumer final : Threads::ThreadWith<Inbox> {
    std::atomic<unsigned> Calls{0};
    std::atomic<std::uint32_t> LastValue{0};
    void Handle(const Remote& event) { LastValue=event.Value; ++Calls; }
    void OnInitialization() override { assert(GetCapability<E::ThreadCapabilityTag>().Listen<Remote>(*this,&Consumer::Handle)); }
    ~Consumer() override { assert(Shutdown()==Threads::ThreadStatus::Success); }
};
struct External final {
    E::EventTargetNode Node;
    std::atomic<unsigned> Calls{0};
    External() {
        Node.Owner=this;Node.Class=E::EventTargetClass::ExternalAdapter;
        Node.Validate=[](void*) noexcept {return true;};
        Node.Quiesce=[](void*) noexcept {};
        Node.TryAdmit=[](void* p,const E::EventLease& lease) noexcept {
            const auto& occurrence=lease.Get<Remote>();
            assert(occurrence.IsOriginLocal());
            std::array<std::uint8_t,E::MaximumCompletePrimitiveWireBytes<Remote,Serializable::DirectBinary>> wire;
            assert((E::EncodeEventWire<Remote,Serializable::DirectBinary>(occurrence,wire.data(),wire.size())));
            ++static_cast<External*>(p)->Calls;
            return E::EventTargetAdmission::Accepted;
        };
    }
};
template<class Predicate> void Await(Predicate predicate) {
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while(!predicate()) { assert(std::chrono::steady_clock::now()<deadline); std::this_thread::yield(); }
}
int main() {
    HostRuntime platform;
    Primitive::TypeDirectory<1> directory;
    assert(directory.Register<Remote>()==Primitive::TypeDirectoryRegistrationStatus::Success);
    assert(directory.Initialize()==Primitive::TypeDirectoryInitializationStatus::Success);
    std::array<E::RemoteAdmissionReceipt,2> receipts;
    E::RuntimeConfiguration config;
    config.RemoteAdmissionReceipts=receipts.data();config.RemoteAdmissionReceiptCapacity=receipts.size();
    E::Runtime runtime(config);
    assert(runtime.Initialize(directory.View())==E::EventRuntimeStatus::Success);
    auto inbound=runtime.BindInbound<Remote,Serializable::DirectBinary>();assert(inbound);
    Consumer consumer;assert(consumer.Initialize()==Threads::ThreadStatus::Success);
    External external;
    auto& type=E::EventTypeRuntime<Remote>::Get();
    assert(type.StageTarget(external.Node)==E::EventRuntimeStatus::Success);
    assert(runtime.Start()==E::EventRuntimeStatus::Success);
    assert(Remote::TryDispatch().Status==E::EventDispatchStatus::IdentityUnavailable);
    assert(consumer.Start()==Threads::ThreadStatus::Success);
    Remote payload;payload.Value=41;
    std::array<std::uint8_t,E::MaximumCompletePrimitiveWireBytes<Remote,Serializable::DirectBinary>> wire{};
    const auto encoded=Serializable::SerializeDirectBinary(payload,wire.data()+53,wire.size()-53);assert(encoded);
    System::DeviceIdentifier::Storage source{};source[0]=1;
    E::EventWireHeader header{{Remote::TypeId,{System::DeviceIdentifier{source},System::RuntimeIncarnationId{7}},E::ConceptualMessageId{18}},
        {123456,Timing::TimeReliability::Acquiring},static_cast<std::uint32_t>(encoded.Bytes)};
    assert(E::EncodeEventWireHeader(header,wire.data(),wire.size()));
    const auto bytes=53+encoded.Bytes;
    auto invalid=wire;
    invalid[4]^=1; // A valid header for an unknown Type cannot use this binding.
    assert(runtime.TryAdmitRemote(inbound,invalid.data(),bytes).Status==E::EventRemoteAdmissionStatus::UnknownType);
    invalid=wire;invalid[2]=2;
    assert(runtime.TryAdmitRemote(inbound,invalid.data(),bytes).Status==E::EventRemoteAdmissionStatus::UnsupportedProtocol);
    assert(runtime.TryAdmitRemote(inbound,wire.data(),bytes-1).Status==E::EventRemoteAdmissionStatus::Invalid);
    std::array<std::uint8_t,decltype(wire){}.size()+1> oversize{};
    header.PayloadLength=oversize.size()-53;
    assert(E::EncodeEventWireHeader(header,oversize.data(),oversize.size()));
    assert(runtime.TryAdmitRemote(inbound,oversize.data(),oversize.size()).Status==E::EventRemoteAdmissionStatus::Invalid);
    header.PayloadLength=encoded.Bytes;
    E::EventRemoteAdmissionResult first;
    Await([&]{ first=runtime.TryAdmitRemote(inbound,wire.data(),bytes);return first.Status!=E::EventRemoteAdmissionStatus::TemporarilyUnavailable; });
    assert(first.Status==E::EventRemoteAdmissionStatus::Admitted && first.EstablishesDestinationAdmission());
    assert(runtime.TryAdmitRemote(inbound,wire.data(),bytes).Status==E::EventRemoteAdmissionStatus::AlreadyAdmitted);
    Await([&]{return consumer.Calls==1;}); assert(consumer.LastValue==41 && external.Calls==0);
    // Failed payload decode releases its provisional receipt; a repaired retry can admit.
    header.Key.MessageId=E::ConceptualMessageId{19};assert(E::EncodeEventWireHeader(header,wire.data(),wire.size()));
    auto saved=wire[53];wire[53]=0;
    Await([&]{return type.LiveInstances()==0;});
    E::EventRemoteAdmissionResult bad;
    Await([&]{bad=runtime.TryAdmitRemote(inbound,wire.data(),bytes);return bad.Status!=E::EventRemoteAdmissionStatus::TemporarilyUnavailable;});
    assert(bad.Status==E::EventRemoteAdmissionStatus::SchemaOrDecodeFailure);
    wire[53]=saved;
    E::EventRemoteAdmissionResult repaired;
    Await([&]{repaired=runtime.TryAdmitRemote(inbound,wire.data(),bytes);return repaired.Status!=E::EventRemoteAdmissionStatus::TemporarilyUnavailable;});
    assert(repaired.Status==E::EventRemoteAdmissionStatus::Admitted);
    header.Key.MessageId=E::ConceptualMessageId{20};assert(E::EncodeEventWireHeader(header,wire.data(),wire.size()));
    assert(runtime.TryAdmitRemote(inbound,wire.data(),bytes).Status==E::EventRemoteAdmissionStatus::TemporarilyUnavailable);
    // Installed local source is separate from the immutable remote identity.
    System::DeviceIdentifier::Storage local{};local[0]=2;
    assert(System::RuntimeIdentity::Install({System::DeviceIdentifier{local},System::RuntimeIncarnationId{3}})==System::RuntimeIdentity::InstallationStatus::Success);
    Await([&]{return consumer.Calls==2;});
    assert(Remote::Dispatch());
    Await([&]{return external.Calls==1 && consumer.Calls==3;});
    assert(runtime.Shutdown()==E::EventRuntimeStatus::Success);
    assert(consumer.Shutdown()==Threads::ThreadStatus::Success);
    type.RemoveTarget(external.Node);assert(type.LiveInstances()==0);
}
