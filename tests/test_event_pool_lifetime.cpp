#include <ESPressio_EventInstancePool.hpp>
#include <cassert>
#include <cstdlib>
#include <new>
#include <thread>
#include <atomic>

static std::atomic<bool> denyHeap{false};
void* operator new(std::size_t n) { if (denyHeap) std::abort(); if (auto* p=std::malloc(n)) return p; throw std::bad_alloc(); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
using namespace ESPressio;
struct alignas(64) Value final {
    static constexpr Event::EventTypeId TypeId{37};
    static constexpr std::size_t MaximumLiveInstances=2, MaximumPendingInstances=1;
    inline static std::atomic<int> Alive{0},Destroyed{0};
    int Number;
    explicit Value(int n) : Number(n) { if (n<0) throw n; ++Alive; }
    ~Value() noexcept { --Alive; ++Destroyed; }
};
int main() {
    static_assert(Event::Detail::ValidateEventType<Value>());
    static_assert(sizeof(Event::EventLease)==sizeof(void*));
    Event::EventInstancePool<Value,2> pool;
    int wakeCount=0;
    pool.BindCapacityWake(&wakeCount,[](void* p) noexcept { ++*static_cast<int*>(p); });
    const Event::EventOccurrenceFacts facts{Value::TypeId,Event::ConceptualMessageId{9},{17,Timing::TimeReliability::Holdover}};
    // Resolve the provider mutex before measuring occurrence admission.
    { auto warm=pool.TryReserve(); assert(warm); }
    denyHeap=true;
    {
        auto first=pool.TryReserve(); auto second=pool.TryReserve();
        assert(first && second && pool.Occupied()==2 && !pool.TryReserve());
        auto a=first.Construct(facts,4); auto b=second.Construct(facts,5);
        assert(a.Get<Value>().Number==4 && reinterpret_cast<std::uintptr_t>(&a.Get<Value>())%64==0);
        assert(a.Facts().MessageId.Value()==9 && a.Facts().OriginDispatchTime.Reliability==Timing::TimeReliability::Holdover);
        auto retained=a.Retain(); auto* onePointer=retained.ReleaseToInbox();
        a.Reset(); assert(pool.Occupied()==2 && Value::Alive==2);
        auto callback=Event::EventLease::AdoptFromInbox(onePointer);
        assert(callback.Get<Value>().Number==4);
        callback.Reset(); assert(pool.Occupied()==1 && Value::Alive==1);
        auto reused=pool.TryReserve(); assert(reused);
        auto replacement=reused.Construct(facts,6);
        b=std::move(replacement); assert(pool.Occupied()==1 && b.Get<Value>().Number==6);
    }
    denyHeap=false;
    assert(pool.Occupied()==0 && Value::Alive==0 && Value::Destroyed==3);
    try { auto reservation=pool.TryReserve(); (void)reservation.Construct(facts,-1); assert(false); }
    catch(int n) { assert(n==-1); }
    assert(pool.Occupied()==0 && Value::Destroyed==3 && wakeCount==5);
    // References on separate threads keep one generation alive until every owner releases.
    auto reservation=pool.TryReserve(); auto root=reservation.Construct(facts,8);
    auto left=root.Retain(),right=root.Retain(); root.Reset();
    std::thread t1([lease=std::move(left)]() mutable { assert(lease.Get<Value>().Number==8); lease.Reset(); });
    std::thread t2([lease=std::move(right)]() mutable { assert(lease.Get<Value>().Number==8); lease.Reset(); });
    t1.join(); t2.join();
    assert(pool.Occupied()==0 && Value::Alive==0 && Value::Destroyed==4);
}
