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
 * Inherited Memory Total: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members:
 * - Snapshot (ThreadSnapshot): sizeof(ThreadSnapshot) [0 bytes dynamic allocation]
 * Total Memory: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr + sizeof(ThreadSnapshot) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
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
 * Inherited Memory Total: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members:
 * - ThreadAddress (uintptr_t): 4 bytes [0 bytes dynamic allocation]
 * - Cause (std::exception_ptr): sizeof(std::exception_ptr) [0 bytes dynamic allocation]
 * Total Memory: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr + 4 bytes known members + sizeof(std::exception_ptr) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
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
 * Inherited Memory Total: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members:
 * - Snapshot (ThreadSnapshot): sizeof(ThreadSnapshot) [0 bytes dynamic allocation]
 * Total Memory: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr + sizeof(ThreadSnapshot) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
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
 * Inherited Memory Total: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members:
 * - Snapshot (ThreadSnapshot): sizeof(ThreadSnapshot) [0 bytes dynamic allocation]
 * Total Memory: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr + sizeof(ThreadSnapshot) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
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
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members:
 * - Result (\ public: \ ThreadCleanupResult): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr + 4 bytes known members [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
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
 * Inherited Memory Total: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members:
 * - Result (ThreadCleanupResult): 4 bytes [0 bytes dynamic allocation]
 * - Cause (std::exception_ptr): sizeof(std::exception_ptr) [0 bytes dynamic allocation]
 * Total Memory: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr + 4 bytes known members + sizeof(std::exception_ptr) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
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
 * Inherited Memory Total: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members:
 * - Result (ThreadInitializationResult): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr + 4 bytes known members [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
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
 * Inherited Memory Total: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members:
 * - Available (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr + 1 bytes known members [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
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
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members:
 * - Snapshot (\ public: \ ThreadSnapshot): sizeof(\ public: \ ThreadSnapshot) [0 bytes dynamic allocation]
 * Total Memory: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr + sizeof(\ public: \ ThreadSnapshot) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
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