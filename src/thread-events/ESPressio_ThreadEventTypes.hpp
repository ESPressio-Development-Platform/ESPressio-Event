#pragma once

#include <cstddef>
#include <cstdint>
#include <exception>

#include <ESPressio_ThreadManagerTypes.hpp>

#include "../ESPressio_Event.hpp"

namespace ESPressio {
namespace Event {

using ThreadSnapshot = Threads::ThreadManagerThreadSnapshot;
using ThreadCleanupResult = Threads::ThreadManagerCleanupResult;
using ThreadInitializationResult = Threads::ThreadManagerInitializationResult;

class ThreadRegisteredEvent final :
    public TypedEvent<ThreadRegisteredEvent> {
public:
    const ThreadSnapshot Snapshot;
    explicit ThreadRegisteredEvent(const ThreadSnapshot& snapshot)
        : Snapshot(snapshot) {}
};

class ThreadRegistrationFailedEvent final :
    public TypedEvent<ThreadRegistrationFailedEvent> {
public:
    const uintptr_t ThreadAddress;
    const std::exception_ptr Cause;
    ThreadRegistrationFailedEvent(uintptr_t threadAddress, std::exception_ptr cause)
        : ThreadAddress(threadAddress), Cause(cause) {}
};

class ThreadRemovedEvent final :
    public TypedEvent<ThreadRemovedEvent> {
public:
    const ThreadSnapshot Snapshot;
    explicit ThreadRemovedEvent(const ThreadSnapshot& snapshot)
        : Snapshot(snapshot) {}
};

class ThreadCleanupClaimedEvent final :
    public TypedEvent<ThreadCleanupClaimedEvent> {
public:
    const ThreadSnapshot Snapshot;
    explicit ThreadCleanupClaimedEvent(const ThreadSnapshot& snapshot)
        : Snapshot(snapshot) {}
};

#define ESPRESSIO_DEFINE_THREAD_CLEANUP_EVENT(CLASS_NAME) \
class CLASS_NAME final : public TypedEvent<CLASS_NAME> { \
public: \
    const ThreadCleanupResult Result; \
    explicit CLASS_NAME(const ThreadCleanupResult& result) : Result(result) {} \
};

ESPRESSIO_DEFINE_THREAD_CLEANUP_EVENT(ThreadCleanupDeferredEvent)
ESPRESSIO_DEFINE_THREAD_CLEANUP_EVENT(ThreadCleanupStartedEvent)
ESPRESSIO_DEFINE_THREAD_CLEANUP_EVENT(ThreadCleanupCompletedEvent)

#undef ESPRESSIO_DEFINE_THREAD_CLEANUP_EVENT

class ThreadCleanupFailedEvent final :
    public TypedEvent<ThreadCleanupFailedEvent> {
public:
    const ThreadCleanupResult Result;
    const std::exception_ptr Cause;
    ThreadCleanupFailedEvent(
        const ThreadCleanupResult& result,
        std::exception_ptr cause
    ) : Result(result), Cause(cause) {}
};

class ThreadManagerInitializationCompletedEvent final :
    public TypedEvent<ThreadManagerInitializationCompletedEvent> {
public:
    const ThreadInitializationResult Result;
    explicit ThreadManagerInitializationCompletedEvent(
        const ThreadInitializationResult& result
    ) : Result(result) {}
};

class ThreadTerminationDispatcherInitializedEvent final :
    public TypedEvent<ThreadTerminationDispatcherInitializedEvent> {
public:
    const bool Available;
    explicit ThreadTerminationDispatcherInitializedEvent(bool available)
        : Available(available) {}
};

#define ESPRESSIO_DEFINE_TERMINATION_EVENT(CLASS_NAME) \
class CLASS_NAME final : public TypedEvent<CLASS_NAME> { \
public: \
    const ThreadSnapshot Snapshot; \
    explicit CLASS_NAME(const ThreadSnapshot& snapshot) : Snapshot(snapshot) {} \
};

ESPRESSIO_DEFINE_TERMINATION_EVENT(ThreadTerminationDispatchQueuedEvent)
ESPRESSIO_DEFINE_TERMINATION_EVENT(ThreadTerminationDispatchQueueFailedEvent)
ESPRESSIO_DEFINE_TERMINATION_EVENT(ThreadTerminationDispatchStartedEvent)
ESPRESSIO_DEFINE_TERMINATION_EVENT(ThreadTerminationDispatchCompletedEvent)

#undef ESPRESSIO_DEFINE_TERMINATION_EVENT

} // namespace Event
} // namespace ESPressio