#pragma once

#include <ESPressio_IThreadTerminationDispatcherObserver.hpp>
#include <ESPressio_ThreadTerminationDispatcher.hpp>

#include "ESPressio_ThreadEvents.hpp"

namespace ESPressio {
namespace Event {

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members:
 * - _observerHandle (Observable::ObserverHandlePtr): 12 bytes [owned object: 4 bytes]
 * - _initialized (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 20 bytes [_observerHandle: owned object: 4 bytes]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class ThreadTerminationDispatcherEventBridge final :
    public Threads::IThreadTerminationDispatcherObserver {

private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

    ThreadTerminationDispatcherEventBridge() = default;

public:
    ThreadTerminationDispatcherEventBridge(
        const ThreadTerminationDispatcherEventBridge&
    ) = delete;

    ThreadTerminationDispatcherEventBridge& operator=(
        const ThreadTerminationDispatcherEventBridge&
    ) = delete;

    static ThreadTerminationDispatcherEventBridge& GetInstance() {
        static ThreadTerminationDispatcherEventBridge instance;
        return instance;
    }

    bool Initialize() {
        if (_initialized) {
            return true;
        }

        _observerHandle =
            Threads::ThreadTerminationDispatcher::
                GetInstance()->
                RegisterObserver(this);

        _initialized =
            static_cast<bool>(_observerHandle);

        return _initialized;
    }

    void Shutdown() {
        _observerHandle.reset();
        _initialized = false;
    }

    bool IsInitialized() const {
        return _initialized;
    }

    void OnThreadTerminationDispatcherInitialized(
        bool available
    ) override {
        (new ThreadTerminationDispatcherInitializedEvent(available))->Queue();
    }

    void OnThreadTerminationDispatchQueued(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new ThreadTerminationDispatchQueuedEvent(snapshot))->Queue();
    }

    void OnThreadTerminationDispatchQueueFailed(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new ThreadTerminationDispatchQueueFailedEvent(snapshot))->Queue();
    }

    void OnThreadTerminationDispatchStarted(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new ThreadTerminationDispatchStartedEvent(snapshot))->Queue();
    }

    void OnThreadTerminationDispatchCompleted(
        const Threads::ThreadManagerThreadSnapshot& snapshot
    ) override {
        (new ThreadTerminationDispatchCompletedEvent(snapshot))->Queue();
    }
};

}
}
