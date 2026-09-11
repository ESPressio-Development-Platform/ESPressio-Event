#include <ESPressio_EventTypeRuntime.hpp>
#include <HostRuntime.hpp>
#include <cassert>
#include <chrono>
#include <thread>
using namespace ESPressio;
struct Occurrence final : Event::Event<Occurrence> {
    static constexpr ESPressio::Event::EventTypeId TypeId{5};
    static constexpr std::size_t MaximumLiveInstances=2,MaximumPendingInstances=1;
    int Value;
    explicit Occurrence(int value) : Value(value) {}
};
struct Target final {
    Event::EventTargetNode Node;
    Event::EventLease Owned;
    std::atomic<bool> Full{false};
    std::atomic<unsigned> Calls{0};
    std::mutex Mutex;
    Target() {
        Node.Owner=this;
        Node.Validate=[](void*) noexcept { return true; };
        Node.Quiesce=[](void* p) noexcept { auto& t=*static_cast<Target*>(p); std::lock_guard<std::mutex> lock(t.Mutex); t.Owned.Reset(); };
        Node.TryAdmit=[](void* p,const Event::EventLease& event) noexcept {
            auto& t=*static_cast<Target*>(p);
            std::lock_guard<std::mutex> lock(t.Mutex);
            ++t.Calls;
            if(t.Full) return Event::EventTargetAdmission::CapacityUnavailable;
            t.Owned=event.Retain();
            return Event::EventTargetAdmission::Accepted;
        };
    }
};
template<class Predicate> void Await(Predicate predicate) {
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while(!predicate()) { assert(std::chrono::steady_clock::now()<deadline); std::this_thread::yield(); }
}
int main() {
    HostRuntime platform;
    auto& type=Event::EventTypeRuntime<Occurrence>::Get();
    assert(Occurrence::TryDispatch(1).Status==Event::EventDispatchStatus::NotInitialized);
    Task::TaskExecutionConfiguration config;
    assert(type.Initialize(&platform,config,+[]() -> Timing::QualifiedTime {return {123,Timing::TimeReliability::Holdover};})==Event::EventRuntimeStatus::Success);
    Target healthy,blocked;
    blocked.Full=true;
    assert(type.StageTarget(healthy.Node)==Event::EventRuntimeStatus::Success);
    assert(type.StageTarget(blocked.Node)==Event::EventRuntimeStatus::Success);
    assert(type.ValidateStart()); type.StartValidated();
    const auto first=Occurrence::Dispatch(7); assert(first && first.MessageId.Value()==1);
    Await([&]{return healthy.Calls==1;});
    assert(blocked.Calls>=1);
    assert(Occurrence::TryDispatch(8).Status==Event::EventDispatchStatus::CapacityUnavailable);
    { std::lock_guard<std::mutex> lock(healthy.Mutex); assert(healthy.Owned.Get<Occurrence>().Value==7); assert(healthy.Owned.Facts().OriginDispatchTime.Nanoseconds==123); }
    blocked.Full=false; blocked.Node.NotifyCapacityChanged();
    Await([&]{return blocked.Calls>=2;});
    Event::EventDispatchResult second;
    Await([&]{ second=Occurrence::TryDispatch(9); return bool(second); });
    assert(second.MessageId.Value()==2);
    Await([&]{return healthy.Calls==2;});
    assert(type.Shutdown()==Event::EventRuntimeStatus::Success);
    type.RemoveTarget(healthy.Node);type.RemoveTarget(blocked.Node);
    assert(type.LiveInstances()==0);
    assert(Occurrence::TryDispatch(10).Status==Event::EventDispatchStatus::Stopping);
    assert(platform.Created==1 && platform.Joined==1 && platform.Signals==3);
}
