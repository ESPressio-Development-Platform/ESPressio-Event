#pragma once

#if !__has_include(<ESPressio_Serializable.hpp>) || !__has_include(<ESPressio_DirectBinaryArchive.hpp>)
#error "ESPressio Event Transport requires ESPressio-Serializable >= 0.10.0 in the consuming project."
#endif

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <ESPressio_BinaryArchive.hpp>
#include <ESPressio_DirectBinaryArchive.hpp>
#include <ESPressio_Memory.hpp>
#include <ESPressio_SchemaIntrospection.hpp>
#include <ESPressio_SerializationTraits.hpp>
#include <ESPressio_Synchronization.hpp>
#include <ESPressio_TaskExecutor.hpp>
#include <ESPressio_Thread.hpp>
#include <ESPressio_TreeArchive.hpp>

#include "ESPressio_EventManager.hpp"
#include "ESPressio_EventReceiver.hpp"
#include "ESPressio_EventTransportManagerObservable.hpp"
#include "ESPressio_EventTransportTypes.hpp"
#include "ESPressio_EventTypeKey.hpp"
#include "ESPressio_IEventTransport.hpp"
#include "ESPressio_SerializableEventDescriptor.hpp"

#ifndef ESPRESSIO_EVENT_TRANSPORT_MANAGER_PRIORITY
    #define ESPRESSIO_EVENT_TRANSPORT_MANAGER_PRIORITY 2
#endif

#ifndef ESPRESSIO_EVENT_TRANSPORT_MANAGER_CORE_ID
    #define ESPRESSIO_EVENT_TRANSPORT_MANAGER_CORE_ID 0
#endif

#ifndef ESPRESSIO_EVENT_TRANSPORT_MAX_PENDING_INBOUND
    #define ESPRESSIO_EVENT_TRANSPORT_MAX_PENDING_INBOUND 32
#endif

#ifndef ESPRESSIO_EVENT_TRANSPORT_MAX_PENDING_OUTBOUND_EVENTS
    #define ESPRESSIO_EVENT_TRANSPORT_MAX_PENDING_OUTBOUND_EVENTS 32
#endif

#ifndef ESPRESSIO_EVENT_TRANSPORT_OUTBOUND_TASK_STACK_SIZE
    #define ESPRESSIO_EVENT_TRANSPORT_OUTBOUND_TASK_STACK_SIZE 6144
#endif

#ifndef ESPRESSIO_EVENT_TRANSPORT_OUTBOUND_TASK_QUEUE_DEPTH
    #define ESPRESSIO_EVENT_TRANSPORT_OUTBOUND_TASK_QUEUE_DEPTH 16
#endif

#ifndef ESPRESSIO_EVENT_TRANSPORT_OUTBOUND_TASK_PRIORITY
    #define ESPRESSIO_EVENT_TRANSPORT_OUTBOUND_TASK_PRIORITY 2
#endif

namespace ESPressio::Event {

/// <summary>
/// Serializable Event subscriber that converts Event ownership into owned serialized transport payloads.
/// </summary>
/// <remarks>
/// EventTransportManager behaves like an ordinary Event receiver: EventManager fans one Event reference into this
/// subscriber mailbox for each transport-enabled Serializable Event type. Its coordinator Thread transfers outbound
/// ownership into a bounded TaskExecutor and returns immediately; serialization, transport observation, and fanout execute
/// off the coordinator stack. The executor serializes once into ExternalPreferred storage, releases Event ownership after
/// Event-aware serialized-stage notifications, and then fans shared immutable packet ownership to physical transports.
/// Inbound physical packets enter as owned buffers and are deserialized before being submitted to EventManager.
/// </remarks>
class EventTransportManager final :
    public Threads::Thread,
    public EventReceiver,
    public IEventTransportReceiver {
private:
    static constexpr auto ExternalPreferred =
        System::Memory::MemoryPolicy::ExternalPreferred;

    using RuntimePropertyVector = System::Memory::Vector<
        Serializable::PropertySchemaInfo,
        ExternalPreferred
    >;

    struct RuntimeRegistration {
        EventTypeKey EventType = nullptr;
        uint64_t TypeID = 0;
        std::string_view TypeName{};
        uint32_t SchemaVersion = 1;
        std::function<bool(IEvent*, EventTransportBuffer&)> Serialize;
        std::function<IEvent*(const uint8_t*, std::size_t)> Deserialize;
        RuntimePropertyVector Properties;
        std::function<SerializableEventConstructionResult(
            const Serializable::SerializationNode&,
            const Serializable::DeserializationOptions&)>
            ConstructFromNode;
    };

    using RuntimeRegistrationPtr = std::shared_ptr<const RuntimeRegistration>;

    struct Registration {
        RuntimeRegistrationPtr Runtime;
        EventTransportDirection DefaultDirection = EventTransportDirection::None;
        System::Memory::UnorderedMap<
            IEventTransport*,
            EventTransportDirection,
            ExternalPreferred
        > TransportDirections;

        EventTransportDirection EffectiveDirection(IEventTransport* transport) const {
            const auto found = TransportDirections.find(transport);
            return found == TransportDirections.end()
                ? DefaultDirection
                : found->second;
        }

        bool HasAnyDirection() const {
            if (DefaultDirection != EventTransportDirection::None) return true;
            for (const auto& entry : TransportDirections) {
                if (entry.second != EventTransportDirection::None) return true;
            }
            return false;
        }

        bool HasOutboundDirection() const {
            if (HasDirection(DefaultDirection, EventTransportDirection::Outbound)) {
                return true;
            }
            for (const auto& entry : TransportDirections) {
                if (HasDirection(entry.second, EventTransportDirection::Outbound)) {
                    return true;
                }
            }
            return false;
        }
    };

    struct InboundWork {
        IEventTransport* Transport = nullptr;
        uint64_t TypeID = 0;
        RuntimeRegistrationPtr Runtime;
        EventTransportPacket Packet;
    };

    struct OutboundWork {
        IEvent* Event = nullptr;
        EventDispatchMethod Method = EventDispatchMethod::Queue;
        EventPriority Priority = EventPriority::Normal;
    };

    static_assert(
        std::is_trivially_copyable<OutboundWork>::value,
        "Event transport outbound work must remain trivially copyable"
    );

    using RegistrationMap = System::Memory::UnorderedMap<
        uint64_t,
        Registration,
        ExternalPreferred
    >;
    using RuntimeTypeMap = System::Memory::UnorderedMap<
        EventTypeKey,
        uint64_t,
        ExternalPreferred
    >;
    using TransportVector = System::Memory::Vector<
        IEventTransport*,
        ExternalPreferred
    >;
    using SubscriptionVector = System::Memory::Vector<
        EventTypeKey,
        ExternalPreferred
    >;
    using InboundQueue = System::Memory::Deque<
        InboundWork,
        ExternalPreferred
    >;
    using TypeIdVector = System::Memory::Vector<
        uint64_t,
        ExternalPreferred
    >;

    static Task::TaskConfiguration CreateOutboundTaskConfiguration() {
        Task::TaskConfiguration configuration;
        configuration.Name = "eventTransportTx";
        configuration.StackSize = ESPRESSIO_EVENT_TRANSPORT_OUTBOUND_TASK_STACK_SIZE;
        configuration.Priority = ESPRESSIO_EVENT_TRANSPORT_OUTBOUND_TASK_PRIORITY;
        configuration.Core = -1;
        configuration.QueueDepth = ESPRESSIO_EVENT_TRANSPORT_OUTBOUND_TASK_QUEUE_DEPTH;
        configuration.OverflowPolicy = Task::TaskQueueOverflowPolicy::Reject;
        configuration.MemoryPolicy = Task::TaskMemoryPolicy::PreferExternal;
        return configuration;
    }

    mutable System::Synchronization::Mutex _mutex;
    RegistrationMap _registrations;
    RuntimeTypeMap _runtimeTypes;
    TransportVector _transports;
    SubscriptionVector _subscriptions;
    InboundQueue _inbound;
    std::unique_ptr<System::Synchronization::ISignal> _workSignal =
        System::Synchronization::CreateBinarySignal();
    std::shared_ptr<EventTransportManagerObservable> _observable =
        CreateEventTransportManagerObservable();
    std::atomic<uint64_t> _nextMessageID{1};
    std::atomic<uint64_t> _rejectedInboundCount{0};
    std::atomic<uint64_t> _processedInboundCount{0};
    std::atomic<uint64_t> _processedOutboundCount{0};
    std::atomic<uint64_t> _rejectedOutboundWorkCount{0};
    std::atomic<std::size_t> _peakInboundCount{0};
    Task::TaskExecutor<OutboundWork> _outboundExecutor;
    // Single-worker scratch storage retains target-vector capacity in PSRAM
    // rather than allocating a fresh vector for every outbound Event.
    TransportVector _outboundTargets;
    std::atomic<bool> _outboundExecutorInitialized{false};
    std::atomic<bool> _outboundExecutorReady{false};
    std::atomic<bool> _initialized{false};

    EventTransportManager()
        : Threads::Thread(Threads::ThreadReleasePolicy::ExplicitRelease),
          _outboundExecutor(CreateOutboundTaskConfiguration()) {
        SetPriority(ESPRESSIO_EVENT_TRANSPORT_MANAGER_PRIORITY);
        SetCoreID(ESPRESSIO_EVENT_TRANSPORT_MANAGER_CORE_ID);
        SetStartOnInitialize(false);
        SetMaximumPendingEventCount(
            ESPRESSIO_EVENT_TRANSPORT_MAX_PENDING_OUTBOUND_EVENTS
        );
        SetEventQueueOverflowPolicy(EventQueueOverflowPolicy::RejectIncoming);
    }

    static bool ParseEnvelope(
        const EventTransportPacket& packet,
        EventTransportEnvelope& envelope,
        const uint8_t*& payload
    ) {
        if (!packet || packet.Size() < sizeof(EventTransportEnvelope)) return false;
        std::memcpy(&envelope, packet.Data(), sizeof(envelope));
        if (
            envelope.Magic != EventTransportEnvelope::MagicValue ||
            envelope.Version != EventTransportEnvelope::CurrentVersion ||
            envelope.PayloadLength != packet.Size() - sizeof(EventTransportEnvelope)
        ) {
            return false;
        }
        payload = packet.Data() + sizeof(EventTransportEnvelope);
        return true;
    }

    bool IsTransportRegisteredLocked(IEventTransport* transport) const {
        return std::find(_transports.begin(), _transports.end(), transport) !=
            _transports.end();
    }

    bool IsSubscribedLocked(EventTypeKey type) const {
        return std::find(_subscriptions.begin(), _subscriptions.end(), type) !=
            _subscriptions.end();
    }

    void Wake() noexcept {
        if (_workSignal != nullptr) (void)_workSignal->Give();
    }

    void EventAdded() override {
        Wake();
    }

    bool HasPendingWork() const {
        if (GetPendingEventCount() != 0) return true;
        std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
        return !_inbound.empty();
    }

    void UpdatePeakInbound(std::size_t value) noexcept {
        std::size_t current = _peakInboundCount.load(std::memory_order_relaxed);
        while (
            value > current &&
            !_peakInboundCount.compare_exchange_weak(
                current,
                value,
                std::memory_order_relaxed,
                std::memory_order_relaxed
            )
        ) {}
    }

    void NotifyTransaction(const EventTransportTransaction& transaction) {
        if (!_observable) return;
        _observable->Notify(
            [&](IEventTransportManagerObserver* observer) {
                observer->OnEventTransportTransaction(transaction);
            }
        );
    }

    void SetOutboundSubscription(EventTypeKey type, bool required) {
        if (type == nullptr) return;

        bool registerReceiver = false;
        bool unregisterReceiver = false;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            const bool subscribed = IsSubscribedLocked(type);
            if (required && !subscribed) {
                _subscriptions.push_back(type);
                registerReceiver = _initialized.load(std::memory_order_acquire);
            } else if (!required && subscribed) {
                _subscriptions.erase(
                    std::remove(_subscriptions.begin(), _subscriptions.end(), type),
                    _subscriptions.end()
                );
                unregisterReceiver = _initialized.load(std::memory_order_acquire);
            }
        }

        if (registerReceiver) {
            EventManager::GetInstance()->RegisterReceiver(type, this);
        } else if (unregisterReceiver) {
            EventManager::GetInstance()->UnregisterReceiver(type, this);
        }
    }

    void RefreshOutboundSubscription(uint64_t typeID) {
        EventTypeKey type = nullptr;
        bool required = false;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            const auto found = _registrations.find(typeID);
            if (found != _registrations.end() && found->second.Runtime) {
                type = found->second.Runtime->EventType;
                required = found->second.HasOutboundDirection();
            }
        }
        if (type != nullptr) SetOutboundSubscription(type, required);
    }

    void RemoveRegistrationIfUnusedLocked(uint64_t typeID) {
        auto found = _registrations.find(typeID);
        if (found == _registrations.end() || found->second.HasAnyDirection()) return;

        const RuntimeRegistrationPtr runtime = found->second.Runtime;
        if (runtime && (runtime->ConstructFromNode || !runtime->Properties.empty())) {
            return;
        }

        if (runtime && runtime->EventType != nullptr) {
            _runtimeTypes.erase(runtime->EventType);
        }
        _registrations.erase(found);
    }

    void ReleaseOutboundWork(const OutboundWork& work) noexcept {
        if (work.Event != nullptr) work.Event->__unref();
    }

    void ProcessOutboundWork(const OutboundWork& work) {
        class EventReferenceGuard final {
        private:
            IEvent* _event = nullptr;
        public:
            explicit EventReferenceGuard(IEvent* event) noexcept : _event(event) {}
            ~EventReferenceGuard() {
                if (_event != nullptr) _event->__unref();
            }
            void ReleaseNow() noexcept {
                IEvent* event = _event;
                _event = nullptr;
                if (event != nullptr) event->__unref();
            }
        } eventReference(work.Event);

        IEvent* event = work.Event;
        if (event == nullptr) return;
        const EventDispatchContext context = event->__getDispatchContext();
        if (context.Origin != EventOrigin::Local) return;

        const EventTypeKey eventType = event->__getTypeKey();
        if (eventType == nullptr) return;

        uint64_t typeID = 0;
        uint64_t messageID = 0;
        RuntimeRegistrationPtr runtime;
        TransportVector& targets = _outboundTargets;
        targets.clear();
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            const auto runtimeType = _runtimeTypes.find(eventType);
            if (runtimeType == _runtimeTypes.end()) return;
            const auto registration = _registrations.find(runtimeType->second);
            if (registration == _registrations.end() || !registration->second.Runtime) {
                return;
            }

            targets.reserve(_transports.size());
            for (IEventTransport* transport : _transports) {
                if (
                    transport != nullptr &&
                    HasDirection(
                        registration->second.EffectiveDirection(transport),
                        EventTransportDirection::Outbound
                    )
                ) {
                    targets.push_back(transport);
                }
            }
            if (targets.empty()) return;

            typeID = runtimeType->second;
            runtime = registration->second.Runtime;
            messageID = _nextMessageID.fetch_add(1, std::memory_order_relaxed);
        }

        if (!runtime || !runtime->Serialize) return;

        if (_observable) {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnOutboundEventAccepted(typeID, messageID);
            });
        }

        EventTransportBuffer bytes(sizeof(EventTransportEnvelope));
        if (!runtime->Serialize(event, bytes)) {
            for (IEventTransport* transport : targets) {
                NotifyTransaction({
                    EventTransportTransactionStage::Failed,
                    EventTransportDirection::Outbound,
                    typeID,
                    runtime->TypeName,
                    runtime->SchemaVersion,
                    messageID,
                    transport,
                    event,
                    nullptr,
                    0,
                    work.Method,
                    work.Priority,
                    EventOrigin::Local,
                    0,
                    false
                });
            }
            return;
        }

        const std::size_t payloadSize = bytes.size() - sizeof(EventTransportEnvelope);
        EventTransportEnvelope envelope;
        envelope.EventTypeID = runtime->TypeID;
        envelope.SchemaVersion = runtime->SchemaVersion;
        envelope.MessageID = messageID;
        envelope.DispatchMethod = static_cast<uint8_t>(work.Method);
        envelope.Priority = static_cast<uint8_t>(work.Priority);
        envelope.HopCount = 0;
        envelope.PayloadLength = static_cast<uint32_t>(payloadSize);
        std::memcpy(bytes.data(), &envelope, sizeof(envelope));

        EventTransportPacket packet(std::move(bytes), messageID);
        const uint8_t* payload = packet.Data() + sizeof(EventTransportEnvelope);

        // Event-aware notifications run before releasing asynchronous Event ownership.
        // Once serialization is complete, downstream transport fanout needs only the
        // immutable ExternalPreferred packet and no longer keeps the Event alive.
        for (IEventTransport* transport : targets) {
            if (_observable) {
                _observable->Notify([&](IEventTransportManagerObserver* observer) {
                    observer->OnOutboundEventAcceptedForTransport(
                        typeID,
                        messageID,
                        transport
                    );
                });
            }
            NotifyTransaction({
                EventTransportTransactionStage::OutboundSerialized,
                EventTransportDirection::Outbound,
                typeID,
                runtime->TypeName,
                runtime->SchemaVersion,
                messageID,
                transport,
                event,
                payload,
                payloadSize,
                work.Method,
                work.Priority,
                EventOrigin::Local,
                0,
                false
            });
        }

        eventReference.ReleaseNow();
        event = nullptr;

        for (IEventTransport* transport : targets) {
            const bool accepted = transport->Send(packet);
            if (_observable) {
                _observable->Notify([&](IEventTransportManagerObserver* observer) {
                    observer->OnOutboundEventHandedToTransport(
                        typeID,
                        messageID,
                        transport,
                        accepted
                    );
                });
            }
            NotifyTransaction({
                EventTransportTransactionStage::OutboundHandedToTransport,
                EventTransportDirection::Outbound,
                typeID,
                runtime->TypeName,
                runtime->SchemaVersion,
                messageID,
                transport,
                nullptr,
                payload,
                payloadSize,
                work.Method,
                work.Priority,
                EventOrigin::Local,
                0,
                accepted
            });
        }

        targets.clear();
        _processedOutboundCount.fetch_add(1, std::memory_order_relaxed);
    }

    void SubmitOutboundEvent(
        IEvent* event,
        EventDispatchMethod method,
        EventPriority priority
    ) noexcept {
        if (
            event == nullptr ||
            !_outboundExecutorReady.load(std::memory_order_acquire)
        ) return;

        OutboundWork work;
        work.Event = event;
        work.Method = method;
        work.Priority = priority;

        event->__ref();
        const auto status = _outboundExecutor.Submit(work);
        if (status != Task::TaskExecutionStatus::Success) {
            event->__unref();
            _rejectedOutboundWorkCount.fetch_add(1, std::memory_order_relaxed);
        }
    }

    bool PopInbound(InboundWork& work) {
        std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
        if (_inbound.empty()) return false;
        work = std::move(_inbound.front());
        _inbound.pop_front();
        return true;
    }

    void ProcessInbound(InboundWork work) {
        EventTransportEnvelope envelope;
        const uint8_t* payload = nullptr;
        if (!ParseEnvelope(work.Packet, envelope, payload)) return;

        if (
            envelope.DispatchMethod > static_cast<uint8_t>(EventDispatchMethod::Queue) ||
            envelope.Priority > static_cast<uint8_t>(EventPriority::High)
        ) {
            return;
        }

        const RuntimeRegistrationPtr& runtime = work.Runtime;
        if (!runtime || !runtime->Deserialize) return;

        IEvent* event = runtime->Deserialize(payload, envelope.PayloadLength);
        if (event == nullptr) {
            NotifyTransaction({
                EventTransportTransactionStage::Failed,
                EventTransportDirection::Inbound,
                envelope.EventTypeID,
                runtime->TypeName,
                envelope.SchemaVersion,
                envelope.MessageID,
                work.Transport,
                nullptr,
                payload,
                envelope.PayloadLength,
                static_cast<EventDispatchMethod>(envelope.DispatchMethod),
                static_cast<EventPriority>(envelope.Priority),
                EventOrigin::Remote,
                envelope.HopCount,
                false
            });
            return;
        }

        if (_observable) {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnInboundEventDeserialized(
                    envelope.EventTypeID,
                    envelope.MessageID
                );
            });
        }

        EventDispatchContext context;
        context.Origin = EventOrigin::Remote;
        context.TransportMessageID = envelope.MessageID;
        context.HopCount = envelope.HopCount;
        event->__setDispatchContext(context);

        const auto method = static_cast<EventDispatchMethod>(envelope.DispatchMethod);
        const auto priority = static_cast<EventPriority>(envelope.Priority);

        const bool accepted = method == EventDispatchMethod::Stack
            ? EventManager::GetInstance()->TryStackEvent(event, priority)
            : EventManager::GetInstance()->TryQueueEvent(event, priority);

        if (!accepted) {
            NotifyTransaction({
                EventTransportTransactionStage::Failed,
                EventTransportDirection::Inbound,
                envelope.EventTypeID,
                runtime->TypeName,
                envelope.SchemaVersion,
                envelope.MessageID,
                work.Transport,
                nullptr,
                payload,
                envelope.PayloadLength,
                method,
                priority,
                EventOrigin::Remote,
                envelope.HopCount,
                false
            });
            return;
        }

        if (_observable) {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnInboundEventDispatched(
                    envelope.EventTypeID,
                    envelope.MessageID
                );
            });
        }

        NotifyTransaction({
            EventTransportTransactionStage::InboundDispatched,
            EventTransportDirection::Inbound,
            envelope.EventTypeID,
            runtime->TypeName,
            envelope.SchemaVersion,
            envelope.MessageID,
            work.Transport,
            nullptr,
            payload,
            envelope.PayloadLength,
            method,
            priority,
            EventOrigin::Remote,
            envelope.HopCount,
            true
        });
        _processedInboundCount.fetch_add(1, std::memory_order_relaxed);
    }

    void OnLoop() override {
        if (_workSignal != nullptr) {
            if (!HasPendingWork()) {
                (void)_workSignal->Wait(System::Synchronization::WaitForever);
            } else {
                (void)_workSignal->Wait(0);
            }
        }

        // Always service one inbound packet, then one bounded snapshot of outbound
        // Event mailbox work. Neither direction can indefinitely exclude the other.
        for (;;) {
            bool didWork = false;

            InboundWork inbound;
            if (PopInbound(inbound)) {
                ProcessInbound(std::move(inbound));
                didWork = true;
            }

            if (GetPendingEventCount() != 0) {
                WithEvents(
                    [&](IEvent* event, EventDispatchMethod method, EventPriority priority) {
                        SubmitOutboundEvent(event, method, priority);
                    }
                );
                didWork = true;
            }

            if (!didWork) break;
        }
    }

    template<typename TEvent>
    static Registration CreateRegistration(EventTransportDirection defaultDirection) {
        auto runtime = System::Memory::MakeShared<
            RuntimeRegistration,
            ExternalPreferred
        >();
        runtime->EventType = EventTypeKeyOf<TEvent>();
        runtime->TypeID = EventTransportTypeID<TEvent>();
        runtime->TypeName = EventTransportTypeTraits<TEvent>::Name;
        runtime->SchemaVersion = TEvent::GetSchemaVersion();
        const auto properties = Serializable::SchemaInspector<TEvent>::Properties();
        runtime->Properties.assign(properties.begin(), properties.end());

        runtime->Serialize = [](IEvent* event, EventTransportBuffer& bytes) {
            if (
                event == nullptr ||
                event->__getTypeKey() != EventTypeKeyOf<TEvent>()
            ) return false;

            auto* typed = static_cast<TEvent*>(event);
            const std::size_t prefix = bytes.size();
            if (Serializable::AppendDirectBinary(*typed, bytes)) {
                return bytes.size() > prefix;
            }

            bytes.resize(prefix);
            Serializable::BinaryArchive archive;
            typed->Serialize(archive);
            const auto& payload = archive.GetData();
            if (payload.empty()) return false;
            bytes.insert(bytes.end(), payload.begin(), payload.end());
            return true;
        };

        if constexpr (std::is_default_constructible_v<TEvent>) {
            runtime->Deserialize = [](const uint8_t* data, std::size_t size) -> IEvent* {
                auto event = std::make_unique<TEvent>();
                const auto direct = Serializable::DeserializeDirectBinary(data, size, *event);
                if (direct.Success()) return event.release();

                Serializable::BinaryArchive archive;
                if (!archive.Load(data, size)) return nullptr;
                event = std::make_unique<TEvent>();
                if (!event->Deserialize(archive)) return nullptr;
                return event.release();
            };

            runtime->ConstructFromNode = [](
                const Serializable::SerializationNode& node,
                const Serializable::DeserializationOptions& options
            ) -> SerializableEventConstructionResult {
                SerializableEventConstructionResult result;
                result.TypeRegistered = true;
                result.Constructible = true;
                auto event = std::make_unique<TEvent>();
                Serializable::TreeArchive archive;
                archive.GetNode() = node;
                result.Deserialization = event->DeserializeDetailed(archive, options);
                if (result.Deserialization.Success()) {
                    result.Event = std::move(event);
                }
                return result;
            };
        }

        Registration registration;
        registration.Runtime = std::move(runtime);
        registration.DefaultDirection = defaultDirection;
        return registration;
    }

    template<typename TEvent>
    static void ValidateTransportEventType() {
        static_assert(
            std::is_base_of_v<IEvent, TEvent>,
            "Transported types must derive from ESPressio::Event::IEvent."
        );
        static_assert(
            Serializable::IsSerializable<TEvent>,
            "Transported Events must implement ESPressio Serializable."
        );
        static_assert(
            EventTransportTypeTraits<TEvent>::Name.size() != 0,
            "Transported Events require ESPRESSIO_EVENT_TRANSPORT_TYPE(Type, StableName)."
        );
    }

    template<typename TEvent>
    EventTransportRegistrationResult RegisterGlobalEventImpl(
        EventTransportDirection direction,
        bool requireInboundFactory
    ) {
        ValidateTransportEventType<TEvent>();
        if (requireInboundFactory && !std::is_default_constructible_v<TEvent>) {
            return EventTransportRegistrationResult::TypeConflict;
        }

        constexpr uint64_t typeID = EventTransportTypeID<TEvent>();
        Registration proposed = CreateRegistration<TEvent>(direction);
        const EventTypeKey eventType = proposed.Runtime->EventType;
        EventTransportRegistrationResult result;
        EventTransportDirection before = EventTransportDirection::None;
        EventTransportDirection after = EventTransportDirection::None;

        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            auto found = _registrations.find(typeID);
            if (found == _registrations.end()) {
                _registrations.emplace(typeID, std::move(proposed));
                _runtimeTypes[eventType] = typeID;
                result = EventTransportRegistrationResult::Registered;
                after = direction;
            } else if (
                !found->second.Runtime ||
                found->second.Runtime->EventType != eventType
            ) {
                return EventTransportRegistrationResult::TypeConflict;
            } else {
                before = found->second.DefaultDirection;
                after = before | direction;
                if (after == before) {
                    return EventTransportRegistrationResult::AlreadyRegistered;
                }
                found->second.DefaultDirection = after;
                _runtimeTypes[eventType] = typeID;
                result = EventTransportRegistrationResult::Updated;
            }
        }

        RefreshOutboundSubscription(typeID);

        if (_observable) {
            if (result == EventTransportRegistrationResult::Registered) {
                _observable->Notify([&](IEventTransportManagerObserver* observer) {
                    observer->OnEventTransportTypeRegistered(typeID, after);
                });
            } else {
                _observable->Notify([&](IEventTransportManagerObserver* observer) {
                    observer->OnEventTransportTypeRegistrationChanged(typeID, before, after);
                });
            }
        }
        return result;
    }

    template<typename TEvent>
    EventTransportRegistrationResult RegisterTransportEventImpl(
        IEventTransport* transport,
        EventTransportDirection direction,
        bool requireInboundFactory
    ) {
        ValidateTransportEventType<TEvent>();
        if (transport == nullptr) return EventTransportRegistrationResult::InvalidTransport;
        if (requireInboundFactory && !std::is_default_constructible_v<TEvent>) {
            return EventTransportRegistrationResult::TypeConflict;
        }

        constexpr uint64_t typeID = EventTransportTypeID<TEvent>();
        Registration proposed = CreateRegistration<TEvent>(EventTransportDirection::None);
        const EventTypeKey eventType = proposed.Runtime->EventType;
        EventTransportDirection before = EventTransportDirection::None;
        EventTransportDirection after = EventTransportDirection::None;
        bool newRoute = false;

        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            auto found = _registrations.find(typeID);
            if (found == _registrations.end()) {
                proposed.TransportDirections[transport] = direction;
                _registrations.emplace(typeID, std::move(proposed));
                _runtimeTypes[eventType] = typeID;
                after = direction;
                newRoute = true;
            } else if (
                !found->second.Runtime ||
                found->second.Runtime->EventType != eventType
            ) {
                return EventTransportRegistrationResult::TypeConflict;
            } else {
                auto route = found->second.TransportDirections.find(transport);
                if (route == found->second.TransportDirections.end()) {
                    before = found->second.EffectiveDirection(transport);
                    after = before | direction;
                    found->second.TransportDirections[transport] = after;
                    newRoute = true;
                } else {
                    before = route->second;
                    after = before | direction;
                    if (after == before) {
                        return EventTransportRegistrationResult::AlreadyRegistered;
                    }
                    route->second = after;
                }
                _runtimeTypes[eventType] = typeID;
            }
        }

        RefreshOutboundSubscription(typeID);

        if (_observable) {
            if (newRoute) {
                _observable->Notify([&](IEventTransportManagerObserver* observer) {
                    observer->OnEventTransportTypeRouteRegistered(typeID, transport, after);
                });
                return EventTransportRegistrationResult::Registered;
            }
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnEventTransportTypeRouteChanged(typeID, transport, before, after);
            });
        }
        return newRoute
            ? EventTransportRegistrationResult::Registered
            : EventTransportRegistrationResult::Updated;
    }

    template<typename TOperation>
    static void AccumulateBulkResult(
        EventTransportBulkOperationResult& result,
        TOperation operation
    ) {
        const auto current = operation();
        if constexpr (std::is_same_v<
            std::decay_t<decltype(current)>,
            EventTransportRegistrationResult
        >) {
            if (current == EventTransportRegistrationResult::AlreadyRegistered) {
                ++result.Unchanged;
            } else if (
                current == EventTransportRegistrationResult::TypeConflict ||
                current == EventTransportRegistrationResult::InvalidTransport
            ) {
                ++result.Failed;
            } else {
                ++result.Changed;
            }
        } else {
            if (current == EventTransportUnregistrationResult::NotRegistered) {
                ++result.Unchanged;
            } else if (current == EventTransportUnregistrationResult::InvalidTransport) {
                ++result.Failed;
            } else {
                ++result.Changed;
            }
        }
    }

    void DiscardInboundLocked(
        uint64_t typeID,
        IEventTransport* transport,
        EventTransportDirection removedDirection,
        const EventTransportUnregistrationOptions& options,
        bool inboundStillAllowed
    ) {
        if (
            !HasDirection(removedDirection, EventTransportDirection::Inbound) ||
            inboundStillAllowed ||
            options.PendingInbound != EventTransportPendingAction::Discard
        ) return;

        _inbound.erase(
            std::remove_if(
                _inbound.begin(),
                _inbound.end(),
                [&](const InboundWork& work) {
                    return work.TypeID == typeID && work.Transport == transport;
                }
            ),
            _inbound.end()
        );
    }

public:
    EventTransportManager(const EventTransportManager&) = delete;
    EventTransportManager& operator=(const EventTransportManager&) = delete;

    ~EventTransportManager() override {
        Shutdown();
        Threads::Thread::Shutdown();
    }

    static EventTransportManager& GetInstance() {
        static EventTransportManager instance;
        return instance;
    }

    Threads::ThreadInitializationStatus Initialize() override {
        if (_initialized.load(std::memory_order_acquire)) {
            return Threads::ThreadInitializationStatus::AlreadyInitialized;
        }

        if (!_outboundExecutorInitialized.load(std::memory_order_acquire)) {
            const auto outboundInitialization = _outboundExecutor.Initialize(
                [this](const OutboundWork& work) { ProcessOutboundWork(work); },
                [this](const OutboundWork& work) { ReleaseOutboundWork(work); }
            );
            if (
                outboundInitialization != Task::TaskExecutionStatus::Success &&
                outboundInitialization != Task::TaskExecutionStatus::AlreadyInitialized
            ) {
                return Threads::ThreadInitializationStatus::TaskCreationFailed;
            }
            _outboundExecutorInitialized.store(true, std::memory_order_release);
        }

        const auto initializationStatus = Threads::Thread::Initialize();
        if (
            initializationStatus != Threads::ThreadInitializationStatus::Success &&
            initializationStatus != Threads::ThreadInitializationStatus::AlreadyInitialized
        ) {
            _outboundExecutor.Stop();
            _outboundExecutorInitialized.store(false, std::memory_order_release);
            _outboundExecutorReady.store(false, std::memory_order_release);
            return initializationStatus;
        }

        SubscriptionVector subscriptions;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            _initialized.store(true, std::memory_order_release);
            subscriptions = _subscriptions;
        }
        for (EventTypeKey type : subscriptions) {
            EventManager::GetInstance()->RegisterReceiver(type, this);
        }
        return Threads::ThreadInitializationStatus::Success;
    }

    Threads::ThreadInitializationStatus Start() override {
        if (!_initialized.load(std::memory_order_acquire)) {
            const auto initialization = Initialize();
            if (
                initialization != Threads::ThreadInitializationStatus::Success &&
                initialization != Threads::ThreadInitializationStatus::AlreadyInitialized
            ) return initialization;
        }

        if (!_outboundExecutorReady.load(std::memory_order_acquire)) {
            const auto outboundStart = _outboundExecutor.Start();
            if (
                outboundStart != Task::TaskExecutionStatus::Success &&
                outboundStart != Task::TaskExecutionStatus::AlreadyStarted
            ) {
                return Threads::ThreadInitializationStatus::TaskCreationFailed;
            }
            _outboundExecutorReady.store(true, std::memory_order_release);
        }

        const auto threadStart = Threads::Thread::Start();
        if (
            threadStart != Threads::ThreadInitializationStatus::Success &&
            threadStart != Threads::ThreadInitializationStatus::AlreadyInitialized
        ) {
            _outboundExecutorReady.store(false, std::memory_order_release);
            _outboundExecutor.Stop();
            _outboundExecutorInitialized.store(false, std::memory_order_release);
        }
        return threadStart;
    }

    bool IsInitialized() const noexcept {
        return _initialized.load(std::memory_order_acquire);
    }

    void Shutdown() {
        SubscriptionVector subscriptions;
        TransportVector transports;
        const bool wasInitialized = _initialized.exchange(false, std::memory_order_acq_rel);
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            if (!wasInitialized && _transports.empty()) return;
            subscriptions = _subscriptions;
            transports = _transports;
            _transports.clear();
            _inbound.clear();
        }

        _outboundExecutorReady.store(false, std::memory_order_release);
        for (EventTypeKey type : subscriptions) {
            EventManager::GetInstance()->UnregisterReceiver(type, this);
        }

        ClearPendingEvents();
        _outboundExecutor.Stop();
        _outboundExecutorInitialized.store(false, std::memory_order_release);

        for (IEventTransport* transport : transports) {
            if (transport != nullptr) transport->SetReceiver(nullptr);
        }
    }

    bool RegisterTransport(IEventTransport* transport) {
        if (transport == nullptr) return false;
        bool added = false;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            if (!IsTransportRegisteredLocked(transport)) {
                _transports.push_back(transport);
                added = true;
            }
        }
        if (added) {
            transport->SetReceiver(this);
            if (_observable) {
                _observable->Notify([&](IEventTransportManagerObserver* observer) {
                    observer->OnEventTransportRegistered(transport);
                });
            }
        }
        return true;
    }

    void UnregisterTransport(IEventTransport* transport) {
        if (transport == nullptr) return;
        bool removed = false;
        TypeIdVector refreshTypes;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            const auto oldSize = _transports.size();
            _transports.erase(
                std::remove(_transports.begin(), _transports.end(), transport),
                _transports.end()
            );
            removed = _transports.size() != oldSize;
            if (!removed) return;

            _inbound.erase(
                std::remove_if(
                    _inbound.begin(),
                    _inbound.end(),
                    [&](const InboundWork& work) { return work.Transport == transport; }
                ),
                _inbound.end()
            );

            for (auto& entry : _registrations) {
                entry.second.TransportDirections.erase(transport);
                refreshTypes.push_back(entry.first);
            }
        }

        transport->SetReceiver(nullptr);
        for (uint64_t typeID : refreshTypes) RefreshOutboundSubscription(typeID);
        if (_observable) {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnEventTransportUnregistered(transport);
            });
        }
    }

    std::vector<SerializableEventDescriptor> GetRegisteredSerializableEvents() const {
        std::vector<SerializableEventDescriptor> descriptors;
        std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
        descriptors.reserve(_registrations.size());
        for (const auto& entry : _registrations) {
            const Registration& registration = entry.second;
            if (!registration.Runtime) continue;
            SerializableEventDescriptor descriptor;
            descriptor.TypeID = registration.Runtime->TypeID;
            descriptor.TypeName = std::string(registration.Runtime->TypeName);
            descriptor.SchemaVersion = registration.Runtime->SchemaVersion;
            descriptor.DefaultDirection = registration.DefaultDirection;
            descriptor.Properties.assign(
                registration.Runtime->Properties.begin(),
                registration.Runtime->Properties.end()
            );
            descriptor.CanConstruct = static_cast<bool>(registration.Runtime->ConstructFromNode);
            descriptors.push_back(std::move(descriptor));
        }
        std::sort(descriptors.begin(), descriptors.end(), [](const auto& a, const auto& b) {
            return a.TypeName < b.TypeName;
        });
        return descriptors;
    }

    bool FindRegisteredSerializableEvent(
        uint64_t typeID,
        SerializableEventDescriptor& descriptor
    ) const {
        std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
        const auto found = _registrations.find(typeID);
        if (found == _registrations.end() || !found->second.Runtime) return false;
        const auto& runtime = found->second.Runtime;
        descriptor.TypeID = runtime->TypeID;
        descriptor.TypeName = std::string(runtime->TypeName);
        descriptor.SchemaVersion = runtime->SchemaVersion;
        descriptor.DefaultDirection = found->second.DefaultDirection;
        descriptor.Properties.assign(runtime->Properties.begin(), runtime->Properties.end());
        descriptor.CanConstruct = static_cast<bool>(runtime->ConstructFromNode);
        return true;
    }

    bool FindRegisteredSerializableEvent(
        std::string_view typeName,
        SerializableEventDescriptor& descriptor
    ) const {
        std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
        for (const auto& entry : _registrations) {
            if (!entry.second.Runtime || entry.second.Runtime->TypeName != typeName) continue;
            const auto& runtime = entry.second.Runtime;
            descriptor.TypeID = runtime->TypeID;
            descriptor.TypeName = std::string(runtime->TypeName);
            descriptor.SchemaVersion = runtime->SchemaVersion;
            descriptor.DefaultDirection = entry.second.DefaultDirection;
            descriptor.Properties.assign(runtime->Properties.begin(), runtime->Properties.end());
            descriptor.CanConstruct = static_cast<bool>(runtime->ConstructFromNode);
            return true;
        }
        return false;
    }

    SerializableEventConstructionResult CreateSerializableEvent(
        uint64_t typeID,
        const Serializable::SerializationNode& node,
        const Serializable::DeserializationOptions& options = {}
    ) const {
        RuntimeRegistrationPtr runtime;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            const auto found = _registrations.find(typeID);
            if (found == _registrations.end()) return {};
            runtime = found->second.Runtime;
        }
        if (!runtime) return {};
        if (!runtime->ConstructFromNode) {
            SerializableEventConstructionResult result;
            result.TypeRegistered = true;
            return result;
        }
        return runtime->ConstructFromNode(node, options);
    }

    SerializableEventConstructionResult CreateSerializableEvent(
        std::string_view typeName,
        const Serializable::SerializationNode& node,
        const Serializable::DeserializationOptions& options = {}
    ) const {
        RuntimeRegistrationPtr runtime;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            for (const auto& entry : _registrations) {
                if (entry.second.Runtime && entry.second.Runtime->TypeName == typeName) {
                    runtime = entry.second.Runtime;
                    break;
                }
            }
        }
        if (!runtime) return {};
        if (!runtime->ConstructFromNode) {
            SerializableEventConstructionResult result;
            result.TypeRegistered = true;
            return result;
        }
        return runtime->ConstructFromNode(node, options);
    }

    static RuntimeEventDispatchResult DispatchSerializableEvent(
        std::unique_ptr<IEvent> event,
        EventDispatchMethod method = EventDispatchMethod::Queue,
        EventPriority priority = EventPriority::Normal
    ) {
        if (!event) return RuntimeEventDispatchResult::NullEvent;
        IEvent* released = event.release();
        switch (method) {
            case EventDispatchMethod::Queue:
                released->Queue(priority);
                return RuntimeEventDispatchResult::Dispatched;
            case EventDispatchMethod::Stack:
                released->Stack(priority);
                return RuntimeEventDispatchResult::Dispatched;
            default:
                delete released;
                return RuntimeEventDispatchResult::UnsupportedMethod;
        }
    }

    template<typename TEvent>
    EventTransportRegistrationResult RegisterEvent(EventTransportDirection direction) {
        if (HasDirection(direction, EventTransportDirection::Inbound)) {
            static_assert(
                std::is_default_constructible_v<TEvent>,
                "Inbound transported Events must be default constructible."
            );
        }
        return RegisterGlobalEventImpl<TEvent>(
            direction,
            HasDirection(direction, EventTransportDirection::Inbound)
        );
    }

    template<typename TEvent>
    EventTransportRegistrationResult RegisterEvent(
        IEventTransport* transport,
        EventTransportDirection direction
    ) {
        if (HasDirection(direction, EventTransportDirection::Inbound)) {
            static_assert(
                std::is_default_constructible_v<TEvent>,
                "Inbound transported Events must be default constructible."
            );
        }
        return RegisterTransportEventImpl<TEvent>(
            transport,
            direction,
            HasDirection(direction, EventTransportDirection::Inbound)
        );
    }

    template<typename... TEvents>
    EventTransportBulkOperationResult RegisterEvents(EventTransportDirection direction) {
        EventTransportBulkOperationResult result;
        result.Requested = sizeof...(TEvents);
        (AccumulateBulkResult(result, [&]() { return RegisterEvent<TEvents>(direction); }), ...);
        return result;
    }

    template<typename... TEvents>
    EventTransportBulkOperationResult RegisterEvents(
        IEventTransport* transport,
        EventTransportDirection direction
    ) {
        EventTransportBulkOperationResult result;
        result.Requested = sizeof...(TEvents);
        (AccumulateBulkResult(result, [&]() { return RegisterEvent<TEvents>(transport, direction); }), ...);
        return result;
    }

    template<typename TEvent> EventTransportRegistrationResult RegisterInboundEvent() { return RegisterEvent<TEvent>(EventTransportDirection::Inbound); }
    template<typename TEvent> EventTransportRegistrationResult RegisterOutboundEvent() { return RegisterEvent<TEvent>(EventTransportDirection::Outbound); }
    template<typename TEvent> EventTransportRegistrationResult RegisterBidirectionalEvent() { return RegisterEvent<TEvent>(EventTransportDirection::Bidirectional); }
    template<typename TEvent> EventTransportRegistrationResult RegisterInboundEvent(IEventTransport* t) { return RegisterEvent<TEvent>(t, EventTransportDirection::Inbound); }
    template<typename TEvent> EventTransportRegistrationResult RegisterOutboundEvent(IEventTransport* t) { return RegisterEvent<TEvent>(t, EventTransportDirection::Outbound); }
    template<typename TEvent> EventTransportRegistrationResult RegisterBidirectionalEvent(IEventTransport* t) { return RegisterEvent<TEvent>(t, EventTransportDirection::Bidirectional); }
    template<typename... TEvents> EventTransportBulkOperationResult RegisterInboundEvents() { return RegisterEvents<TEvents...>(EventTransportDirection::Inbound); }
    template<typename... TEvents> EventTransportBulkOperationResult RegisterOutboundEvents() { return RegisterEvents<TEvents...>(EventTransportDirection::Outbound); }
    template<typename... TEvents> EventTransportBulkOperationResult RegisterBidirectionalEvents() { return RegisterEvents<TEvents...>(EventTransportDirection::Bidirectional); }
    template<typename... TEvents> EventTransportBulkOperationResult RegisterInboundEvents(IEventTransport* t) { return RegisterEvents<TEvents...>(t, EventTransportDirection::Inbound); }
    template<typename... TEvents> EventTransportBulkOperationResult RegisterOutboundEvents(IEventTransport* t) { return RegisterEvents<TEvents...>(t, EventTransportDirection::Outbound); }
    template<typename... TEvents> EventTransportBulkOperationResult RegisterBidirectionalEvents(IEventTransport* t) { return RegisterEvents<TEvents...>(t, EventTransportDirection::Bidirectional); }

    template<typename TEvent>
    EventTransportUnregistrationResult UnregisterEvent(
        EventTransportDirection direction,
        const EventTransportUnregistrationOptions& options = {}
    ) {
        static_assert(
            EventTransportTypeTraits<TEvent>::Name.size() != 0,
            "Unregistering a transported Event requires its stable transport type trait."
        );
        constexpr uint64_t typeID = EventTransportTypeID<TEvent>();
        EventTransportDirection before = EventTransportDirection::None;
        EventTransportDirection after = EventTransportDirection::None;
        EventTypeKey subscriptionType = nullptr;
        bool outboundRequired = false;
        bool remains = false;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            auto found = _registrations.find(typeID);
            if (found == _registrations.end()) {
                return EventTransportUnregistrationResult::NotRegistered;
            }
            before = found->second.DefaultDirection;
            after = RemoveDirection(before, direction);
            if (after == before) return EventTransportUnregistrationResult::NotRegistered;
            found->second.DefaultDirection = after;
            for (IEventTransport* transport : _transports) {
                DiscardInboundLocked(
                    typeID,
                    transport,
                    direction,
                    options,
                    HasDirection(
                        found->second.EffectiveDirection(transport),
                        EventTransportDirection::Inbound
                    )
                );
            }
            remains = found->second.HasAnyDirection();
            if (found->second.Runtime) {
                subscriptionType = found->second.Runtime->EventType;
                outboundRequired = found->second.HasOutboundDirection();
            }
            RemoveRegistrationIfUnusedLocked(typeID);
        }

        if (subscriptionType != nullptr) {
            SetOutboundSubscription(subscriptionType, outboundRequired);
        }
        if (_observable) {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnEventTransportTypeUnregistered(typeID, before, after);
            });
        }
        return remains
            ? EventTransportUnregistrationResult::Updated
            : EventTransportUnregistrationResult::Removed;
    }

    template<typename TEvent>
    EventTransportUnregistrationResult UnregisterEvent(
        IEventTransport* transport,
        EventTransportDirection direction,
        const EventTransportUnregistrationOptions& options = {}
    ) {
        static_assert(
            EventTransportTypeTraits<TEvent>::Name.size() != 0,
            "Unregistering a transported Event requires its stable transport type trait."
        );
        if (transport == nullptr) return EventTransportUnregistrationResult::InvalidTransport;
        constexpr uint64_t typeID = EventTransportTypeID<TEvent>();
        EventTransportDirection before = EventTransportDirection::None;
        EventTransportDirection after = EventTransportDirection::None;
        EventTypeKey subscriptionType = nullptr;
        bool outboundRequired = false;
        bool remains = false;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            auto found = _registrations.find(typeID);
            if (found == _registrations.end()) {
                return EventTransportUnregistrationResult::NotRegistered;
            }
            before = found->second.EffectiveDirection(transport);
            after = RemoveDirection(before, direction);
            if (after == before) return EventTransportUnregistrationResult::NotRegistered;
            found->second.TransportDirections[transport] = after;
            DiscardInboundLocked(
                typeID,
                transport,
                direction,
                options,
                HasDirection(after, EventTransportDirection::Inbound)
            );
            if (
                after == EventTransportDirection::None &&
                found->second.DefaultDirection == EventTransportDirection::None
            ) {
                found->second.TransportDirections.erase(transport);
            }
            remains = found->second.HasAnyDirection();
            if (found->second.Runtime) {
                subscriptionType = found->second.Runtime->EventType;
                outboundRequired = found->second.HasOutboundDirection();
            }
            RemoveRegistrationIfUnusedLocked(typeID);
        }

        if (subscriptionType != nullptr) {
            SetOutboundSubscription(subscriptionType, outboundRequired);
        }
        if (_observable) {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnEventTransportTypeRouteUnregistered(
                    typeID,
                    transport,
                    before,
                    after
                );
            });
        }
        return remains && after != EventTransportDirection::None
            ? EventTransportUnregistrationResult::Updated
            : EventTransportUnregistrationResult::Removed;
    }

    template<typename... TEvents>
    EventTransportBulkOperationResult UnregisterEvents(
        EventTransportDirection direction,
        const EventTransportUnregistrationOptions& options = {}
    ) {
        EventTransportBulkOperationResult result;
        result.Requested = sizeof...(TEvents);
        (AccumulateBulkResult(result, [&]() { return UnregisterEvent<TEvents>(direction, options); }), ...);
        return result;
    }

    template<typename... TEvents>
    EventTransportBulkOperationResult UnregisterEvents(
        IEventTransport* transport,
        EventTransportDirection direction,
        const EventTransportUnregistrationOptions& options = {}
    ) {
        EventTransportBulkOperationResult result;
        result.Requested = sizeof...(TEvents);
        (AccumulateBulkResult(result, [&]() { return UnregisterEvent<TEvents>(transport, direction, options); }), ...);
        return result;
    }

    template<typename TEvent> auto UnregisterInboundEvent(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(EventTransportDirection::Inbound, o); }
    template<typename TEvent> auto UnregisterOutboundEvent(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(EventTransportDirection::Outbound, o); }
    template<typename TEvent> auto UnregisterBidirectionalEvent(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(EventTransportDirection::Bidirectional, o); }
    template<typename TEvent> auto UnregisterInboundEvent(IEventTransport* t, const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(t, EventTransportDirection::Inbound, o); }
    template<typename TEvent> auto UnregisterOutboundEvent(IEventTransport* t, const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(t, EventTransportDirection::Outbound, o); }
    template<typename TEvent> auto UnregisterBidirectionalEvent(IEventTransport* t, const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(t, EventTransportDirection::Bidirectional, o); }
    template<typename... TEvents> auto UnregisterInboundEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(EventTransportDirection::Inbound, o); }
    template<typename... TEvents> auto UnregisterOutboundEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(EventTransportDirection::Outbound, o); }
    template<typename... TEvents> auto UnregisterBidirectionalEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(EventTransportDirection::Bidirectional, o); }
    template<typename... TEvents> auto UnregisterInboundEvents(IEventTransport* t, const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(t, EventTransportDirection::Inbound, o); }
    template<typename... TEvents> auto UnregisterOutboundEvents(IEventTransport* t, const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(t, EventTransportDirection::Outbound, o); }
    template<typename... TEvents> auto UnregisterBidirectionalEvents(IEventTransport* t, const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(t, EventTransportDirection::Bidirectional, o); }

    EventTransportBulkOperationResult UnregisterAllEvents(
        EventTransportDirection direction,
        const EventTransportUnregistrationOptions& options = {}
    ) {
        EventTransportBulkOperationResult result;
        TypeIdVector typeIDs;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            typeIDs.reserve(_registrations.size());
            for (const auto& entry : _registrations) typeIDs.push_back(entry.first);
        }
        result.Requested = typeIDs.size();
        for (uint64_t typeID : typeIDs) {
            EventTransportDirection before = EventTransportDirection::None;
            EventTransportDirection after = EventTransportDirection::None;
            EventTypeKey subscriptionType = nullptr;
            bool outboundRequired = false;
            bool changed = false;
            {
                std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
                auto found = _registrations.find(typeID);
                if (found == _registrations.end()) { ++result.Unchanged; continue; }
                before = found->second.DefaultDirection;
                after = RemoveDirection(before, direction);
                if (after == before) { ++result.Unchanged; continue; }
                found->second.DefaultDirection = after;
                for (IEventTransport* transport : _transports) {
                    DiscardInboundLocked(
                        typeID,
                        transport,
                        direction,
                        options,
                        HasDirection(
                            found->second.EffectiveDirection(transport),
                            EventTransportDirection::Inbound
                        )
                    );
                }
                if (found->second.Runtime) {
                    subscriptionType = found->second.Runtime->EventType;
                    outboundRequired = found->second.HasOutboundDirection();
                }
                RemoveRegistrationIfUnusedLocked(typeID);
                changed = true;
            }
            if (subscriptionType != nullptr) {
                SetOutboundSubscription(subscriptionType, outboundRequired);
            }
            if (changed) {
                ++result.Changed;
                if (_observable) {
                    _observable->Notify([&](IEventTransportManagerObserver* observer) {
                        observer->OnEventTransportTypeUnregistered(typeID, before, after);
                    });
                }
            }
        }
        return result;
    }

    EventTransportBulkOperationResult UnregisterAllEvents(
        IEventTransport* transport,
        EventTransportDirection direction,
        const EventTransportUnregistrationOptions& options = {}
    ) {
        EventTransportBulkOperationResult result;
        if (transport == nullptr) { result.Failed = 1; return result; }
        TypeIdVector typeIDs;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            typeIDs.reserve(_registrations.size());
            for (const auto& entry : _registrations) typeIDs.push_back(entry.first);
        }
        result.Requested = typeIDs.size();
        for (uint64_t typeID : typeIDs) {
            EventTransportDirection before = EventTransportDirection::None;
            EventTransportDirection after = EventTransportDirection::None;
            EventTypeKey subscriptionType = nullptr;
            bool outboundRequired = false;
            bool changed = false;
            {
                std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
                auto found = _registrations.find(typeID);
                if (found == _registrations.end()) { ++result.Unchanged; continue; }
                before = found->second.EffectiveDirection(transport);
                after = RemoveDirection(before, direction);
                if (after == before) { ++result.Unchanged; continue; }
                found->second.TransportDirections[transport] = after;
                DiscardInboundLocked(
                    typeID,
                    transport,
                    direction,
                    options,
                    HasDirection(after, EventTransportDirection::Inbound)
                );
                if (
                    after == EventTransportDirection::None &&
                    found->second.DefaultDirection == EventTransportDirection::None
                ) found->second.TransportDirections.erase(transport);
                if (found->second.Runtime) {
                    subscriptionType = found->second.Runtime->EventType;
                    outboundRequired = found->second.HasOutboundDirection();
                }
                RemoveRegistrationIfUnusedLocked(typeID);
                changed = true;
            }
            if (subscriptionType != nullptr) {
                SetOutboundSubscription(subscriptionType, outboundRequired);
            }
            if (changed) {
                ++result.Changed;
                if (_observable) {
                    _observable->Notify([&](IEventTransportManagerObserver* observer) {
                        observer->OnEventTransportTypeRouteUnregistered(
                            typeID,
                            transport,
                            before,
                            after
                        );
                    });
                }
            }
        }
        return result;
    }

    EventTransportBulkOperationResult UnregisterAllInboundEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterAllEvents(EventTransportDirection::Inbound, o); }
    EventTransportBulkOperationResult UnregisterAllOutboundEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterAllEvents(EventTransportDirection::Outbound, o); }
    EventTransportBulkOperationResult UnregisterAllBidirectionalEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterAllEvents(EventTransportDirection::Bidirectional, o); }
    EventTransportBulkOperationResult UnregisterAllInboundEvents(IEventTransport* t, const EventTransportUnregistrationOptions& o = {}) { return UnregisterAllEvents(t, EventTransportDirection::Inbound, o); }
    EventTransportBulkOperationResult UnregisterAllOutboundEvents(IEventTransport* t, const EventTransportUnregistrationOptions& o = {}) { return UnregisterAllEvents(t, EventTransportDirection::Outbound, o); }
    EventTransportBulkOperationResult UnregisterAllBidirectionalEvents(IEventTransport* t, const EventTransportUnregistrationOptions& o = {}) { return UnregisterAllEvents(t, EventTransportDirection::Bidirectional, o); }

    template<typename TEvent>
    EventTransportDirection GetEventTransportDirection() const {
        static_assert(
            EventTransportTypeTraits<TEvent>::Name.size() != 0,
            "Querying a transported Event requires its stable transport type trait."
        );
        constexpr uint64_t typeID = EventTransportTypeID<TEvent>();
        std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
        const auto found = _registrations.find(typeID);
        return found == _registrations.end()
            ? EventTransportDirection::None
            : found->second.DefaultDirection;
    }

    template<typename TEvent>
    EventTransportDirection GetEventTransportDirection(IEventTransport* transport) const {
        static_assert(
            EventTransportTypeTraits<TEvent>::Name.size() != 0,
            "Querying a transported Event requires its stable transport type trait."
        );
        if (transport == nullptr) return EventTransportDirection::None;
        constexpr uint64_t typeID = EventTransportTypeID<TEvent>();
        std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
        const auto found = _registrations.find(typeID);
        return found == _registrations.end()
            ? EventTransportDirection::None
            : found->second.EffectiveDirection(transport);
    }

    Observable::ObserverHandlePtr RegisterObserver(IEventTransportManagerObserver* observer) {
        return _observable->RegisterObserver(observer);
    }

    void UnregisterObserver(IEventTransportManagerObserver* observer) {
        _observable->UnregisterObserver(observer);
    }

    void ReceiveEventTransportPacket(
        IEventTransport* transport,
        EventTransportPacket packet
    ) override {
        if (!IsInitialized() || transport == nullptr || !packet) return;

        EventTransportEnvelope envelope;
        const uint8_t* payload = nullptr;
        if (!ParseEnvelope(packet, envelope, payload)) {
            _rejectedInboundCount.fetch_add(1, std::memory_order_relaxed);
            if (_observable) {
                _observable->Notify([&](IEventTransportManagerObserver* observer) {
                    observer->OnInboundPacketRejected(0, 0, transport);
                });
            }
            return;
        }

        bool accepted = false;
        RuntimeRegistrationPtr runtime;
        std::string_view typeName{};
        uint32_t schemaVersion = envelope.SchemaVersion;
        std::size_t inboundDepth = 0;
        {
            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
            if (
                IsTransportRegisteredLocked(transport) &&
                _inbound.size() < ESPRESSIO_EVENT_TRANSPORT_MAX_PENDING_INBOUND
            ) {
                const auto found = _registrations.find(envelope.EventTypeID);
                if (found != _registrations.end() && found->second.Runtime) {
                    runtime = found->second.Runtime;
                    typeName = runtime->TypeName;
                    schemaVersion = runtime->SchemaVersion;
                    if (HasDirection(
                        found->second.EffectiveDirection(transport),
                        EventTransportDirection::Inbound
                    )) {
                        _inbound.push_back(InboundWork{
                            transport,
                            envelope.EventTypeID,
                            runtime,
                            std::move(packet)
                        });
                        inboundDepth = _inbound.size();
                        accepted = true;
                    }
                }
            }
        }

        if (!accepted) {
            _rejectedInboundCount.fetch_add(1, std::memory_order_relaxed);
            if (_observable) {
                _observable->Notify([&](IEventTransportManagerObserver* observer) {
                    observer->OnInboundPacketRejected(
                        envelope.EventTypeID,
                        envelope.MessageID,
                        transport
                    );
                });
            }
            NotifyTransaction({
                EventTransportTransactionStage::InboundRejected,
                EventTransportDirection::Inbound,
                envelope.EventTypeID,
                typeName,
                schemaVersion,
                envelope.MessageID,
                transport,
                nullptr,
                payload,
                envelope.PayloadLength,
                static_cast<EventDispatchMethod>(envelope.DispatchMethod),
                static_cast<EventPriority>(envelope.Priority),
                EventOrigin::Remote,
                envelope.HopCount,
                false
            });
            return;
        }

        UpdatePeakInbound(inboundDepth);
        if (_observable) {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnInboundPacketAccepted(
                    envelope.EventTypeID,
                    envelope.MessageID,
                    transport
                );
            });
        }
        Wake();
    }

    std::size_t GetPendingInboundPacketCount() const {
        std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
        return _inbound.size();
    }

    std::size_t GetPeakInboundPacketCount() const noexcept {
        return _peakInboundCount.load(std::memory_order_relaxed);
    }

    uint64_t GetRejectedInboundPacketCount() const noexcept {
        return _rejectedInboundCount.load(std::memory_order_relaxed);
    }

    uint64_t GetProcessedInboundPacketCount() const noexcept {
        return _processedInboundCount.load(std::memory_order_relaxed);
    }

    uint64_t GetProcessedOutboundEventCount() const noexcept {
        return _processedOutboundCount.load(std::memory_order_relaxed);
    }

    uint64_t GetRejectedOutboundWorkCount() const noexcept {
        return _rejectedOutboundWorkCount.load(std::memory_order_relaxed);
    }

    Task::TaskExecutionStatistics GetOutboundExecutionStatistics() const {
        return _outboundExecutor.GetStatistics();
    }

    bool IsOutboundExecutorReady() const noexcept {
        return _outboundExecutorReady.load(std::memory_order_acquire);
    }
};

}
