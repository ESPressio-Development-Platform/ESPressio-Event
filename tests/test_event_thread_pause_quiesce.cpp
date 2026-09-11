#include <ESPressio_Event.hpp>
#include <ESPressio_ThreadWith.hpp>
#include <HostRuntime.hpp>
#include <cassert>
using namespace ESPressio;
namespace E=ESPressio::Event;
struct Message:E::Event<Message>{
    static constexpr E::EventTypeId TypeId{111};
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=1;
};
using Inbox=E::ThreadCapability<E::SharedPendingCapacity<0>,Message>;
std::atomic<bool> entered{false},releaseHandler{false};
struct Receiver:Threads::ThreadWith<Inbox>{
    std::atomic<unsigned> Calls{0};
    Inbox& Events(){return GetCapability<E::ThreadCapabilityTag>();}
    void Handle(const Message&){
        assert(Events().Pending()==0); // Quota is released before application invocation.
        ++Calls;entered=true;
        while(!releaseHandler)std::this_thread::yield();
    }
    void OnInitialization()override{assert(Events().Listen<Message>(*this,&Receiver::Handle));}
    ~Receiver()override{assert(Shutdown()==Threads::ThreadStatus::Success);}
};
template<class P>void Await(P p){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(3);while(!p()){assert(std::chrono::steady_clock::now()<end);std::this_thread::yield();}}
int main(){
    HostRuntime host;auto& type=E::EventTypeRuntime<Message>::Get();assert(type.Initialize(&host,{})==E::EventRuntimeStatus::Success);
    Receiver receiver;assert(receiver.Initialize()==Threads::ThreadStatus::Success);assert(type.ValidateStart());type.StartValidated();
    assert(receiver.Start()==Threads::ThreadStatus::Success);
    assert(receiver.Pause()==Threads::ThreadStatus::Success);
    assert(Message::Dispatch());Await([&]{return receiver.Events().Pending()==1;});assert(receiver.Calls==0);
    assert(receiver.Start()==Threads::ThreadStatus::Success);Await([]{return entered.load();});
    // The first callback still owns its occurrence, but the private inbox slot
    // accepts the next occurrence because dequeue released capacity first.
    assert(Message::Dispatch());Await([&]{return receiver.Events().Pending()==1;});
    receiver.Terminate();releaseHandler=true;
    assert(receiver.Shutdown()==Threads::ThreadStatus::Success);assert(receiver.Calls==1 && receiver.Events().Pending()==0);
    assert(type.Shutdown()==E::EventRuntimeStatus::Success);assert(type.LiveInstances()==0);
}
