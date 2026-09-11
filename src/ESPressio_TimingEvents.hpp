#pragma once
#include <ESPressio_ClockSynchronization.hpp>
#include "ESPressio_SerializableEvent.hpp"
namespace ESPressio::Event {
// Fixed diagnostic snapshots follow the current Timing contract. They do not
// perform clock service, synthesize qualification or retain transport state.
struct ClockObservationDiagnostic final : Serializable::SerializableBase<ClockObservationDiagnostic> {
    bool Accepted{};
    std::uint8_t Rejection{};
    std::int64_t MeasuredOffsetNanoseconds{};
    std::int64_t ModelPhaseResidualNanoseconds{};
    std::uint64_t RoundTripDelayNanoseconds{};
    double EstimatedFrequencyCorrectionPpm{};
    ClockObservationDiagnostic() noexcept = default;
    explicit ClockObservationDiagnostic(const Timing::ClockSynchronizationResult& value) noexcept {
        Accepted=static_cast<bool>(value.Accepted);
        Rejection=static_cast<std::uint8_t>(value.Rejection);
        MeasuredOffsetNanoseconds=static_cast<std::int64_t>(value.MeasuredOffsetNanoseconds);
        ModelPhaseResidualNanoseconds=static_cast<std::int64_t>(value.ModelPhaseResidualNanoseconds);
        RoundTripDelayNanoseconds=static_cast<std::uint64_t>(value.RoundTripDelayNanoseconds);
        EstimatedFrequencyCorrectionPpm=static_cast<double>(value.EstimatedFrequencyCorrectionPpm);
    }
    ESPRESSIO_SERIALIZABLE_TYPE(ClockObservationDiagnostic)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("accepted",Accepted),
        ESPRESSIO_PROPERTY("rejection",Rejection),
        ESPRESSIO_PROPERTY("measuredOffsetNanoseconds",MeasuredOffsetNanoseconds),
        ESPRESSIO_PROPERTY("modelPhaseResidualNanoseconds",ModelPhaseResidualNanoseconds),
        ESPRESSIO_PROPERTY("roundTripDelayNanoseconds",RoundTripDelayNanoseconds),
        ESPRESSIO_PROPERTY("estimatedFrequencyCorrectionPpm",EstimatedFrequencyCorrectionPpm))
};
struct ClockStatusDiagnostic final : Serializable::SerializableBase<ClockStatusDiagnostic> {
    std::uint8_t Reliability{};
    std::uint64_t ReferenceIdentity{};
    std::uint64_t LastAcceptedSampleMonotonic{};
    std::uint64_t SampleAgeNanoseconds{};
    std::uint64_t LastRoundTripDelayNanoseconds{};
    std::uint64_t ModelResidualEnvelopeNanoseconds{};
    std::int64_t LastMeasuredOffsetNanoseconds{};
    std::int64_t ModelPhaseResidualNanoseconds{};
    std::int64_t PendingPhaseSlewNanoseconds{};
    double EstimatedFrequencyCorrectionPpm{};
    double ResidualFrequencyErrorBoundPpm{};
    bool UncertaintyKnown{};
    std::uint64_t UncertaintyNanoseconds{};
    std::uint64_t RetainedSamples{};
    std::uint64_t InlierSamples{};
    std::uint64_t ObservationSpanNanoseconds{};
    std::uint64_t AcceptedSamples{};
    std::uint64_t RejectedSamples{};
    std::uint64_t SchedulerDeadlineMisses{};
    bool HasSynchronizationDeadline{};
    std::uint64_t NextRequiredSynchronizationMonotonic{};
    ClockStatusDiagnostic() noexcept = default;
    explicit ClockStatusDiagnostic(const Timing::ClockSynchronizationStatus& value) noexcept {
        Reliability=static_cast<std::uint8_t>(value.Reliability);
        ReferenceIdentity=static_cast<std::uint64_t>(value.ReferenceIdentity);
        LastAcceptedSampleMonotonic=static_cast<std::uint64_t>(value.LastAcceptedSampleMonotonic);
        SampleAgeNanoseconds=static_cast<std::uint64_t>(value.SampleAgeNanoseconds);
        LastRoundTripDelayNanoseconds=static_cast<std::uint64_t>(value.LastRoundTripDelayNanoseconds);
        ModelResidualEnvelopeNanoseconds=static_cast<std::uint64_t>(value.ModelResidualEnvelopeNanoseconds);
        LastMeasuredOffsetNanoseconds=static_cast<std::int64_t>(value.LastMeasuredOffsetNanoseconds);
        ModelPhaseResidualNanoseconds=static_cast<std::int64_t>(value.ModelPhaseResidualNanoseconds);
        PendingPhaseSlewNanoseconds=static_cast<std::int64_t>(value.PendingPhaseSlewNanoseconds);
        EstimatedFrequencyCorrectionPpm=static_cast<double>(value.EstimatedFrequencyCorrectionPpm);
        ResidualFrequencyErrorBoundPpm=static_cast<double>(value.ResidualFrequencyErrorBoundPpm);
        UncertaintyKnown=static_cast<bool>(value.CurrentUncertainty.IsKnown);
        UncertaintyNanoseconds=static_cast<std::uint64_t>(value.CurrentUncertainty.Nanoseconds);
        RetainedSamples=static_cast<std::uint64_t>(value.RetainedSamples);
        InlierSamples=static_cast<std::uint64_t>(value.InlierSamples);
        ObservationSpanNanoseconds=static_cast<std::uint64_t>(value.ObservationSpanNanoseconds);
        AcceptedSamples=static_cast<std::uint64_t>(value.AcceptedSamples);
        RejectedSamples=static_cast<std::uint64_t>(value.RejectedSamples);
        SchedulerDeadlineMisses=static_cast<std::uint64_t>(value.SchedulerDeadlineMisses);
        HasSynchronizationDeadline=static_cast<bool>(value.HasSynchronizationDeadline);
        NextRequiredSynchronizationMonotonic=static_cast<std::uint64_t>(value.NextRequiredSynchronizationMonotonic);
    }
    ESPRESSIO_SERIALIZABLE_TYPE(ClockStatusDiagnostic)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("reliability",Reliability),
        ESPRESSIO_PROPERTY("referenceIdentity",ReferenceIdentity),
        ESPRESSIO_PROPERTY("lastAcceptedSampleMonotonic",LastAcceptedSampleMonotonic),
        ESPRESSIO_PROPERTY("sampleAgeNanoseconds",SampleAgeNanoseconds),
        ESPRESSIO_PROPERTY("lastRoundTripDelayNanoseconds",LastRoundTripDelayNanoseconds),
        ESPRESSIO_PROPERTY("modelResidualEnvelopeNanoseconds",ModelResidualEnvelopeNanoseconds),
        ESPRESSIO_PROPERTY("lastMeasuredOffsetNanoseconds",LastMeasuredOffsetNanoseconds),
        ESPRESSIO_PROPERTY("modelPhaseResidualNanoseconds",ModelPhaseResidualNanoseconds),
        ESPRESSIO_PROPERTY("pendingPhaseSlewNanoseconds",PendingPhaseSlewNanoseconds),
        ESPRESSIO_PROPERTY("estimatedFrequencyCorrectionPpm",EstimatedFrequencyCorrectionPpm),
        ESPRESSIO_PROPERTY("residualFrequencyErrorBoundPpm",ResidualFrequencyErrorBoundPpm),
        ESPRESSIO_PROPERTY("uncertaintyKnown",UncertaintyKnown),
        ESPRESSIO_PROPERTY("uncertaintyNanoseconds",UncertaintyNanoseconds),
        ESPRESSIO_PROPERTY("retainedSamples",RetainedSamples),
        ESPRESSIO_PROPERTY("inlierSamples",InlierSamples),
        ESPRESSIO_PROPERTY("observationSpanNanoseconds",ObservationSpanNanoseconds),
        ESPRESSIO_PROPERTY("acceptedSamples",AcceptedSamples),
        ESPRESSIO_PROPERTY("rejectedSamples",RejectedSamples),
        ESPRESSIO_PROPERTY("schedulerDeadlineMisses",SchedulerDeadlineMisses),
        ESPRESSIO_PROPERTY("hasSynchronizationDeadline",HasSynchronizationDeadline),
        ESPRESSIO_PROPERTY("nextRequiredSynchronizationMonotonic",NextRequiredSynchronizationMonotonic))
};
struct ClockProfileDiagnostic final : Serializable::SerializableBase<ClockProfileDiagnostic> {
    std::uint64_t MaximumAcceptedRoundTripDelayNanoseconds{};
    std::uint64_t MaximumCaptureUncertaintyNanoseconds{};
    std::uint64_t MinimumRegressionObservationSpanNanoseconds{};
    std::uint64_t MinimumAcceptedSamples{};
    std::uint64_t UncertaintyWeightFloorNanoseconds{};
    std::uint64_t ResidualEnvelopeNanoseconds{};
    std::uint32_t ResidualEnvelopeUncertaintyMultiplier{};
    double ResidualFrequencyErrorBoundPpm{};
    double MaximumFrequencyCorrectionPpm{};
    std::uint32_t MaximumSlewRatePpm{};
    std::uint64_t QuantizationGuardNanoseconds{};
    std::uint64_t OperationalGuardNanoseconds{};
    std::uint64_t AcquisitionIntervalNanoseconds{};
    std::uint64_t MinimumExchangeIntervalNanoseconds{};
    std::uint64_t MaximumExchangeIntervalNanoseconds{};
    std::uint64_t MaximumFreshSampleAgeNanoseconds{};
    bool AllowUnqualifiedReferenceForAcquisition{};
    ClockProfileDiagnostic() noexcept = default;
    explicit ClockProfileDiagnostic(const Timing::ClockSynchronizationProfile& value) noexcept {
        MaximumAcceptedRoundTripDelayNanoseconds=static_cast<std::uint64_t>(value.MaximumAcceptedRoundTripDelayNanoseconds);
        MaximumCaptureUncertaintyNanoseconds=static_cast<std::uint64_t>(value.MaximumCaptureUncertaintyNanoseconds);
        MinimumRegressionObservationSpanNanoseconds=static_cast<std::uint64_t>(value.MinimumRegressionObservationSpanNanoseconds);
        MinimumAcceptedSamples=static_cast<std::uint64_t>(value.MinimumAcceptedSamples);
        UncertaintyWeightFloorNanoseconds=static_cast<std::uint64_t>(value.UncertaintyWeightFloorNanoseconds);
        ResidualEnvelopeNanoseconds=static_cast<std::uint64_t>(value.ResidualEnvelopeNanoseconds);
        ResidualEnvelopeUncertaintyMultiplier=static_cast<std::uint32_t>(value.ResidualEnvelopeUncertaintyMultiplier);
        ResidualFrequencyErrorBoundPpm=static_cast<double>(value.ResidualFrequencyErrorBoundPpm);
        MaximumFrequencyCorrectionPpm=static_cast<double>(value.MaximumFrequencyCorrectionPpm);
        MaximumSlewRatePpm=static_cast<std::uint32_t>(value.MaximumSlewRatePpm);
        QuantizationGuardNanoseconds=static_cast<std::uint64_t>(value.QuantizationGuardNanoseconds);
        OperationalGuardNanoseconds=static_cast<std::uint64_t>(value.OperationalGuardNanoseconds);
        AcquisitionIntervalNanoseconds=static_cast<std::uint64_t>(value.AcquisitionIntervalNanoseconds);
        MinimumExchangeIntervalNanoseconds=static_cast<std::uint64_t>(value.MinimumExchangeIntervalNanoseconds);
        MaximumExchangeIntervalNanoseconds=static_cast<std::uint64_t>(value.MaximumExchangeIntervalNanoseconds);
        MaximumFreshSampleAgeNanoseconds=static_cast<std::uint64_t>(value.MaximumFreshSampleAgeNanoseconds);
        AllowUnqualifiedReferenceForAcquisition=static_cast<bool>(value.AllowUnqualifiedReferenceForAcquisition);
    }
    ESPRESSIO_SERIALIZABLE_TYPE(ClockProfileDiagnostic)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("maximumAcceptedRoundTripDelayNanoseconds",MaximumAcceptedRoundTripDelayNanoseconds),
        ESPRESSIO_PROPERTY("maximumCaptureUncertaintyNanoseconds",MaximumCaptureUncertaintyNanoseconds),
        ESPRESSIO_PROPERTY("minimumRegressionObservationSpanNanoseconds",MinimumRegressionObservationSpanNanoseconds),
        ESPRESSIO_PROPERTY("minimumAcceptedSamples",MinimumAcceptedSamples),
        ESPRESSIO_PROPERTY("uncertaintyWeightFloorNanoseconds",UncertaintyWeightFloorNanoseconds),
        ESPRESSIO_PROPERTY("residualEnvelopeNanoseconds",ResidualEnvelopeNanoseconds),
        ESPRESSIO_PROPERTY("residualEnvelopeUncertaintyMultiplier",ResidualEnvelopeUncertaintyMultiplier),
        ESPRESSIO_PROPERTY("residualFrequencyErrorBoundPpm",ResidualFrequencyErrorBoundPpm),
        ESPRESSIO_PROPERTY("maximumFrequencyCorrectionPpm",MaximumFrequencyCorrectionPpm),
        ESPRESSIO_PROPERTY("maximumSlewRatePpm",MaximumSlewRatePpm),
        ESPRESSIO_PROPERTY("quantizationGuardNanoseconds",QuantizationGuardNanoseconds),
        ESPRESSIO_PROPERTY("operationalGuardNanoseconds",OperationalGuardNanoseconds),
        ESPRESSIO_PROPERTY("acquisitionIntervalNanoseconds",AcquisitionIntervalNanoseconds),
        ESPRESSIO_PROPERTY("minimumExchangeIntervalNanoseconds",MinimumExchangeIntervalNanoseconds),
        ESPRESSIO_PROPERTY("maximumExchangeIntervalNanoseconds",MaximumExchangeIntervalNanoseconds),
        ESPRESSIO_PROPERTY("maximumFreshSampleAgeNanoseconds",MaximumFreshSampleAgeNanoseconds),
        ESPRESSIO_PROPERTY("allowUnqualifiedReferenceForAcquisition",AllowUnqualifiedReferenceForAcquisition))
};
/// Bounded optional diagnostic occurrence; register only Types the composition needs.
struct SystemClockTimeChangedEvent final : SerializableEvent<SystemClockTimeChangedEvent> {
    static constexpr EventTypeId TypeId{0x4553544900000000ULL+1};
    static constexpr std::string_view CanonicalName="espressio.timing.clock-rebased";
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=1;
    std::uint64_t PreviousTimeNanoseconds{};
    std::uint64_t NewTimeNanoseconds{};
    std::int64_t DifferenceNanoseconds{};
    SystemClockTimeChangedEvent() noexcept = default;
    SystemClockTimeChangedEvent(const std::uint64_t& previousTimeNanoseconds,const std::uint64_t& newTimeNanoseconds,const std::int64_t& differenceNanoseconds) noexcept
        : PreviousTimeNanoseconds(previousTimeNanoseconds),NewTimeNanoseconds(newTimeNanoseconds),DifferenceNanoseconds(differenceNanoseconds) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SystemClockTimeChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("previousTimeNanoseconds",PreviousTimeNanoseconds),
        ESPRESSIO_PROPERTY("newTimeNanoseconds",NewTimeNanoseconds),
        ESPRESSIO_PROPERTY("differenceNanoseconds",DifferenceNanoseconds))
};
/// Bounded optional diagnostic occurrence; register only Types the composition needs.
struct SynchronizationSampleAcceptedEvent final : SerializableEvent<SynchronizationSampleAcceptedEvent> {
    static constexpr EventTypeId TypeId{0x4553544900000000ULL+2};
    static constexpr std::string_view CanonicalName="espressio.timing.sample-accepted";
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=1;
    ClockObservationDiagnostic Result{};
    ClockStatusDiagnostic Status{};
    SynchronizationSampleAcceptedEvent() noexcept = default;
    SynchronizationSampleAcceptedEvent(const ClockObservationDiagnostic& result,const ClockStatusDiagnostic& status) noexcept
        : Result(result),Status(status) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SynchronizationSampleAcceptedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("result",Result),
        ESPRESSIO_PROPERTY("status",Status))
};
/// Bounded optional diagnostic occurrence; register only Types the composition needs.
struct SynchronizationSampleRejectedEvent final : SerializableEvent<SynchronizationSampleRejectedEvent> {
    static constexpr EventTypeId TypeId{0x4553544900000000ULL+3};
    static constexpr std::string_view CanonicalName="espressio.timing.sample-rejected";
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=1;
    ClockObservationDiagnostic Result{};
    ClockStatusDiagnostic Status{};
    SynchronizationSampleRejectedEvent() noexcept = default;
    SynchronizationSampleRejectedEvent(const ClockObservationDiagnostic& result,const ClockStatusDiagnostic& status) noexcept
        : Result(result),Status(status) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SynchronizationSampleRejectedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("result",Result),
        ESPRESSIO_PROPERTY("status",Status))
};
/// Bounded optional diagnostic occurrence; register only Types the composition needs.
struct SynchronizationStateChangedEvent final : SerializableEvent<SynchronizationStateChangedEvent> {
    static constexpr EventTypeId TypeId{0x4553544900000000ULL+4};
    static constexpr std::string_view CanonicalName="espressio.timing.reliability-changed";
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=1;
    std::uint8_t PreviousReliability{};
    std::uint8_t NewReliability{};
    ClockStatusDiagnostic Status{};
    SynchronizationStateChangedEvent() noexcept = default;
    SynchronizationStateChangedEvent(const std::uint8_t& previousReliability,const std::uint8_t& newReliability,const ClockStatusDiagnostic& status) noexcept
        : PreviousReliability(previousReliability),NewReliability(newReliability),Status(status) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SynchronizationStateChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("previousReliability",PreviousReliability),
        ESPRESSIO_PROPERTY("newReliability",NewReliability),
        ESPRESSIO_PROPERTY("status",Status))
};
/// Bounded optional diagnostic occurrence; register only Types the composition needs.
struct SynchronizationResetEvent final : SerializableEvent<SynchronizationResetEvent> {
    static constexpr EventTypeId TypeId{0x4553544900000000ULL+5};
    static constexpr std::string_view CanonicalName="espressio.timing.synchronization-reset";
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=1;
    ClockStatusDiagnostic PreviousStatus{};
    ClockStatusDiagnostic NewStatus{};
    SynchronizationResetEvent() noexcept = default;
    SynchronizationResetEvent(const ClockStatusDiagnostic& previousStatus,const ClockStatusDiagnostic& newStatus) noexcept
        : PreviousStatus(previousStatus),NewStatus(newStatus) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SynchronizationResetEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("previousStatus",PreviousStatus),
        ESPRESSIO_PROPERTY("newStatus",NewStatus))
};
/// Bounded optional diagnostic occurrence; register only Types the composition needs.
struct SynchronizationConfigurationChangedEvent final : SerializableEvent<SynchronizationConfigurationChangedEvent> {
    static constexpr EventTypeId TypeId{0x4553544900000000ULL+6};
    static constexpr std::string_view CanonicalName="espressio.timing.profile-changed";
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=1;
    ClockProfileDiagnostic PreviousProfile{};
    ClockProfileDiagnostic NewProfile{};
    SynchronizationConfigurationChangedEvent() noexcept = default;
    SynchronizationConfigurationChangedEvent(const ClockProfileDiagnostic& previousProfile,const ClockProfileDiagnostic& newProfile) noexcept
        : PreviousProfile(previousProfile),NewProfile(newProfile) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SynchronizationConfigurationChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("previousProfile",PreviousProfile),
        ESPRESSIO_PROPERTY("newProfile",NewProfile))
};
/// Bounded optional diagnostic occurrence; register only Types the composition needs.
struct SystemClockCallbackScheduledEvent final : SerializableEvent<SystemClockCallbackScheduledEvent> {
    static constexpr EventTypeId TypeId{0x4553544900000000ULL+7};
    static constexpr std::string_view CanonicalName="espressio.timing.callback-scheduled";
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=1;
    std::uint64_t ScheduledTimeNanoseconds{};
    SystemClockCallbackScheduledEvent() noexcept = default;
    SystemClockCallbackScheduledEvent(const std::uint64_t& scheduledTimeNanoseconds) noexcept
        : ScheduledTimeNanoseconds(scheduledTimeNanoseconds) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SystemClockCallbackScheduledEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("scheduledTimeNanoseconds",ScheduledTimeNanoseconds))
};
/// Bounded optional diagnostic occurrence; register only Types the composition needs.
struct SystemClockCallbackScheduleFailedEvent final : SerializableEvent<SystemClockCallbackScheduleFailedEvent> {
    static constexpr EventTypeId TypeId{0x4553544900000000ULL+8};
    static constexpr std::string_view CanonicalName="espressio.timing.callback-schedule-failed";
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=1;
    std::uint64_t ScheduledTimeNanoseconds{};
    SystemClockCallbackScheduleFailedEvent() noexcept = default;
    SystemClockCallbackScheduleFailedEvent(const std::uint64_t& scheduledTimeNanoseconds) noexcept
        : ScheduledTimeNanoseconds(scheduledTimeNanoseconds) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SystemClockCallbackScheduleFailedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("scheduledTimeNanoseconds",ScheduledTimeNanoseconds))
};
/// Bounded optional diagnostic occurrence; register only Types the composition needs.
struct SystemClockCallbackExecutedEvent final : SerializableEvent<SystemClockCallbackExecutedEvent> {
    static constexpr EventTypeId TypeId{0x4553544900000000ULL+9};
    static constexpr std::string_view CanonicalName="espressio.timing.callback-executed";
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=1;
    std::uint64_t ScheduledTimeNanoseconds{};
    std::uint64_t ActualTimeNanoseconds{};
    std::int64_t DifferenceNanoseconds{};
    SystemClockCallbackExecutedEvent() noexcept = default;
    SystemClockCallbackExecutedEvent(const std::uint64_t& scheduledTimeNanoseconds,const std::uint64_t& actualTimeNanoseconds,const std::int64_t& differenceNanoseconds) noexcept
        : ScheduledTimeNanoseconds(scheduledTimeNanoseconds),ActualTimeNanoseconds(actualTimeNanoseconds),DifferenceNanoseconds(differenceNanoseconds) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SystemClockCallbackExecutedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("scheduledTimeNanoseconds",ScheduledTimeNanoseconds),
        ESPRESSIO_PROPERTY("actualTimeNanoseconds",ActualTimeNanoseconds),
        ESPRESSIO_PROPERTY("differenceNanoseconds",DifferenceNanoseconds))
};
/// Bounded optional diagnostic occurrence; register only Types the composition needs.
struct SystemClockCallbackExecutionFailedEvent final : SerializableEvent<SystemClockCallbackExecutionFailedEvent> {
    static constexpr EventTypeId TypeId{0x4553544900000000ULL+10};
    static constexpr std::string_view CanonicalName="espressio.timing.callback-execution-failed";
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=1;
    std::uint64_t ScheduledTimeNanoseconds{};
    std::uint64_t ActualTimeNanoseconds{};
    std::int64_t DifferenceNanoseconds{};
    bool HasCause{};
    SystemClockCallbackExecutionFailedEvent() noexcept = default;
    SystemClockCallbackExecutionFailedEvent(const std::uint64_t& scheduledTimeNanoseconds,const std::uint64_t& actualTimeNanoseconds,const std::int64_t& differenceNanoseconds,const bool& hasCause) noexcept
        : ScheduledTimeNanoseconds(scheduledTimeNanoseconds),ActualTimeNanoseconds(actualTimeNanoseconds),DifferenceNanoseconds(differenceNanoseconds),HasCause(hasCause) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SystemClockCallbackExecutionFailedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("scheduledTimeNanoseconds",ScheduledTimeNanoseconds),
        ESPRESSIO_PROPERTY("actualTimeNanoseconds",ActualTimeNanoseconds),
        ESPRESSIO_PROPERTY("differenceNanoseconds",DifferenceNanoseconds),
        ESPRESSIO_PROPERTY("hasCause",HasCause))
};
/// Bounded optional diagnostic occurrence; register only Types the composition needs.
struct SystemClockCallbacksClearedEvent final : SerializableEvent<SystemClockCallbacksClearedEvent> {
    static constexpr EventTypeId TypeId{0x4553544900000000ULL+11};
    static constexpr std::string_view CanonicalName="espressio.timing.callbacks-cleared";
    static constexpr std::size_t MaximumLiveInstances=4,MaximumPendingInstances=1;
    std::uint64_t ClearedCallbackCount{};
    SystemClockCallbacksClearedEvent() noexcept = default;
    SystemClockCallbacksClearedEvent(const std::uint64_t& clearedCallbackCount) noexcept
        : ClearedCallbackCount(clearedCallbackCount) {}
    ESPRESSIO_SERIALIZABLE_TYPE(SystemClockCallbacksClearedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("clearedCallbackCount",ClearedCallbackCount))
};
}
