#include <ESPressio_Event.hpp>
#include <ESPressio_EventOutboundBinding.hpp>
#include <HostRuntime.hpp>
#include <cassert>
#include <atomic>
#include <array>
#include <new>
#include <cstdlib>
using namespace ESPressio;
namespace E=ESPressio::Event;
static std::atomic<bool> deny{false};
void* operator new(std::size_t n) {if(deny) std::abort();if(auto* p=std::malloc(n?n:1)) return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
struct Policy {
    using PolicyCategory=Primitive::OccurrenceDeliveryPolicyTag;
    using RequiredEvidence=Primitive::DestinationPrimitiveAdmission;
    using TerminalDisposition=Primitive::DiagnosticOnlyAfterBudget;
    static constexpr std::uint64_t MaximumResidenceNanoseconds=1000000000,MaximumAdapterAdmissionWaitNanoseconds=1000;
    static constexpr std::uint16_t MaximumAttempts=2;
    static constexpr std::uint64_t MinimumRetrySpacingNanoseconds=1000,MaximumRetrySpacingNanoseconds=10000;
};
struct EquivalentPolicy:Policy {};
struct DifferentPolicy:Policy {static constexpr std::uint64_t MaximumResidenceNanoseconds=2000000000;};
template<class SelectedPolicy,std::size_t Capacity> struct Packet final:E::TransmissibleEvent<Packet<SelectedPolicy,Capacity>> {
    static constexpr E::EventTypeId TypeId{77};
    static constexpr std::string_view CanonicalName="test.packet";
    static constexpr std::size_t MaximumLiveInstances=Capacity,MaximumPendingInstances=1;
    using DeliveryPolicy=SelectedPolicy;
    Serializable::BoundedString<16> Text;
    Serializable::BoundedVector<std::int16_t,4> Samples;
    ESPRESSIO_SERIALIZABLE_TYPE(Packet)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("text",Text).Alias("formerText"),ESPRESSIO_PROPERTY("samples",Samples))
};
using Message=Packet<Policy,4>;
template<class Format> struct FormatStorage final {
    std::array<std::uint8_t,E::MaximumCompletePrimitiveWireBytes<Message,Format>> Bytes{};
    std::size_t Size=0;
    void Encode(const Message& message) noexcept {
        const auto encoded=E::EncodeEventWire<Message,Format>(message,Bytes.data(),Bytes.size());assert(encoded);
        Size=encoded.Bytes;assert(Size<=Bytes.size());
        E::EventWireHeader header;assert(E::DecodeEventWireHeader(Bytes.data(),Size,header));
        assert(header.Key.Origin==message.GetOriginDeviceRuntimeIdentity());
        Message decoded;
        if constexpr(std::is_same_v<Format,Serializable::DirectBinary>) assert(Serializable::DeserializeBoundedDirectBinary(Bytes.data()+53,Size-53,decoded));
        else if constexpr(std::is_same_v<Format,Serializable::CBOR>) assert(Serializable::DeserializeBoundedCbor(Bytes.data()+53,Size-53,decoded));
        else assert(Serializable::DeserializeBoundedJson(Bytes.data()+53,Size-53,decoded));
        assert(decoded.Text==message.Text && decoded.Samples==message.Samples);
    }
};
struct Adapter final {
    FormatStorage<Serializable::DirectBinary> Binary;
    FormatStorage<Serializable::CBOR> Cbor;
    FormatStorage<Serializable::JSON> Json;
    std::atomic<unsigned> Calls{0};
    E::EventOutboundBinding<Message> Binding;
    bool Validate() noexcept {return true;}
    E::EventTargetAdmission Admit(const E::EventLease& lease) noexcept {
        const auto& message=lease.Get<Message>();Binary.Encode(message);Cbor.Encode(message);Json.Encode(message);++Calls;
        return E::EventTargetAdmission::Accepted;
    }
};
template<class P> void Await(P p) {const auto limit=std::chrono::steady_clock::now()+std::chrono::seconds(3);while(!p()){assert(std::chrono::steady_clock::now()<limit);std::this_thread::yield();}}
int main() {
    const auto original=Message::GetPrimitiveTypeDescriptor();
    const auto equivalent=Packet<EquivalentPolicy,8>::GetPrimitiveTypeDescriptor();
    const auto different=Packet<DifferentPolicy,4>::GetPrimitiveTypeDescriptor();
    assert(!original.Contract.IsZero() && original.Contract==equivalent.Contract && original.Contract!=different.Contract);
    assert((original.SerializedSize.MaximumCompletePrimitiveWireBytes==E::MaximumCompletePrimitiveWireBytes<Message,Serializable::JSON>));
    HostRuntime platform;
    System::DeviceIdentifier::Storage local{};local[0]=3;
    assert(System::RuntimeIdentity::Install({System::DeviceIdentifier{local},System::RuntimeIncarnationId{5}})==System::RuntimeIdentity::InstallationStatus::Success);
    Primitive::TypeDirectory<1> directory;assert(directory.Register<Message>()==Primitive::TypeDirectoryRegistrationStatus::Success);
    assert(directory.Initialize()==Primitive::TypeDirectoryInitializationStatus::Success);
    std::array<E::RemoteAdmissionReceipt,4> receipts;
    E::Runtime runtime({{},receipts.data(),receipts.size()});
    assert(runtime.Initialize(directory.View())==E::EventRuntimeStatus::Success);
    auto binary=runtime.BindInbound<Message,Serializable::DirectBinary>();auto cbor=runtime.BindInbound<Message,Serializable::CBOR>();auto json=runtime.BindInbound<Message,Serializable::JSON>();
    Adapter adapter;assert((adapter.Binding.Initialize<Adapter,&Adapter::Admit,&Adapter::Validate>(adapter)==E::EventRuntimeStatus::Success));
    assert(runtime.Start()==E::EventRuntimeStatus::Success);
    Message message;assert(message.Text.assign("\"\\\n\t\x01utf8-\xc3\xa9"));
    for(int i=0;i<4;++i) assert(message.Samples.push_back(i%2 ? INT16_MAX : INT16_MIN));
    deny=true;
    assert(Message::Dispatch(message));Await([&]{return adapter.Calls==1;});
    auto deliver=[&](auto& storage,const E::EventInboundBinding& binding,std::uint64_t id) {
        E::EventWireHeader header;assert(E::DecodeEventWireHeader(storage.Bytes.data(),storage.Size,header));
        auto source=local;source[0]=4;
        header.Key.Origin.Device=System::DeviceIdentifier{source};header.Key.MessageId=E::ConceptualMessageId{id};
        assert(E::EncodeEventWireHeader(header,storage.Bytes.data(),storage.Bytes.size()));
        E::EventRemoteAdmissionResult result;
        Await([&]{result=runtime.TryAdmitRemote(binding,storage.Bytes.data(),storage.Size);return result.Status!=E::EventRemoteAdmissionStatus::TemporarilyUnavailable;});
        assert(result.Status==E::EventRemoteAdmissionStatus::Admitted);
        assert(runtime.TryAdmitRemote(binding,storage.Bytes.data(),storage.Size).Status==E::EventRemoteAdmissionStatus::AlreadyAdmitted);
    };
    deliver(adapter.Binary,binary,100);deliver(adapter.Cbor,cbor,101);deliver(adapter.Json,json,102);
    assert(adapter.Calls==1); // Remote deliveries cannot create another outbound campaign.
    adapter.Binding.Shutdown();assert(runtime.Shutdown()==E::EventRuntimeStatus::Success);
    assert(E::EventTypeRuntime<Message>::Get().LiveInstances()==0);
    const auto profile=runtime.GetResourceProfile();assert(profile.ReceiptBytes==sizeof(receipts) && profile.ExecutionContexts==1 && profile.Signals==4);
    deny=false;
}
