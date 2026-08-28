#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <utility>

#include <ESPressio_IObservable.hpp>
#include <ESPressio_Memory.hpp>
#include <ESPressio_TimeTraits.hpp>

#include "ESPressio_IEvent.hpp"
#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventObserver.hpp"
#include "ESPressio_EventTypeKey.hpp"

namespace ESPressio {
namespace Event {

/// <summary>RAII handle controlling one event-listener registration.</summary>
class IEventListenerHandle {
public:
    virtual ~IEventListenerHandle() = default;
    /// <summary>Unregisters the associated listener if it is still active.</summary>
    virtual void Unregister() = 0;
    /// <summary>Indicates whether the associated registration remains active.</summary>
    virtual bool IsRegistered() const = 0;
};

/// <summary>Owning pointer type for event-listener registration handles.</summary>
using EventListenerHandlePtr = std::unique_ptr<IEventListenerHandle>;

/// <summary>Contract for registering callbacks or typed observers against event type identities.</summary>
class IEventListener {
public:
    virtual ~IEventListener() = default;

    /// <summary>Registers a type-erased event callback for the supplied event type key.</summary>
    /// <param name="eventType">RTTI-free event type identity to listen for.</param>
    /// <param name="callback">Callback invoked for matching events.</param>
    /// <param name="interest">Filter policy controlling which matching events are delivered.</param>
    /// <param name="maximumTimeSinceDispatch">Age threshold used by YoungerThan filtering.</param>
    /// <param name="customInterestCallback">Predicate used by Custom filtering.</param>
    virtual EventListenerHandlePtr RegisterListener(
        EventTypeKey eventType,
        std::function<void(IEvent*, EventDispatchMethod, EventPriority)> callback,
        EventListenerInterest interest = EventListenerInterest::All,
        EventTime maximumTimeSinceDispatch = EventTime(0),
        std::function<bool(IEvent*)> customInterestCallback = nullptr
    ) = 0;

    /// <summary>Registers a strongly typed callback for a concrete event type.</summary>
    template<typename EventType>
    EventListenerHandlePtr RegisterListener(
        std::function<void(EventType*, EventDispatchMethod, EventPriority)> callback,
        EventListenerInterest interest = EventListenerInterest::All,
        EventTime maximumTimeSinceDispatch = EventTime(0),
        std::function<bool(EventType*)> customInterestCallback = nullptr
    ) {
        std::function<bool(IEvent*)> erasedInterest;
        if (customInterestCallback) {
            erasedInterest = [customInterestCallback = std::move(customInterestCallback)](IEvent* event) {
                return customInterestCallback(static_cast<EventType*>(event));
            };
        }
        return RegisterTypedListener<EventType>(
            [callback = std::move(callback)](IEvent* event, EventDispatchMethod method, EventPriority priority) {
                callback(static_cast<EventType*>(event), method, priority);
            },
            interest,
            maximumTimeSinceDispatch,
            std::move(erasedInterest)
        );
    }

    /// <summary>Registers a typed event observer and adapts its callbacks to the listener contract.</summary>
    template<typename EventType>
    EventListenerHandlePtr RegisterObserver(
        IEventObserver<EventType>* observer,
        EventListenerInterest interest = EventListenerInterest::All,
        EventTime maximumTimeSinceDispatch = EventTime(0)
    ) {
        if (observer == nullptr) throw Observable::InvalidObserverRegistrationException();
        std::function<bool(EventType*)> customInterest;
        if (interest == EventListenerInterest::Custom) {
            customInterest = [observer](EventType* event) { return observer->IsInterestedInEvent(event); };
        }
        return RegisterListener<EventType>(
            [observer](EventType* event, EventDispatchMethod method, EventPriority priority) {
                observer->OnEvent(event, method, priority);
            },
            interest,
            maximumTimeSinceDispatch,
            std::move(customInterest)
        );
    }

    /// <summary>Unregisters a listener handle for the supplied event type.</summary>
    virtual void UnregisterListener(EventTypeKey eventType, IEventListenerHandle* handler) = 0;

    /// <summary>Unregisters a listener handle for a concrete event type.</summary>
    template<typename EventType>
    void UnregisterListener(IEventListenerHandle* handler) {
        UnregisterListener(EventTypeKeyOf<EventType>(), handler);
    }

protected:
    /// <summary>Registers a typed listener after its callback and optional filter have been type-erased.</summary>
    virtual EventListenerHandlePtr RegisterTypedListenerErased(
        EventTypeKey eventType,
        std::function<void(IEvent*, EventDispatchMethod, EventPriority)> callback,
        EventListenerInterest interest,
        EventTime maximumTimeSinceDispatch,
        std::function<bool(IEvent*)> customInterestCallback
    ) = 0;

    /// <summary>Resolves a concrete event type to its stable key and forwards registration to the erased implementation.</summary>
    template<typename EventType>
    EventListenerHandlePtr RegisterTypedListener(
        std::function<void(IEvent*, EventDispatchMethod, EventPriority)> callback,
        EventListenerInterest interest,
        EventTime maximumTimeSinceDispatch,
        std::function<bool(IEvent*)> customInterestCallback
    ) {
        return RegisterTypedListenerErased(
            EventTypeKeyOf<EventType>(), std::move(callback), interest,
            maximumTimeSinceDispatch, std::move(customInterestCallback)
        );
    }
};

/// <summary>Concrete RAII listener handle allocated from ExternalPreferred memory.</summary>
class EventListenerHandle final : public IEventListenerHandle {
private:
    std::atomic<bool> _isRegistered{true};
    EventTypeKey _eventType = nullptr;
    IEventListener* _listener = nullptr;

public:
    static void* operator new(std::size_t bytes) {
        return System::Memory::GetProvider().Allocate(
            bytes,
            alignof(EventListenerHandle),
            System::Memory::MemoryPolicy::ExternalPreferred
        );
    }
    static void operator delete(void* pointer) noexcept {
        System::Memory::GetProvider().Deallocate(
            pointer,
            sizeof(EventListenerHandle),
            alignof(EventListenerHandle),
            System::Memory::MemoryPolicy::ExternalPreferred
        );
    }

    /// <summary>Creates a handle associated with one event type and listener owner.</summary>
    EventListenerHandle(EventTypeKey eventType, IEventListener* listener)
        : _eventType(eventType), _listener(listener) {}

    ~EventListenerHandle() noexcept override {
        try { Unregister(); } catch (...) { ForceUnregister(); }
    }

    /// <inheritdoc/>
    void Unregister() override {
        if (!_isRegistered.load(std::memory_order_acquire) || _listener == nullptr) return;
        _listener->UnregisterListener(_eventType, this);
    }

    /// <inheritdoc/>
    bool IsRegistered() const override { return _isRegistered.load(std::memory_order_acquire); }

    /// <summary>Marks the handle unregistered without invoking its listener owner.</summary>
    void ForceUnregister() noexcept {
        _isRegistered.store(false, std::memory_order_release);
        _listener = nullptr;
    }
};

/// <summary>Thread-safe listener registry that filters and invokes callbacks for matching event types.</summary>
class EventListener : public IEventListener {
private:
    struct ListenerRecord {
        EventTypeKey EventType = nullptr;
        IEventListenerHandle* Handler = nullptr;
        std::function<void(IEvent*, EventDispatchMethod, EventPriority)> Callback;
        EventListenerInterest Interest = EventListenerInterest::All;
        uint64_t MaximumTimeSinceDispatchNanoseconds = 0;
        std::function<bool(IEvent*)> CustomInterest;
        bool Active = false;
    };

    using ListenerStorage = System::Memory::Deque<
        ListenerRecord,
        System::Memory::MemoryPolicy::ExternalPreferred
    >;
    using RemovedTypeStorage = System::Memory::Vector<
        EventTypeKey,
        System::Memory::MemoryPolicy::ExternalPreferred
    >;

    ListenerStorage _listeners;
    mutable std::recursive_mutex _listenersMutex;
    std::size_t _processingDepth = 0;
    bool _needsCompaction = false;

    bool HasActiveListenerLocked(EventTypeKey type) const {
        for (const auto& listener : _listeners) if (listener.Active && listener.EventType == type) return true;
        return false;
    }

    void CompactLocked() {
        _listeners.erase(std::remove_if(_listeners.begin(), _listeners.end(),
            [](const ListenerRecord& listener) { return !listener.Active; }), _listeners.end());
        _needsCompaction = false;
    }

    void FinishProcessingLocked() {
        if (_processingDepth > 0) --_processingDepth;
        if (_processingDepth == 0 && _needsCompaction) CompactLocked();
    }

    bool MatchesEvent(const ListenerRecord& listener, IEvent* event) const {
        return listener.EventType == event->__getTypeKey();
    }

    bool IsInterested(const ListenerRecord& listener, IEvent* event) const {
        switch (listener.Interest) {
            case EventListenerInterest::All: return true;
            case EventListenerInterest::YoungerThan:
                return event->GetTimeSinceDispatchNanoseconds() < listener.MaximumTimeSinceDispatchNanoseconds;
            case EventListenerInterest::Custom:
                return listener.CustomInterest && listener.CustomInterest(event);
            default: return false;
        }
    }

protected:
    /// <summary>Hook invoked when the first active listener for an event type is registered.</summary>
    virtual void OnListenerRegistered(EventTypeKey) {}
    /// <summary>Hook invoked when the last active listener for an event type is removed.</summary>
    virtual void OnListenerUnregistered(EventTypeKey) {}

    /// <inheritdoc/>
    EventListenerHandlePtr RegisterTypedListenerErased(
        EventTypeKey eventType,
        std::function<void(IEvent*, EventDispatchMethod, EventPriority)> callback,
        EventListenerInterest interest,
        EventTime maximumTimeSinceDispatch,
        std::function<bool(IEvent*)> customInterestCallback
    ) override {
        if (eventType == nullptr || !callback) return {};
        std::unique_ptr<EventListenerHandle> handler(new EventListenerHandle(eventType, this));
        bool firstListener = false;
        {
            std::lock_guard<std::recursive_mutex> lock(_listenersMutex);
            firstListener = !HasActiveListenerLocked(eventType);
            _listeners.emplace_back(ListenerRecord{
                eventType,
                handler.get(),
                std::move(callback),
                interest,
                Timing::TimeTraits<EventTime>::template ToNanoseconds<uint64_t>(maximumTimeSinceDispatch),
                std::move(customInterestCallback),
                true
            });
        }
        if (firstListener) OnListenerRegistered(eventType);
        return EventListenerHandlePtr(handler.release());
    }

    /// <summary>Unregisters every active listener and notifies derived types for each event type removed.</summary>
    void UnregisterAllListeners() noexcept {
        RemovedTypeStorage removedTypes;
        {
            std::lock_guard<std::recursive_mutex> lock(_listenersMutex);
            removedTypes.reserve(_listeners.size());
            for (auto& listener : _listeners) {
                if (!listener.Active) continue;
                bool typeAlreadyRecorded = false;
                for (EventTypeKey type : removedTypes) {
                    if (type == listener.EventType) { typeAlreadyRecorded = true; break; }
                }
                if (!typeAlreadyRecorded) removedTypes.push_back(listener.EventType);
                static_cast<EventListenerHandle*>(listener.Handler)->ForceUnregister();
                listener.Active = false;
            }
            if (_processingDepth == 0) {
                _listeners.clear();
                _needsCompaction = false;
            } else {
                _needsCompaction = true;
            }
        }
        for (EventTypeKey type : removedTypes) {
            try { OnListenerUnregistered(type); } catch (...) {}
        }
    }

public:
    using IEventListener::RegisterListener;
    using IEventListener::RegisterObserver;
    using IEventListener::UnregisterListener;

    ~EventListener() override { UnregisterAllListeners(); }

    /// <inheritdoc/>
    EventListenerHandlePtr RegisterListener(
        EventTypeKey eventType,
        std::function<void(IEvent*, EventDispatchMethod, EventPriority)> callback,
        EventListenerInterest interest = EventListenerInterest::All,
        EventTime maximumTimeSinceDispatch = EventTime(0),
        std::function<bool(IEvent*)> customInterestCallback = nullptr
    ) override {
        return RegisterTypedListenerErased(eventType, std::move(callback), interest,
            maximumTimeSinceDispatch, std::move(customInterestCallback));
    }

    /// <inheritdoc/>
    void UnregisterListener(EventTypeKey eventType, IEventListenerHandle* handler) override {
        if (handler == nullptr) return;
        bool removed = false;
        bool removedLast = false;
        {
            std::lock_guard<std::recursive_mutex> lock(_listenersMutex);
            for (auto& listener : _listeners) {
                if (listener.Active && listener.EventType == eventType && listener.Handler == handler) {
                    static_cast<EventListenerHandle*>(handler)->ForceUnregister();
                    listener.Active = false;
                    removed = true;
                    break;
                }
            }
            if (!removed) return;
            removedLast = !HasActiveListenerLocked(eventType);
            if (_processingDepth > 0) _needsCompaction = true;
            else CompactLocked();
        }
        if (removedLast) OnListenerUnregistered(eventType);
    }

    /// <summary>Delivers one event to all active matching listeners whose interest filters accept it.</summary>
    void ProcessEvent(IEvent* event, EventDispatchMethod dispatchMethod, EventPriority priority) {
        if (event == nullptr) return;
        std::lock_guard<std::recursive_mutex> lock(_listenersMutex);
        ++_processingDepth;
        const std::size_t listenerCount = _listeners.size();
        try {
            for (std::size_t index = 0; index < listenerCount; ++index) {
                ListenerRecord& listener = _listeners[index];
                if (!listener.Active || !MatchesEvent(listener, event) || !IsInterested(listener, event)) continue;
                listener.Callback(event, dispatchMethod, priority);
            }
        } catch (...) {
            FinishProcessingLocked();
            throw;
        }
        FinishProcessingLocked();
    }
};

} // namespace Event
} // namespace ESPressio
