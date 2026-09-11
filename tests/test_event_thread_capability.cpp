#include <ESPressio_EventThreadCapability.hpp>
#include <ESPressio_ThreadWith.hpp>
#include <ESPressio_Precision.hpp>
#include <HostRuntime.hpp>
#include <cassert>
#include <chrono>
#include <thread>
using namespace ESPressio;
namespace E=ESPressio::Event;
struct Private final : E::Event<Private> {
    static constexpr E::EventTypeId TypeId{11};
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=1;
    int Value;
    explicit Private(int n) : Value(n) {}
};
struct Shared final : E::Event<Shared> {
    static constexpr E::EventTypeId TypeId{12};
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=0;
    int Value;
    explicit Shared(int n) : Value(n) {}
};
using Events=E::ThreadCapability<E::SharedPendingCapacity<2>,Private,Shared>;
class Receiver final : public Threads::ThreadWith<Events,Threads::Precision<2>> {
public:
    std::atomic<unsigned> Calls{0};
    std::array<int,8> Values{};
    std::atomic<bool> Throw{false};
    ~Receiver() override { assert(Shutdown()==Threads::ThreadStatus::Success); }
    Events& Inbox() { return GetCapability<E::ThreadCapabilityTag>(); }
protected:
    void OnInitialization() override {
        assert(Inbox().Listen<Private>(*this,&Receiver::OnPrivate));
        assert(Inbox().Listen<Shared>(*this,&Receiver::OnShared));
    }
    void OnPrivate(const Private& e) { const bool fail=Throw.load(); Values[Calls.load()]=e.Value; ++Calls; if(fail) throw 19; }
    void OnShared(const Shared& e) { Values[Calls.load()]=e.Value; ++Calls; }
};
template<class Predicate> void Await(Predicate predicate) {
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while(!predicate()) { assert(std::chrono::steady_clock::now()<deadline); std::this_thread::yield(); }
}
int main() {
    HostRuntime platform;
    auto& a=E::EventTypeRuntime<Private>::Get(); auto& b=E::EventTypeRuntime<Shared>::Get();
    assert(a.Initialize(&platform,{})==E::EventRuntimeStatus::Success);
    assert(b.Initialize(&platform,{})==E::EventRuntimeStatus::Success);
    Receiver receiver;
    assert(receiver.Initialize()==Threads::ThreadStatus::Success);
    assert(Events::PendingCapacity==3 && Events::PendingPointerBytes==3*sizeof(void*));
    assert(a.ValidateStart() && b.ValidateStart()); a.StartValidated();b.StartValidated();
    // Initialized but not started: all admitted references remain in one FIFO.
    assert(Private::Dispatch(1)); Await([&]{return receiver.Inbox().Pending()==1;});
    assert(Shared::Dispatch(2)); Await([&]{return receiver.Inbox().Pending()==2;});
    assert(Shared::Dispatch(3)); Await([&]{return receiver.Inbox().Pending()==3;});
    assert(receiver.Calls==0);
    // The lane accepts one extra root; its owed target waits without borrowing.
    assert(Private::Dispatch(4));
    assert(Private::TryDispatch(5).Status==E::EventDispatchStatus::CapacityUnavailable);
    assert(receiver.Start()==Threads::ThreadStatus::Success);
    Await([&]{return receiver.Calls==4;});
    assert((receiver.Values[0]==1 && receiver.Values[1]==2 && receiver.Values[2]==3 && receiver.Values[3]==4));
    receiver.Throw=true;
    assert(Private::Dispatch(6));
    Await([&]{return receiver.GetThreadState()==Threads::ThreadState::Terminated;});
    const auto failure=receiver.GetDiagnostics().Failure;
    assert(failure.Cause);
    try { std::rethrow_exception(failure.Cause); } catch(int n) { assert(n==19); }
    assert(receiver.Shutdown()==Threads::ThreadStatus::Success);
    assert(a.Shutdown()==E::EventRuntimeStatus::Success && b.Shutdown()==E::EventRuntimeStatus::Success);
    assert(a.LiveInstances()==0 && b.LiveInstances()==0);
    assert(platform.Created==3 && platform.Joined==3 && platform.Signals==7);
}
