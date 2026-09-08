#pragma once

#include <cstddef>
#include <cstdint>
#include <exception>

#include <ESPressio_ClockSynchronization.hpp>
#include <ESPressio_Event.hpp>

namespace ESPressio {
namespace Event {

using TimingClockTick = Timing::ClockTick;
using SynchronizationResult = Timing::ClockSynchronizationResult<TimingClockTick>;
using SynchronizationStatus = Timing::ClockSynchronizationStatus<TimingClockTick>;

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - PreviousTimeNanoseconds (TimingClockTick): 8 bytes [0 bytes dynamic allocation]
 * - NewTimeNanoseconds (TimingClockTick): 8 bytes [0 bytes dynamic allocation]
 * - DifferenceNanoseconds (int64_t): 8 bytes [0 bytes dynamic allocation]
 * Total Memory: 48 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SystemClockTimeChangedEvent final : public TypedEvent<SystemClockTimeChangedEvent> {
public:
    const TimingClockTick PreviousTimeNanoseconds;
    const TimingClockTick NewTimeNanoseconds;
    const int64_t DifferenceNanoseconds;
    SystemClockTimeChangedEvent(TimingClockTick previousTime, TimingClockTick newTime, int64_t difference)
        : PreviousTimeNanoseconds(previousTime), NewTimeNanoseconds(newTime), DifferenceNanoseconds(difference) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - ClockBeforeNanoseconds (TimingClockTick): 8 bytes [0 bytes dynamic allocation]
 * - ClockAfterNanoseconds (TimingClockTick): 8 bytes [0 bytes dynamic allocation]
 * - ImmediateDifferenceNanoseconds (int64_t): 8 bytes [0 bytes dynamic allocation]
 * - Result (SynchronizationResult): 44 bytes [0 bytes dynamic allocation]
 * - Status (SynchronizationStatus): 72 bytes [0 bytes dynamic allocation]
 * Total Memory: 164 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SynchronizationSampleAcceptedEvent final : public TypedEvent<SynchronizationSampleAcceptedEvent> {
public:
    const TimingClockTick ClockBeforeNanoseconds;
    const TimingClockTick ClockAfterNanoseconds;
    const int64_t ImmediateDifferenceNanoseconds;
    const SynchronizationResult Result;
    const SynchronizationStatus Status;
    SynchronizationSampleAcceptedEvent(TimingClockTick before, TimingClockTick after, int64_t difference,
        const SynchronizationResult& result, const SynchronizationStatus& status)
        : ClockBeforeNanoseconds(before), ClockAfterNanoseconds(after), ImmediateDifferenceNanoseconds(difference),
          Result(result), Status(status) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - Result (SynchronizationResult): 44 bytes [0 bytes dynamic allocation]
 * - Status (SynchronizationStatus): 72 bytes [0 bytes dynamic allocation]
 * Total Memory: 140 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SynchronizationSampleRejectedEvent final : public TypedEvent<SynchronizationSampleRejectedEvent> {
public:
    const SynchronizationResult Result;
    const SynchronizationStatus Status;
    SynchronizationSampleRejectedEvent(const SynchronizationResult& result, const SynchronizationStatus& status)
        : Result(result), Status(status) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - ClockBeforeNanoseconds (TimingClockTick): 8 bytes [0 bytes dynamic allocation]
 * - ClockAfterNanoseconds (TimingClockTick): 8 bytes [0 bytes dynamic allocation]
 * - ImmediateDifferenceNanoseconds (int64_t): 8 bytes [0 bytes dynamic allocation]
 * - Result (SynchronizationResult): 44 bytes [0 bytes dynamic allocation]
 * - Status (SynchronizationStatus): 72 bytes [0 bytes dynamic allocation]
 * Total Memory: 164 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SystemClockSynchronizedEvent final : public TypedEvent<SystemClockSynchronizedEvent> {
public:
    const TimingClockTick ClockBeforeNanoseconds;
    const TimingClockTick ClockAfterNanoseconds;
    const int64_t ImmediateDifferenceNanoseconds;
    const SynchronizationResult Result;
    const SynchronizationStatus Status;
    SystemClockSynchronizedEvent(TimingClockTick before, TimingClockTick after, int64_t difference,
        const SynchronizationResult& result, const SynchronizationStatus& status)
        : ClockBeforeNanoseconds(before), ClockAfterNanoseconds(after), ImmediateDifferenceNanoseconds(difference),
          Result(result), Status(status) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - PreviousState (Timing::ClockSynchronizationState): 1 bytes [0 bytes dynamic allocation]
 * - NewState (Timing::ClockSynchronizationState): 1 bytes [0 bytes dynamic allocation]
 * - Status (SynchronizationStatus): 72 bytes [0 bytes dynamic allocation]
 * Total Memory: 100 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SynchronizationStateChangedEvent final : public TypedEvent<SynchronizationStateChangedEvent> {
public:
    const Timing::ClockSynchronizationState PreviousState;
    const Timing::ClockSynchronizationState NewState;
    const SynchronizationStatus Status;
    SynchronizationStateChangedEvent(Timing::ClockSynchronizationState previousState,
        Timing::ClockSynchronizationState newState, const SynchronizationStatus& status)
        : PreviousState(previousState), NewState(newState), Status(status) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - PreviousStatus (SynchronizationStatus): 72 bytes [0 bytes dynamic allocation]
 * - NewStatus (SynchronizationStatus): 72 bytes [0 bytes dynamic allocation]
 * Total Memory: 168 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SynchronizationResetEvent final : public TypedEvent<SynchronizationResetEvent> {
public:
    const SynchronizationStatus PreviousStatus;
    const SynchronizationStatus NewStatus;
    SynchronizationResetEvent(const SynchronizationStatus& previousStatus, const SynchronizationStatus& newStatus)
        : PreviousStatus(previousStatus), NewStatus(newStatus) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - PreviousConfig (Timing::ClockSynchronizationConfig): 76 bytes [0 bytes dynamic allocation]
 * - NewConfig (Timing::ClockSynchronizationConfig): 76 bytes [0 bytes dynamic allocation]
 * Total Memory: 176 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SynchronizationConfigurationChangedEvent final : public TypedEvent<SynchronizationConfigurationChangedEvent> {
public:
    const Timing::ClockSynchronizationConfig PreviousConfig;
    const Timing::ClockSynchronizationConfig NewConfig;
    SynchronizationConfigurationChangedEvent(const Timing::ClockSynchronizationConfig& previousConfig,
        const Timing::ClockSynchronizationConfig& newConfig)
        : PreviousConfig(previousConfig), NewConfig(newConfig) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - ScheduledTimeNanoseconds (TimingClockTick): 8 bytes [0 bytes dynamic allocation]
 * Total Memory: 32 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SystemClockCallbackScheduledEvent final : public TypedEvent<SystemClockCallbackScheduledEvent> {
public:
    const TimingClockTick ScheduledTimeNanoseconds;
    explicit SystemClockCallbackScheduledEvent(TimingClockTick scheduled) : ScheduledTimeNanoseconds(scheduled) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - ScheduledTimeNanoseconds (TimingClockTick): 8 bytes [0 bytes dynamic allocation]
 * Total Memory: 32 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SystemClockCallbackScheduleFailedEvent final : public TypedEvent<SystemClockCallbackScheduleFailedEvent> {
public:
    const TimingClockTick ScheduledTimeNanoseconds;
    explicit SystemClockCallbackScheduleFailedEvent(TimingClockTick scheduled) : ScheduledTimeNanoseconds(scheduled) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - ScheduledTimeNanoseconds (TimingClockTick): 8 bytes [0 bytes dynamic allocation]
 * - ActualTimeNanoseconds (TimingClockTick): 8 bytes [0 bytes dynamic allocation]
 * - DifferenceNanoseconds (int64_t): 8 bytes [0 bytes dynamic allocation]
 * Total Memory: 48 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SystemClockCallbackExecutedEvent final : public TypedEvent<SystemClockCallbackExecutedEvent> {
public:
    const TimingClockTick ScheduledTimeNanoseconds;
    const TimingClockTick ActualTimeNanoseconds;
    const int64_t DifferenceNanoseconds;
    SystemClockCallbackExecutedEvent(TimingClockTick scheduled, TimingClockTick actual, int64_t difference)
        : ScheduledTimeNanoseconds(scheduled), ActualTimeNanoseconds(actual), DifferenceNanoseconds(difference) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - ScheduledTimeNanoseconds (TimingClockTick): 8 bytes [0 bytes dynamic allocation]
 * - ActualTimeNanoseconds (TimingClockTick): 8 bytes [0 bytes dynamic allocation]
 * - DifferenceNanoseconds (int64_t): 8 bytes [0 bytes dynamic allocation]
 * - Cause (std::exception_ptr): 4 bytes [referenced exception object/control storage is external]
 * Total Memory: 52 bytes [Cause: referenced exception object/control storage is external]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class SystemClockCallbackExecutionFailedEvent final : public TypedEvent<SystemClockCallbackExecutionFailedEvent> {
public:
    const TimingClockTick ScheduledTimeNanoseconds;
    const TimingClockTick ActualTimeNanoseconds;
    const int64_t DifferenceNanoseconds;
    const std::exception_ptr Cause;
    SystemClockCallbackExecutionFailedEvent(TimingClockTick scheduled, TimingClockTick actual,
        int64_t difference, std::exception_ptr cause)
        : ScheduledTimeNanoseconds(scheduled), ActualTimeNanoseconds(actual),
          DifferenceNanoseconds(difference), Cause(cause) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - ClearedCallbackCount (std::size_t): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 28 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SystemClockCallbacksClearedEvent final : public TypedEvent<SystemClockCallbacksClearedEvent> {
public:
    const std::size_t ClearedCallbackCount;
    explicit SystemClockCallbacksClearedEvent(std::size_t count) : ClearedCallbackCount(count) {}
};

} // namespace Event
} // namespace ESPressio
