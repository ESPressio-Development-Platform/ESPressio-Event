#include <ESPressio_Event.hpp>
#include <HostRuntime.hpp>
#include <cassert>
using namespace ESPressio;
namespace E=ESPressio::Event;
template<unsigned Id> struct Message final:E::Event<Message<Id>> {
    static constexpr E::EventTypeId TypeId{Id};
    static constexpr std::string_view CanonicalName=Id==1 ? "test.bootstrap.one" : "test.bootstrap.two";
    static constexpr std::size_t MaximumLiveInstances=2,MaximumPendingInstances=1;
    explicit Message(bool fail=false) {if(fail) throw 17;}
};
struct FailingSignals final:System::Synchronization::ISynchronizationProvider {
    HostRuntime& Host;
    unsigned Calls=0,FailAt=4;
    explicit FailingSignals(HostRuntime& host):Host(host){}
    std::unique_ptr<System::Synchronization::ISignal> CreateBinarySignal(bool initial=false) override {
        if(++Calls==FailAt) throw std::bad_alloc();
        return Host.CreateBinarySignal(initial);
    }
};
int main() {
    HostRuntime host;
    FailingSignals signals(host);System::Synchronization::SetProvider(&signals);
    Primitive::TypeDirectory<2> directory;
    assert(directory.Register<Message<1>>()==Primitive::TypeDirectoryRegistrationStatus::Success);
    assert(directory.Register<Message<2>>()==Primitive::TypeDirectoryRegistrationStatus::Success);
    assert(directory.Initialize()==Primitive::TypeDirectoryInitializationStatus::Success);
    E::Runtime runtime;
    assert(runtime.Initialize(directory.View())==E::EventRuntimeStatus::StorageUnavailable);
    assert(host.Created==1 && host.Joined==1 && !runtime.IsRunning());
    assert(Message<1>::TryDispatch().Status==E::EventDispatchStatus::NotInitialized);
    signals.FailAt=0;
    assert(runtime.Initialize(directory.View())==E::EventRuntimeStatus::Success);
    assert(runtime.Start()==E::EventRuntimeStatus::Success);
    try {(void)Message<1>::Dispatch(true);assert(false);} catch(int failure){assert(failure==17);}
    assert(E::EventTypeRuntime<Message<1>>::Get().LiveInstances()==0);
    const auto admitted=Message<1>::Dispatch();assert(admitted && admitted.MessageId.Value()==2);
    assert(runtime.Shutdown()==E::EventRuntimeStatus::Success);
    assert(host.Created==3 && host.Joined==3);
    assert(E::EventTypeRuntime<Message<1>>::Get().LiveInstances()==0);
    System::Synchronization::SetProvider(&host);
}
