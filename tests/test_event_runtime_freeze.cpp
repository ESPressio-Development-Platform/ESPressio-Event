#include <ESPressio_EventRuntime.hpp>
#include <ESPressio_EventThreadCapability.hpp>
#include <ESPressio_ThreadWith.hpp>
#include <HostRuntime.hpp>
#include <cassert>
using namespace ESPressio;
namespace E=ESPressio::Event;
struct Definition final : E::Event<Definition> {
    static constexpr E::EventTypeId TypeId{67};
    static constexpr std::size_t MaximumLiveInstances=2,MaximumPendingInstances=1;
    static constexpr std::string_view CanonicalName="test.definition";
};
using Inbox=E::ThreadCapability<E::SharedPendingCapacity<0>,Definition>;
struct Consumer final : Threads::ThreadWith<Inbox> {
    bool Missing=false,Duplicate=false;
    void Handle(const Definition&) {}
    void OnInitialization() override {
        auto& inbox=GetCapability<E::ThreadCapabilityTag>();
        if(!Missing) assert(inbox.Listen<Definition>(*this,&Consumer::Handle));
        if(Duplicate) assert(!inbox.Listen<Definition>(*this,&Consumer::Handle));
    }
    ~Consumer() override { assert(Shutdown()==Threads::ThreadStatus::Success); }
};
int main() {
    HostRuntime platform;
    Primitive::TypeDirectory<1> directory;
    E::Runtime runtime;
    assert(runtime.Initialize(directory.View())==E::EventRuntimeStatus::InvalidDirectory);
    assert(directory.Register<Definition>()==Primitive::TypeDirectoryRegistrationStatus::Success);
    assert(directory.Initialize()==Primitive::TypeDirectoryInitializationStatus::Success);
    assert(runtime.Initialize(directory.View())==E::EventRuntimeStatus::Success);
    assert(Definition::TryDispatch().Status==E::EventDispatchStatus::NotInitialized);
    Consumer missing; missing.Missing=true;
    assert(missing.Initialize()==Threads::ThreadStatus::InitializationFailed);
    Consumer duplicate; duplicate.Duplicate=true;
    assert(duplicate.Initialize()==Threads::ThreadStatus::InitializationFailed);
    Consumer consumer;
    assert(consumer.Initialize()==Threads::ThreadStatus::Success);
    assert(runtime.Start()==E::EventRuntimeStatus::Success && runtime.IsRunning());
    assert(runtime.Start()==E::EventRuntimeStatus::Frozen);
    E::EventTargetNode late;
    assert(E::EventTypeRuntime<Definition>::Get().StageTarget(late)==E::EventRuntimeStatus::Frozen);
    assert(Definition::Dispatch());
    assert(runtime.Shutdown()==E::EventRuntimeStatus::Success && !runtime.IsRunning());
    assert(E::EventTypeRuntime<Definition>::Get().LiveInstances()==0);
    assert(consumer.Shutdown()==Threads::ThreadStatus::Success);
}
