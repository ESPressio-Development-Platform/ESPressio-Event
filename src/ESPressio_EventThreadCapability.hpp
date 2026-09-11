#pragma once
#include <array>
#include <atomic>
#include <tuple>
#include <type_traits>
#include <ESPressio_ThreadCapability.hpp>
#include "ESPressio_EventTypeRuntime.hpp"

namespace ESPressio::Event {
struct ThreadCapabilityTag final {};
template<std::size_t N> struct SharedPendingCapacity final { static constexpr std::size_t Value=N; };
namespace Detail {
template<class... T> struct UniqueEventTypes : std::true_type {};
template<class First,class... Rest> struct UniqueEventTypes<First,Rest...> :
    std::bool_constant<(!std::is_same_v<First,Rest> && ...) && UniqueEventTypes<Rest...>::value> {};
struct EventMemberAnchor {};
template<class T> struct EventListenerBinding final {
    void* Owner=nullptr;
    void (EventMemberAnchor::*Member)(const T&)=nullptr;
    void (*Invoke)(EventListenerBinding&,const T&)=nullptr;
};
template<std::size_t Shared,class... T> constexpr bool InboxCapacityFits() noexcept {
    std::size_t total=Shared; bool fits=true;
    auto add=[&](std::size_t n) constexpr { if(n>SIZE_MAX-total) fits=false; else total+=n; };
    (add(T::MaximumPendingInstances),...);
    return fits && total<=SIZE_MAX/sizeof(void*);
}
}
/// One fixed pointer FIFO across the declared Types. Positive per-Type quotas
/// are private; only zero-reservation Types use the explicit shared quota.
/// It uses the host's common work wake and owns no worker or private work signal.
template<class Shared,class... TEvents> class ThreadCapability final : public Threads::ThreadCapability {
    static_assert(sizeof...(TEvents)>0, "Event capability requires declared Types");
    static_assert(Detail::UniqueEventTypes<TEvents...>::value, "Event capability Types must be unique");
    static_assert((Detail::ValidateEventType<TEvents>() && ...));
    static_assert(((TEvents::MaximumPendingInstances>0 || Shared::Value>0) && ...),
                  "Zero private reservation requires nonzero shared capacity");
    static_assert(Detail::InboxCapacityFits<Shared::Value,TEvents...>(), "Event FIFO capacity overflows storage size");
    static constexpr std::size_t TypeCount=sizeof...(TEvents);
    static constexpr std::size_t InboxSlots=Shared::Value+(TEvents::MaximumPendingInstances+...+std::size_t{0});
    inline static constexpr std::array<EventTypeId,TypeCount> TypeIds{TEvents::TypeId...};
    inline static constexpr std::array<std::size_t,TypeCount> PrivateLimits{TEvents::MaximumPendingInstances...};
    std::tuple<Detail::EventListenerBinding<TEvents>...> _listeners;
    std::array<EventTargetNode,TypeCount> _targets{};
    std::array<Detail::EventSlotControl*,InboxSlots> _fifo{};
    std::array<std::size_t,TypeCount> _privateCounts{};
    std::size_t _head=0,_tail=0,_sharedCount=0;
    std::atomic<std::size_t> _count{0};
    std::atomic<bool> _closed{true};
    System::Synchronization::Mutex _mutex;
    Threads::ThreadHostServices _host{};
    bool _bound=false,_frozen=false,_bindingError=false;
    template<class T> static constexpr std::size_t Index() noexcept {
        static_assert((std::is_same_v<T,TEvents> || ...), "Listen requires a declared Event Type");
        std::size_t i=0,result=0;
        ((std::is_same_v<T,TEvents> ? (void)(result=i) : (void)0,++i),...);
        return result;
    }
    static std::size_t Find(EventTypeId id) noexcept {
        for(std::size_t i=0;i<TypeCount;++i) if(TypeIds[i]==id) return i;
        std::terminate();
    }
    bool BindingsValid() const noexcept {
        return !_bindingError && std::apply([](const auto&... binding){return ((binding.Owner && binding.Invoke) && ...);},_listeners);
    }
    template<class T> EventRuntimeStatus Stage() noexcept {
        auto& node=_targets[Index<T>()];
        node.Owner=this;
        node.Class=EventTargetClass::LocalThread;
        node.Validate=[](void* p) noexcept { auto& self=*static_cast<ThreadCapability*>(p); return self._frozen && self.BindingsValid(); };
        node.Quiesce=[](void* p) noexcept { static_cast<ThreadCapability*>(p)->CloseInbox(); };
        node.TryAdmit=[](void* p,const EventLease& event) noexcept {
            return static_cast<ThreadCapability*>(p)->Admit(Index<T>(),event);
        };
        return EventTypeRuntime<T>::Get().StageTarget(node);
    }
    EventTargetAdmission Admit(std::size_t index,const EventLease& event) noexcept {
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            // This read is the admission linearization point against root
            // Terminate. Quiesce takes this same inbox gate before discarding;
            // an admission already in flight completes before that discard.
            if(_closed || !_host.IsAccepting()) return EventTargetAdmission::Quiesced;
            if(PrivateLimits[index]) {
                if(_privateCounts[index]==PrivateLimits[index]) return EventTargetAdmission::CapacityUnavailable;
                ++_privateCounts[index];
            } else {
                if(_sharedCount==Shared::Value) return EventTargetAdmission::CapacityUnavailable;
                ++_sharedCount;
            }
            auto retained=event.Retain();
            _fifo[_tail]=retained.ReleaseToInbox();
            _tail=(_tail+1)%InboxSlots; ++_count;
        }
        (void)_host.Wake();
        return EventTargetAdmission::Accepted;
    }
    void PublishCapacity(std::size_t index) noexcept {
        if(PrivateLimits[index]) _targets[index].NotifyCapacityChanged();
        else for(std::size_t i=0;i<TypeCount;++i) if(!PrivateLimits[i]) _targets[i].NotifyCapacityChanged();
    }
    void CloseInbox() noexcept {
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            _closed=true;
            while(_count) {
                auto event=EventLease::AdoptFromInbox(_fifo[_head]);
                _fifo[_head]=nullptr; _head=(_head+1)%InboxSlots; --_count;
            }
            _privateCounts.fill(0); _sharedCount=0; _tail=_head;
        }
        for(auto& target:_targets) target.NotifyCapacityChanged();
    }
    void DetachTargets() noexcept {
        (EventTypeRuntime<TEvents>::Get().RemoveTarget(_targets[Index<TEvents>()]),...);
    }
    template<class T> bool InvokeIf(EventLease& event) {
        if(event.Facts().TypeId!=T::TypeId) return false;
        auto& binding=std::get<Detail::EventListenerBinding<T>>(_listeners);
        binding.Invoke(binding,event.template Get<T>());
        return true;
    }
public:
    using CapabilityTag=ThreadCapabilityTag;
    static constexpr std::uint32_t FrameworkStackFloorBytes=1024;
    static constexpr std::size_t PendingCapacity=InboxSlots;
    static constexpr std::size_t PendingPointerBytes=InboxSlots*sizeof(void*);
    ThreadCapability() noexcept = default;
    ThreadCapability(const ThreadCapability&)=delete;
    ThreadCapability& operator=(const ThreadCapability&)=delete;
    ~ThreadCapability() { if(_bound) { CloseInbox(); DetachTargets(); } }
    /// Initialization-only fixed member binding. Reinterpretation is solely for
    /// storage: invocation restores the exact original member-pointer Type, as
    /// required by the C++ member-pointer round-trip guarantee.
    template<class T,class Owner> bool Listen(Owner& owner,void (Owner::*member)(const T&)) noexcept {
        constexpr auto index=Index<T>(); (void)index;
        if(_bound || _frozen) return false;
        auto& binding=std::get<Detail::EventListenerBinding<T>>(_listeners);
        if(binding.Invoke || member==nullptr) { _bindingError=true; return false; }
        binding.Owner=&owner;
        binding.Member=reinterpret_cast<void (Detail::EventMemberAnchor::*)(const T&)>(member);
        binding.Invoke=[](Detail::EventListenerBinding<T>& b,const T& value) {
            const auto method=reinterpret_cast<void (Owner::*)(const T&)>(b.Member);
            (static_cast<Owner*>(b.Owner)->*method)(value);
        };
        return true;
    }
    Threads::ThreadStatus Initialize(const Threads::ThreadHostServices& host) noexcept {
        { std::lock_guard<System::Synchronization::Mutex> warm(_mutex); }
        if(!BindingsValid() || !host.Owner || !host.WakeFunction || !host.AcceptingFunction)
            return Threads::ThreadStatus::InitializationFailed;
        _host=host; _bound=true; _closed=true;
        bool valid=true;
        ((valid ? (void)(valid=Stage<TEvents>()==EventRuntimeStatus::Success) : (void)0),...);
        return valid ? Threads::ThreadStatus::Success : Threads::ThreadStatus::InitializationFailed;
    }
    Threads::ThreadStatus FinalizeInitialization() noexcept {
        if(!_bound || !BindingsValid()) return Threads::ThreadStatus::InitializationFailed;
        _frozen=true; _closed=false;
        return Threads::ThreadStatus::Success;
    }
    void RollbackInitialization() noexcept {
        CloseInbox(); if(_bound) DetachTargets();
        _bound=false; _frozen=false; _bindingError=false; _listeners={};
    }
    void Quiesce(const Threads::ThreadCycleContext&) noexcept {
        CloseInbox(); if(_bound) DetachTargets(); _bound=false;
    }
    Threads::CapabilityReadiness Readiness(const Threads::ThreadCycleContext&) const noexcept {
        Threads::CapabilityReadiness ready; ready.Immediate=!_closed && _count!=0; return ready;
    }
    /// Exactly one TH7 Event quantum. The FIFO/quota reference is transferred to
    /// a local lease and capacity is published BEFORE invoking the const listener.
    void Service(const Threads::ThreadCycleContext&) {
        EventLease event; std::size_t index=0;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            if(_closed || !_count) return;
            event=EventLease::AdoptFromInbox(_fifo[_head]);
            _fifo[_head]=nullptr; _head=(_head+1)%InboxSlots; --_count;
            index=Find(event.Facts().TypeId);
            if(PrivateLimits[index]) --_privateCounts[index]; else --_sharedCount;
        }
        PublishCapacity(index);
        (void)(InvokeIf<TEvents>(event) || ...); // Exceptions unwind the lease to the Thread fatal boundary.
    }
    static constexpr EventCapabilityResourceProfile ResourceProfile() noexcept {
        return {sizeof(ThreadCapability),alignof(ThreadCapability),InboxSlots,InboxSlots*sizeof(void*),
            sizeof(_privateCounts),sizeof(_sharedCount),sizeof(_targets),sizeof(_listeners)};
    }
    std::size_t Pending() const noexcept { return _count.load(std::memory_order_acquire); }
};
}
