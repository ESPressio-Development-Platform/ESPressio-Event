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

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - Snapshot (ThreadSnapshot): 16 bytes [0 bytes dynamic allocation]
 * Total Memory: 40 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class ThreadRegisteredEvent final :
    public TypedEvent<ThreadRegisteredEvent> {
public:
    const ThreadSnapshot Snapshot;
    explicit ThreadRegisteredEvent(const ThreadSnapshot& snapshot)
        : Snapshot(snapshot) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - ThreadAddress (uintptr_t): 4 bytes [0 bytes dynamic allocation]
 * - Cause (std::exception_ptr): 4 bytes [referenced exception object/control storage is external]
 * Total Memory: 32 bytes [Cause: referenced exception object/control storage is external]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class ThreadRegistrationFailedEvent final :
    public TypedEvent<ThreadRegistrationFailedEvent> {
public:
    const uintptr_t ThreadAddress;
    const std::exception_ptr Cause;
    ThreadRegistrationFailedEvent(uintptr_t threadAddress, std::exception_ptr cause)
        : ThreadAddress(threadAddress), Cause(cause) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - Snapshot (ThreadSnapshot): 16 bytes [0 bytes dynamic allocation]
 * Total Memory: 40 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class ThreadRemovedEvent final :
    public TypedEvent<ThreadRemovedEvent> {
public:
    const ThreadSnapshot Snapshot;
    explicit ThreadRemovedEvent(const ThreadSnapshot& snapshot)
        : Snapshot(snapshot) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - Snapshot (ThreadSnapshot): 16 bytes [0 bytes dynamic allocation]
 * Total Memory: 40 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
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

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - Result (ThreadCleanupResult): 32 bytes [0 bytes dynamic allocation]
 * - Cause (std::exception_ptr): 4 bytes [referenced exception object/control storage is external]
 * Total Memory: 60 bytes [Cause: referenced exception object/control storage is external]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
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

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - Result (ThreadInitializationResult): 2 bytes [0 bytes dynamic allocation]
 * Total Memory: 28 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class ThreadManagerInitializationCompletedEvent final :
    public TypedEvent<ThreadManagerInitializationCompletedEvent> {
public:
    const ThreadInitializationResult Result;
    explicit ThreadManagerInitializationCompletedEvent(
        const ThreadInitializationResult& result
    ) : Result(result) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - Available (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 28 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
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