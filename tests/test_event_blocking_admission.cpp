#include <ESPressio_Event.hpp>
#include <HostRuntime.hpp>
#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>
namespace E=ESPressio::Event;
using namespace ESPressio;
struct Value final:E::Event<Value> {
    static constexpr E::EventTypeId TypeId{91};
    static constexpr std::size_t MaximumLiveInstances=1,MaximumPendingInstances=1;
    inline static std::atomic<unsigned> Constructions{0};
    int Number;
    explicit Value(int n):Number(n) {++Constructions;}
};
std::atomic<std::uint64_t> clockTime{10};
std::atomic<unsigned> clockCaptures{0};
Timing::QualifiedTime Capture() {const auto time=clockTime.load();++clockCaptures;return {time,Timing::TimeReliability::Acquiring};}
struct Target final {
    E::EventTargetNode Node;
    E::EventLease Owned;
    std::mutex Mutex;
    std::atomic<unsigned> Calls{0};
    std::atomic<std::uint64_t> LastOrigin{0};
    Target() {
        Node.Owner=this;Node.Validate=[](void*) noexcept {return true;};
        Node.Quiesce=[](void* p) noexcept {static_cast<Target*>(p)->Release();};
        Node.TryAdmit=[](void* p,const E::EventLease& event) noexcept {
            auto& self=*static_cast<Target*>(p);std::lock_guard<std::mutex> lock(self.Mutex);
            self.Owned=event.Retain();self.LastOrigin=event.Facts().OriginDispatchTime.Nanoseconds;++self.Calls;
            return E::EventTargetAdmission::Accepted;
        };
    }
    void Release() noexcept {std::lock_guard<std::mutex> lock(Mutex);Owned.Reset();}
};
template<class P> void Await(P p) {
    const auto limit=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while(!p()) {assert(std::chrono::steady_clock::now()<limit);std::this_thread::yield();}
}
int main() {
    HostRuntime platform;
    auto& type=E::EventTypeRuntime<Value>::Get();
    assert(type.Initialize(&platform,{},Capture)==E::EventRuntimeStatus::Success);
    Target target;assert(type.StageTarget(target.Node)==E::EventRuntimeStatus::Success);
    assert(type.ValidateStart());type.StartValidated();
    assert(Value::Dispatch(1));Await([&]{return target.Calls==1;});
    clockTime=20;
    std::atomic<bool> returned{false};E::EventDispatchResult result;
    std::thread producer([&]{result=Value::Dispatch(2);returned=true;});
    Await([&]{return clockCaptures>=3;});
    assert(!returned && Value::Constructions==1);
    assert(Value::TryDispatch(3).Status==E::EventDispatchStatus::CapacityUnavailable);
    clockTime=1000;target.Release();
    producer.join();assert(result);Await([&]{return target.Calls==2;});
    assert(target.LastOrigin==20 && Value::Constructions==2);
    // Multiple waiting producers all leave through the stop baton; none retain
    // the short admission gate while waiting or construct an unreserved payload.
    std::array<std::thread,3> waiters;
    std::atomic<unsigned> stopped{0};
    const auto captures=clockCaptures.load();
    for(auto& waiter:waiters) waiter=std::thread([&]{assert(Value::Dispatch(4).Status==E::EventDispatchStatus::Stopping);++stopped;});
    Await([&]{return clockCaptures>=captures+3;});
    assert(type.Shutdown()==E::EventRuntimeStatus::Success);
    for(auto& waiter:waiters) waiter.join();
    assert(stopped==3 && Value::Constructions==2 && type.LiveInstances()==0);
    type.RemoveTarget(target.Node);
}
