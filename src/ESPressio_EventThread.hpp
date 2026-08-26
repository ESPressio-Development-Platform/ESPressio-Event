#pragma once

#include <atomic>

#include <ESPressio_Thread.hpp>
#include <ESPressio_EventThreadBase.hpp>

#include "ESPressio_EventListener.hpp"
#include "ESPressio_EventManager.hpp"

namespace ESPressio {
namespace Event {

class IEventThread {};

class EventThread :
    public EventThreadBase,
    public EventListener,
    public IEventThread {
private:
    std::atomic<bool> _acceptingEvents{true};

    void StopReceivingEvents() noexcept {
        if (!_acceptingEvents.exchange(false)) return;
        StopAcceptingEvents();
        try { UnregisterAllListeners(); } catch (...) {}
        ClearPendingEvents();
    }

protected:
    void OnEvent(
        IEvent* event,
        EventDispatchMethod dispatchMethod,
        EventPriority priority
    ) override {
        try {
            ProcessEvent(event, dispatchMethod, priority);
        } catch (...) {
            StopReceivingEvents();
            throw;
        }
    }

    void OnListenerRegistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->RegisterReceiver(eventType, this);
    }

    void OnListenerUnregistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->UnregisterReceiver(eventType, this);
    }

public:
    explicit EventThread(Threads::ThreadReleasePolicy releasePolicy)
        : EventThreadBase(releasePolicy) {}

    ~EventThread() override {
        Shutdown();
        StopReceivingEvents();
    }

    void Terminate() override {
        StopReceivingEvents();
        EventThreadBase::Terminate();
    }
};

enum EventThreadProcessOrder {
    EventsBeforeLoop,
    EventsAfterLoop
};

class EventThreadWithLoop :
    public Threads::Thread,
    public EventReceiver,
    public IEventThreadBase,
    public EventListener,
    public IEventThread {
private:
    EventThreadProcessOrder _processOrder = EventsBeforeLoop;
    std::atomic<bool> _acceptingEvents{true};

    void StopReceivingEvents() noexcept {
        if (!_acceptingEvents.exchange(false)) return;
        StopAcceptingEvents();
        try { UnregisterAllListeners(); } catch (...) {}
        ClearPendingEvents();
    }

protected:
    void OnLoop() override {
        try {
            if (_processOrder == EventsBeforeLoop) {
                WithEvents(
                    [&](
                        IEvent* event,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority
                    ) {
                        ProcessEvent(event, dispatchMethod, priority);
                    }
                );
            }

            OnThreadLoop();

            if (_processOrder == EventsAfterLoop) {
                WithEvents(
                    [&](
                        IEvent* event,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority
                    ) {
                        ProcessEvent(event, dispatchMethod, priority);
                    }
                );
            }
        } catch (...) {
            StopReceivingEvents();
            throw;
        }
    }

    virtual void OnThreadLoop() = 0;

    void OnListenerRegistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->RegisterReceiver(eventType, this);
    }

    void OnListenerUnregistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->UnregisterReceiver(eventType, this);
    }

public:
    explicit EventThreadWithLoop(Threads::ThreadReleasePolicy releasePolicy)
        : Threads::Thread(releasePolicy) {
        SetPriority(ESPRESSIO_EVENT_THREAD_DEFAULT_PRIORITY);
        SetCoreID(ESPRESSIO_EVENT_THREAD_DEFAULT_CORE_ID);
    }

    ~EventThreadWithLoop() override {
        Shutdown();
        StopReceivingEvents();
    }

    void Terminate() override {
        StopReceivingEvents();
        Threads::Thread::Terminate();
    }

    EventThreadProcessOrder GetProcessOrder() const {
        return _processOrder;
    }

    void SetProcessOrder(EventThreadProcessOrder processOrder) {
        _processOrder = processOrder;
    }
};

} // namespace Event
} // namespace ESPressio
