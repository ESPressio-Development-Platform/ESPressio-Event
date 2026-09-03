#pragma once

#include <atomic>

#include <ESPressio_Thread.hpp>
#include <ESPressio_EventThreadBase.hpp>

#include "ESPressio_EventListener.hpp"
#include "ESPressio_EventManager.hpp"

namespace ESPressio {
namespace Event {

/// <summary>Marker interface shared by concrete Event-thread variants.</summary>
class IEventThread {};

/// <summary>Dedicated Event-processing thread that combines EventThreadBase with typed listener registration.</summary>
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
    /// <summary>Processes one received Event through the registered listener set with its dispatch provenance.</summary>
    void OnEvent(
        IEvent* event,
        EventDispatchMethod dispatchMethod,
        EventPriority priority,
        const EventDispatchContext& context
    ) override {
        try {
            ProcessEvent(event, dispatchMethod, priority, context);
        } catch (...) {
            StopReceivingEvents();
            throw;
        }
    }

    /// <summary>Connects a newly registered Event type to the global EventManager dispatcher.</summary>
    void OnListenerRegistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->RegisterReceiver(eventType, this);
    }

    /// <summary>Disconnects an Event type from the global EventManager dispatcher.</summary>
    void OnListenerUnregistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->UnregisterReceiver(eventType, this);
    }

public:
    /// <summary>Constructs a dedicated Event thread with the specified release policy.</summary>
    explicit EventThread(Threads::ThreadReleasePolicy releasePolicy)
        : EventThreadBase(releasePolicy) {}

    ~EventThread() override {
        Shutdown();
        StopReceivingEvents();
    }

    /// <summary>Stops accepting Events before requesting thread termination.</summary>
    void Terminate() override {
        StopReceivingEvents();
        EventThreadBase::Terminate();
    }
};

/// <summary>Selects whether queued Events are processed before or after each custom thread-loop iteration.</summary>
enum EventThreadProcessOrder {
    /// <summary>Drain queued Events before invoking the custom loop body.</summary>
    EventsBeforeLoop,
    /// <summary>Invoke the custom loop body before draining queued Events.</summary>
    EventsAfterLoop
};

/// <summary>Thread variant that combines ordinary loop work with Event reception and typed listener dispatch.</summary>
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
    /// <summary>Runs one loop iteration and drains Events according to the configured processing order.</summary>
    void OnLoop() override {
        try {
            if (_processOrder == EventsBeforeLoop) {
                WithEvents(
                    [&](
                        IEvent* event,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority,
                        const EventDispatchContext& context
                    ) {
                        ProcessEvent(event, dispatchMethod, priority, context);
                    }
                );
            }

            OnThreadLoop();

            if (_processOrder == EventsAfterLoop) {
                WithEvents(
                    [&](
                        IEvent* event,
                        EventDispatchMethod dispatchMethod,
                        EventPriority priority,
                        const EventDispatchContext& context
                    ) {
                        ProcessEvent(event, dispatchMethod, priority, context);
                    }
                );
            }
        } catch (...) {
            StopReceivingEvents();
            throw;
        }
    }

    /// <summary>Executes application-specific non-Event work for one thread-loop iteration.</summary>
    virtual void OnThreadLoop() = 0;

    /// <summary>Connects a newly registered Event type to the global EventManager dispatcher.</summary>
    void OnListenerRegistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->RegisterReceiver(eventType, this);
    }

    /// <summary>Disconnects an Event type from the global EventManager dispatcher.</summary>
    void OnListenerUnregistered(EventTypeKey eventType) override {
        EventManager::GetInstance()->UnregisterReceiver(eventType, this);
    }

public:
    /// <summary>Constructs a combined loop/Event thread with the specified release policy.</summary>
    explicit EventThreadWithLoop(Threads::ThreadReleasePolicy releasePolicy)
        : Threads::Thread(releasePolicy) {
        SetPriority(ESPRESSIO_EVENT_THREAD_DEFAULT_PRIORITY);
        SetCoreID(ESPRESSIO_EVENT_THREAD_DEFAULT_CORE_ID);
    }

    ~EventThreadWithLoop() override {
        Shutdown();
        StopReceivingEvents();
    }

    /// <summary>Stops accepting Events before requesting thread termination.</summary>
    void Terminate() override {
        StopReceivingEvents();
        Threads::Thread::Terminate();
    }

    /// <summary>Returns whether Events are processed before or after the custom loop body.</summary>
    EventThreadProcessOrder GetProcessOrder() const {
        return _processOrder;
    }

    /// <summary>Sets whether Events are processed before or after the custom loop body.</summary>
    void SetProcessOrder(EventThreadProcessOrder processOrder) {
        _processOrder = processOrder;
    }
};

} // namespace Event
} // namespace ESPressio
