#pragma once
#include "ESPressio_EventTypeRuntime.hpp"
namespace ESPressio::Event {
/// Adapter-owned fixed target seam. The adapter validates its generic substrate's
/// policy/format/capacity proof before Start and synchronously encodes into its
/// owned bytes during admission. Event owns no physical transport or retry engine.
template<class T> class EventOutboundBinding final {
    static_assert(T::IsTransmissibleEvent && T::ValidateTier(),"Outbound Event bindings require a Transmissible Type");
    EventTargetNode _node;
    std::atomic<bool> _closed{true};
    bool _configured=false;
    void* _owner=nullptr;
    EventTargetAdmission (*_admit)(void*,const EventLease&) noexcept=nullptr;
    bool (*_validate)(void*) noexcept=nullptr;
public:
    EventOutboundBinding() noexcept = default;
    EventOutboundBinding(const EventOutboundBinding&)=delete;
    EventOutboundBinding& operator=(const EventOutboundBinding&)=delete;
    ~EventOutboundBinding() { Shutdown(); }
    /// Owner is borrowed and must outlive this binding or explicitly Shutdown it
    /// before destroying resources used by the fixed framework admission thunk.
    template<class Owner,EventTargetAdmission (Owner::*Admit)(const EventLease&) noexcept,
             bool (Owner::*Validate)() noexcept>
    EventRuntimeStatus Initialize(Owner& owner) noexcept {
        if(_configured) return EventRuntimeStatus::AlreadyInitialized;
        _owner=&owner;
        _admit=[](void* p,const EventLease& event) noexcept { return (static_cast<Owner*>(p)->*Admit)(event); };
        _validate=[](void* p) noexcept { return (static_cast<Owner*>(p)->*Validate)(); };
        _node.Owner=this;_node.Class=EventTargetClass::ExternalAdapter;
        _node.Validate=[](void* p) noexcept { auto& self=*static_cast<EventOutboundBinding*>(p); return !self._closed && self._validate(self._owner); };
        _node.Quiesce=[](void* p) noexcept { auto& self=*static_cast<EventOutboundBinding*>(p);self._closed=true;self._node.NotifyCapacityChanged(); };
        _node.TryAdmit=[](void* p,const EventLease& event) noexcept {
            auto& self=*static_cast<EventOutboundBinding*>(p);
            if(self._closed) return EventTargetAdmission::Quiesced;
            auto retained=event.Retain();
            return self._admit(self._owner,retained);
        };
        const auto status=EventTypeRuntime<T>::Get().StageTarget(_node);
        if(status!=EventRuntimeStatus::Success) return status;
        _configured=true;_closed=false;return status;
    }
    /// Publish after the generic adapter releases capacity. This wakes only the
    /// owed Type lane; it does not execute a callback or create another campaign.
    void NotifyCapacityChanged() const noexcept { _node.NotifyCapacityChanged(); }
    void Shutdown() noexcept {
        if(!_configured) return;
        _closed=true;_node.NotifyCapacityChanged();
        EventTypeRuntime<T>::Get().RemoveTarget(_node); // Joins any in-flight bounded admission traversal.
        _configured=false;
    }
};
}
