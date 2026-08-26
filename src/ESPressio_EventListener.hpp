#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <ESPressio_IObservable.hpp>
#include <ESPressio_TimeTraits.hpp>

#include "ESPressio_IEvent.hpp"
#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventObserver.hpp"
#include "ESPressio_EventTypeKey.hpp"

namespace ESPressio {
namespace Event {

class IEventListenerHandle {
public:
    virtual ~IEventListenerHandle() = default;
    virtual void Unregister() = 0;
    virtual bool IsRegistered() const = 0;
};

using EventListenerHandlePtr = std::unique_ptr<IEventListenerHandle>;

class IEventListener {
public:
    virtual ~IEventListener() = default;

    virtual EventListenerHandlePtr RegisterListener(
        EventTypeKey eventType,
        std::function<void(IEvent*, EventDispatchMethod, EventPriority)> callback,
        EventListenerInterest interest = EventListenerInterest::All,
        EventTime maximumTimeSinceDispatch = EventTime(0),
        std::function<bool(IEvent*)> customInterestCallback = nullptr
    ) = 0;

    template<typename EventType>
    EventListenerHandlePtr RegisterListener(
        std::function<void(EventType*, EventDispatchMethod, EventPriority)> callback,
        EventListenerInterest interest = EventListenerInterest::All,
        EventTime maximumTimeSinceDispatch = EventTime(0),
        std::function<bool(EventType*)> customInterestCallback = nullptr
    ) {
        std::function<bool(IEvent*)> erasedInterest;
        if (customInterestCallback) {
            erasedInterest = [customInterestCallback = std::move(customInterestCallback)](
                IEvent* event
            ) {
                return customInterestCallback(static_cast<EventType*>(event));
            };
        }

        return RegisterTypedListener<EventType>(
            [callback = std::move(callback)](
                IEvent* event,
                EventDispatchMethod method,
                EventPriority priority
            ) {
                callback(static_cast<EventType*>(event), method, priority);
            },
            interest,
            maximumTimeSinceDispatch,
            std::move(erasedInterest)
        );
    }

    template<typename EventType>
    EventListenerHandlePtr RegisterObserver(
        IEventObserver<EventType>* observer,
        EventListenerInterest interest = EventListenerInterest::All,
        EventTime maximumTimeSinceDispatch = EventTime(0)
    ) {
        if (observer == nullptr) {
            throw Observable::InvalidObserverRegistrationException();
        }

        std::function<bool(EventType*)> customInterest;
        if (interest == EventListenerInterest::Custom) {
            customInterest = [observer](EventType* event) {
                return observer->IsInterestedInEvent(event);
            };
        }

        return RegisterListener<EventType>(
            [observer](
                EventType* event,
                EventDispatchMethod method,
                EventPriority priority
            ) {
                observer->OnEvent(event, method, priority);
            },
            interest,
            maximumTimeSinceDispatch,
            std::move(customInterest)
        );
    }

    virtual void UnregisterListener(
        EventTypeKey eventType,
        IEventListenerHandle* handler
    ) = 0;

    template<typename EventType>
    void UnregisterListener(IEventListenerHandle* handler) {
        UnregisterListener(EventTypeKeyOf<EventType>(), handler);
    }

protected:
    virtual EventListenerHandlePtr RegisterTypedListenerErased(
        EventTypeKey eventType,
        std::function<void(IEvent*, EventDispatchMethod, EventPriority)> callback,
        EventListenerInterest interest,
        EventTime maximumTimeSinceDispatch,
        std::function<bool(IEvent*)> customInterestCallback
    ) = 0;

    template<typename EventType>
    EventListenerHandlePtr RegisterTypedListener(
        std::function<void(IEvent*, EventDispatchMethod, EventPriority)> callback,
        EventListenerInterest interest,
        EventTime maximumTimeSinceDispatch,
        std::function<bool(IEvent*)> customInterestCallback
    ) {
        return RegisterTypedListenerErased(
            EventTypeKeyOf<EventType>(),
            std::move(callback),
            interest,
            maximumTimeSinceDispatch,
            std::move(customInterestCallback)
        );
    }
};

class EventListenerHandle final : public IEventListenerHandle {
private:
    std::atomic<bool> _isRegistered{true};
    EventTypeKey _eventType = nullptr;
    IEventListener* _listener = nullptr;

public:
    EventListenerHandle(EventTypeKey eventType, IEventListener* listener)
        : _eventType(eventType), _listener(listener) {}

    ~EventListenerHandle() noexcept override {
        try {
            Unregister();
        } catch (...) {
            ForceUnregister();
        }
    }

    void Unregister() override {
        if (!_isRegistered.load(std::memory_order_acquire) || _listener == nullptr) {
            return;
        }
        _listener->UnregisterListener(_eventType, this);
    }

    bool IsRegistered() const override {
        return _isRegistered.load(std::memory_order_acquire);
    }

    void ForceUnregister() noexcept {
        _isRegistered.store(false, std::memory_order_release);
        _listener = nullptr;
    }
};

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

    // deque is intentional here. Registrations may occur re-entrantly from a
    // callback. push_back() preserves references to existing records, allowing
    // ProcessEvent() to invoke the stored std::function in-place instead of
    // copying it merely to survive a vector reallocation.
    std::deque<ListenerRecord> _listeners;
    mutable std::recursive_mutex _listenersMutex;
    std::size_t _processingDepth = 0;
    bool _needsCompaction = false;

    bool HasActiveListenerLocked(EventTypeKey type) const {
        for (const auto& listener : _listeners) {
            if (listener.Active && listener.EventType == type) return true;
        }
        return false;
    }

    void CompactLocked() {
        _listeners.erase(
            std::remove_if(
                _listeners.begin(),
                _listeners.end(),
                [](const ListenerRecord& listener) {
                    return !listener.Active;
                }
            ),
            _listeners.end()
        );
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
            case EventListenerInterest::All:
                return true;
            case EventListenerInterest::YoungerThan:
                return event->GetTimeSinceDispatchNanoseconds() <
                    listener.MaximumTimeSinceDispatchNanoseconds;
            case EventListenerInterest::Custom:
                return listener.CustomInterest && listener.CustomInterest(event);
            default:
                return false;
        }
    }

protected:
    virtual void OnListenerRegistered(EventTypeKey) {}
    virtual void OnListenerUnregistered(EventTypeKey) {}

    EventListenerHandlePtr RegisterTypedListenerErased(
        EventTypeKey eventType,
        std::function<void(IEvent*, EventDispatchMethod, EventPriority)> callback,
        EventListenerInterest interest,
        EventTime maximumTimeSinceDispatch,
        std::function<bool(IEvent*)> customInterestCallback
    ) override {
        if (eventType == nullptr || !callback) return {};

        std::unique_ptr<EventListenerHandle> handler(
            new EventListenerHandle(eventType, this)
        );

        bool firstListener = false;
        {
            std::lock_guard<std::recursive_mutex> lock(_listenersMutex);
            firstListener = !HasActiveListenerLocked(eventType);
            _listeners.emplace_back(ListenerRecord{
                eventType,
                handler.get(),
                std::move(callback),
                interest,
                Timing::TimeTraits<EventTime>::template ToNanoseconds<uint64_t>(
                    maximumTimeSinceDispatch
                ),
                std::move(customInterestCallback),
                true
            });
        }

        if (firstListener) OnListenerRegistered(eventType);
        return EventListenerHandlePtr(handler.release());
    }

    void UnregisterAllListeners() noexcept {
        std::vector<EventTypeKey> removedTypes;
        {
            std::lock_guard<std::recursive_mutex> lock(_listenersMutex);
            removedTypes.reserve(_listeners.size());
            for (auto& listener : _listeners) {
                if (!listener.Active) continue;
                bool typeAlreadyRecorded = false;
                for (EventTypeKey type : removedTypes) {
                    if (type == listener.EventType) {
                        typeAlreadyRecorded = true;
                        break;
                    }
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
            try {
                OnListenerUnregistered(type);
            } catch (...) {}
        }
    }

public:
    using IEventListener::RegisterListener;
    using IEventListener::RegisterObserver;
    using IEventListener::UnregisterListener;

    ~EventListener() override {
        UnregisterAllListeners();
    }

    EventListenerHandlePtr RegisterListener(
        EventTypeKey eventType,
        std::function<void(IEvent*, EventDispatchMethod, EventPriority)> callback,
        EventListenerInterest interest = EventListenerInterest::All,
        EventTime maximumTimeSinceDispatch = EventTime(0),
        std::function<bool(IEvent*)> customInterestCallback = nullptr
    ) override {
        return RegisterTypedListenerErased(
            eventType,
            std::move(callback),
            interest,
            maximumTimeSinceDispatch,
            std::move(customInterestCallback)
        );
    }

    void UnregisterListener(
        EventTypeKey eventType,
        IEventListenerHandle* handler
    ) override {
        if (handler == nullptr) return;

        bool removed = false;
        bool removedLast = false;
        {
            std::lock_guard<std::recursive_mutex> lock(_listenersMutex);
            for (auto& listener : _listeners) {
                if (
                    listener.Active &&
                    listener.EventType == eventType &&
                    listener.Handler == handler
                ) {
                    static_cast<EventListenerHandle*>(handler)->ForceUnregister();
                    listener.Active = false;
                    removed = true;
                    break;
                }
            }

            if (!removed) return;
            removedLast = !HasActiveListenerLocked(eventType);
            if (_processingDepth > 0) {
                _needsCompaction = true;
            } else {
                CompactLocked();
            }
        }

        if (removedLast) OnListenerUnregistered(eventType);
    }

    void ProcessEvent(
        IEvent* event,
        EventDispatchMethod dispatchMethod,
        EventPriority priority
    ) {
        if (event == nullptr) return;

        std::lock_guard<std::recursive_mutex> lock(_listenersMutex);
        ++_processingDepth;
        const std::size_t listenerCount = _listeners.size();

        try {
            for (std::size_t index = 0; index < listenerCount; ++index) {
                ListenerRecord& listener = _listeners[index];
                if (!listener.Active) continue;
                if (!MatchesEvent(listener, event)) continue;
                if (!IsInterested(listener, event)) continue;

                // Existing deque elements keep stable references when a callback
                // appends a listener. Tombstoning defers erasure until the outermost
                // dispatch completes, so this call needs no std::function copy.
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