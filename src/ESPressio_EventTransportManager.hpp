#pragma once

#if !__has_include(<ESPressio_Serializable.hpp>) || !__has_include(<ESPressio_DirectBinaryArchive.hpp>)
#error "ESPressio Event Transport requires ESPressio-Serializable >= 0.10.0 in the consuming project."
#endif

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <ESPressio_BinaryArchive.hpp>
#include <ESPressio_DirectBinaryArchive.hpp>
#include <ESPressio_TreeArchive.hpp>
#include <ESPressio_SchemaIntrospection.hpp>
#include <ESPressio_SerializationTraits.hpp>
#include <ESPressio_Thread.hpp>

#include "ESPressio_EventManager.hpp"
#include "ESPressio_EventTransportManagerObservable.hpp"
#include "ESPressio_EventTransportTypes.hpp"
#include "ESPressio_EventTypeKey.hpp"
#include "ESPressio_IEventManagerObserver.hpp"
#include "ESPressio_IEventTransport.hpp"
#include "ESPressio_SerializableEventDescriptor.hpp"

#ifndef ESPRESSIO_EVENT_TRANSPORT_MANAGER_PRIORITY
    #define ESPRESSIO_EVENT_TRANSPORT_MANAGER_PRIORITY 2
#endif

#ifndef ESPRESSIO_EVENT_TRANSPORT_MANAGER_CORE_ID
    #define ESPRESSIO_EVENT_TRANSPORT_MANAGER_CORE_ID 0
#endif

namespace ESPressio::Event {

class EventTransportManager final :
    public Threads::Thread,
    public IEventTransportReceiver,
    public IEventManagerObserver {

private:
    struct RuntimeRegistration {
        EventTypeKey EventType = nullptr;
        uint64_t TypeID = 0;
        std::string_view TypeName{};
        uint32_t SchemaVersion = 1;
        std::function<bool(IEvent*, std::vector<uint8_t>&)> Serialize;
        std::function<IEvent*(const uint8_t*, std::size_t)> Deserialize;
        std::vector<Serializable::PropertySchemaInfo> Properties;
        std::function<SerializableEventConstructionResult(
            const Serializable::SerializationNode&,
            const Serializable::DeserializationOptions&)>
            ConstructFromNode;
    };

    using RuntimeRegistrationPtr =
        std::shared_ptr<const RuntimeRegistration>;

    struct Registration {
        RuntimeRegistrationPtr Runtime;
        EventTransportDirection DefaultDirection = EventTransportDirection::None;
        std::unordered_map<IEventTransport*, EventTransportDirection>
            TransportDirections;

        EventTransportDirection EffectiveDirection(
            IEventTransport* transport
        ) const {
            const auto found = TransportDirections.find(transport);
            return found == TransportDirections.end()
                ? DefaultDirection
                : found->second;
        }

        bool HasAnyDirection() const {
            if (DefaultDirection != EventTransportDirection::None) {
                return true;
            }
            for (const auto& entry : TransportDirections) {
                if (entry.second != EventTransportDirection::None) {
                    return true;
                }
            }
            return false;
        }
    };

    struct OutboundWork {
        IEvent* Event = nullptr;
        IEventTransport* Transport = nullptr;
        uint64_t TypeID = 0;
        RuntimeRegistrationPtr Runtime;
        EventDispatchMethod Method = EventDispatchMethod::Queue;
        EventPriority Priority = EventPriority::Normal;
        uint64_t MessageID = 0;
    };

    struct InboundWork {
        IEventTransport* Transport = nullptr;
        uint64_t TypeID = 0;
        RuntimeRegistrationPtr Runtime;
        std::vector<uint8_t> Packet;
    };

    mutable std::mutex _mutex;
    std::unordered_map<uint64_t, Registration> _registrations;
    std::unordered_map<EventTypeKey, uint64_t> _runtimeTypes;
    std::vector<IEventTransport*> _transports;
    std::deque<OutboundWork> _outbound;
    std::deque<InboundWork> _inbound;
    std::atomic<TaskHandle_t> _notificationTask{nullptr};
    Observable::ObserverHandlePtr _eventManagerObserverHandle;
    std::shared_ptr<EventTransportManagerObservable> _observable =
        CreateEventTransportManagerObservable();
    std::atomic<uint64_t> _nextMessageID{1};
    bool _initialized = false;

    EventTransportManager() : Threads::Thread(false) {
        SetPriority(ESPRESSIO_EVENT_TRANSPORT_MANAGER_PRIORITY);
        SetCoreID(ESPRESSIO_EVENT_TRANSPORT_MANAGER_CORE_ID);
    }

    static bool ParseEnvelope(
        const uint8_t* data,
        std::size_t size,
        EventTransportEnvelope& envelope,
        const uint8_t*& payload
    ) {
        if (data == nullptr || size < sizeof(EventTransportEnvelope)) {
            return false;
        }
        std::memcpy(&envelope, data, sizeof(envelope));
        if (
            envelope.Magic != EventTransportEnvelope::MagicValue ||
            envelope.Version != EventTransportEnvelope::CurrentVersion ||
            envelope.PayloadLength != size - sizeof(EventTransportEnvelope)
        ) {
            return false;
        }
        payload = data + sizeof(EventTransportEnvelope);
        return true;
    }

    bool HasPendingWork() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return !_inbound.empty() || !_outbound.empty();
    }

    void Wake() {
        const TaskHandle_t task =
            _notificationTask.load(std::memory_order_acquire);
        if (task != nullptr) {
            xTaskNotifyGive(task);
        }
    }

    void ReleaseOutbound(OutboundWork& work) noexcept {
        if (work.Event != nullptr) {
            work.Event->__unref();
            work.Event = nullptr;
        }
    }

    bool IsTransportRegisteredLocked(IEventTransport* transport) const {
        return std::find(
            _transports.begin(),
            _transports.end(),
            transport
        ) != _transports.end();
    }

    void RemoveRegistrationIfUnusedLocked(uint64_t typeID) {
        auto found = _registrations.find(typeID);
        if (found == _registrations.end() || found->second.HasAnyDirection()) {
            return;
        }

        const RuntimeRegistrationPtr runtime = found->second.Runtime;
        if (
            runtime &&
            (runtime->ConstructFromNode || !runtime->Properties.empty())
        ) {
            return;
        }

        if (runtime && runtime->EventType != nullptr) {
            _runtimeTypes.erase(runtime->EventType);
        }
        _registrations.erase(found);
    }

    void NotifyTransaction(
        const EventTransportTransaction& transaction
    ) {
        _observable->Notify(
            [&](IEventTransportManagerObserver* observer) {
                observer->OnEventTransportTransaction(transaction);
            }
        );
    }

    void ProcessOutbound(OutboundWork work) {
        const RuntimeRegistrationPtr& runtime = work.Runtime;
        if (
            work.Transport == nullptr ||
            !runtime ||
            !runtime->Serialize
        ) {
            NotifyTransaction({
                EventTransportTransactionStage::Failed,
                EventTransportDirection::Outbound,
                work.TypeID,
                runtime ? runtime->TypeName : std::string_view{},
                runtime ? runtime->SchemaVersion : 0,
                work.MessageID,
                work.Transport,
                work.Event,
                nullptr,
                0,
                work.Method,
                work.Priority,
                EventOrigin::Local,
                0,
                false
            });
            ReleaseOutbound(work);
            return;
        }

        std::vector<uint8_t> bytes(sizeof(EventTransportEnvelope));
        if (!runtime->Serialize(work.Event, bytes)) {
            NotifyTransaction({
                EventTransportTransactionStage::Failed,
                EventTransportDirection::Outbound,
                work.TypeID,
                runtime->TypeName,
                runtime->SchemaVersion,
                work.MessageID,
                work.Transport,
                work.Event,
                nullptr,
                0,
                work.Method,
                work.Priority,
                EventOrigin::Local,
                0,
                false
            });
            ReleaseOutbound(work);
            return;
        }

        const std::size_t payloadSize =
            bytes.size() - sizeof(EventTransportEnvelope);
        uint8_t* payload =
            bytes.data() + sizeof(EventTransportEnvelope);

        NotifyTransaction({
            EventTransportTransactionStage::OutboundSerialized,
            EventTransportDirection::Outbound,
            work.TypeID,
            runtime->TypeName,
            runtime->SchemaVersion,
            work.MessageID,
            work.Transport,
            work.Event,
            payload,
            payloadSize,
            work.Method,
            work.Priority,
            EventOrigin::Local,
            0,
            false
        });

        EventTransportEnvelope envelope;
        envelope.EventTypeID = runtime->TypeID;
        envelope.SchemaVersion = runtime->SchemaVersion;
        envelope.MessageID = work.MessageID;
        envelope.DispatchMethod = static_cast<uint8_t>(work.Method);
        envelope.Priority = static_cast<uint8_t>(work.Priority);
        envelope.HopCount = 0;
        envelope.PayloadLength = static_cast<uint32_t>(payloadSize);
        std::memcpy(bytes.data(), &envelope, sizeof(envelope));

        EventTransportPacket packet{
            bytes.data(),
            bytes.size(),
            work.MessageID
        };

        const bool accepted = work.Transport->Send(packet);

        _observable->Notify(
            [&](IEventTransportManagerObserver* observer) {
                observer->OnOutboundEventHandedToTransport(
                    work.TypeID,
                    work.MessageID,
                    work.Transport,
                    accepted
                );
            }
        );

        NotifyTransaction({
            EventTransportTransactionStage::OutboundHandedToTransport,
            EventTransportDirection::Outbound,
            work.TypeID,
            runtime->TypeName,
            runtime->SchemaVersion,
            work.MessageID,
            work.Transport,
            work.Event,
            payload,
            payloadSize,
            work.Method,
            work.Priority,
            EventOrigin::Local,
            0,
            accepted
        });

        ReleaseOutbound(work);
    }

    void ProcessInbound(InboundWork work) {
        EventTransportEnvelope envelope;
        const uint8_t* payload = nullptr;
        if (!ParseEnvelope(
            work.Packet.data(),
            work.Packet.size(),
            envelope,
            payload
        )) {
            return;
        }

        const RuntimeRegistrationPtr& runtime = work.Runtime;
        if (!runtime || !runtime->Deserialize) {
            return;
        }

        IEvent* event = runtime->Deserialize(
            payload,
            envelope.PayloadLength
        );

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

        _observable->Notify(
            [&](IEventTransportManagerObserver* observer) {
                observer->OnInboundEventDeserialized(
                    envelope.EventTypeID,
                    envelope.MessageID
                );
            }
        );

        NotifyTransaction({
            EventTransportTransactionStage::InboundDeserialized,
            EventTransportDirection::Inbound,
            envelope.EventTypeID,
            runtime->TypeName,
            envelope.SchemaVersion,
            envelope.MessageID,
            work.Transport,
            event,
            payload,
            envelope.PayloadLength,
            static_cast<EventDispatchMethod>(envelope.DispatchMethod),
            static_cast<EventPriority>(envelope.Priority),
            EventOrigin::Remote,
            envelope.HopCount,
            true
        });

        EventDispatchContext context;
        context.Origin = EventOrigin::Remote;
        context.TransportMessageID = envelope.MessageID;
        context.HopCount = envelope.HopCount;
        event->__setDispatchContext(context);

        if (
            envelope.DispatchMethod >
                static_cast<uint8_t>(EventDispatchMethod::Queue) ||
            envelope.Priority >
                static_cast<uint8_t>(EventPriority::High)
        ) {
            delete event;
            return;
        }

        const auto method =
            static_cast<EventDispatchMethod>(envelope.DispatchMethod);
        const auto priority =
            static_cast<EventPriority>(envelope.Priority);

        if (method == EventDispatchMethod::Stack) {
            event->Stack(priority);
        } else {
            event->Queue(priority);
        }

        _observable->Notify(
            [&](IEventTransportManagerObserver* observer) {
                observer->OnInboundEventDispatched(
                    envelope.EventTypeID,
                    envelope.MessageID
                );
            }
        );

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
    }

    void OnLoop() override {
        _notificationTask.store(
            xTaskGetCurrentTaskHandle(),
            std::memory_order_release
        );

        if (!HasPendingWork()) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        } else {
            ulTaskNotifyTake(pdTRUE, 0);
        }

        for (;;) {
            OutboundWork outbound;
            InboundWork inbound;
            bool hasOutbound = false;
            bool hasInbound = false;

            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (!_inbound.empty()) {
                    inbound = std::move(_inbound.front());
                    _inbound.pop_front();
                    hasInbound = true;
                } else if (!_outbound.empty()) {
                    outbound = std::move(_outbound.front());
                    _outbound.pop_front();
                    hasOutbound = true;
                }
            }

            if (hasInbound) {
                ProcessInbound(std::move(inbound));
            } else if (hasOutbound) {
                ProcessOutbound(std::move(outbound));
            } else {
                break;
            }
        }
    }

    void DropPendingForTransportLocked(
        uint64_t typeID,
        IEventTransport* transport,
        EventTransportDirection removedDirection,
        const EventTransportUnregistrationOptions& options,
        bool outboundStillAllowed,
        bool inboundStillAllowed,
        std::vector<OutboundWork>& discardedOutbound,
        std::size_t& discardedInbound
    ) {
        if (
            HasDirection(removedDirection, EventTransportDirection::Outbound) &&
            !outboundStillAllowed &&
            options.PendingOutbound == EventTransportPendingAction::Discard
        ) {
            auto current = _outbound.begin();
            while (current != _outbound.end()) {
                if (
                    current->TypeID == typeID &&
                    current->Transport == transport
                ) {
                    discardedOutbound.push_back(std::move(*current));
                    current = _outbound.erase(current);
                } else {
                    ++current;
                }
            }
        }

        if (
            HasDirection(removedDirection, EventTransportDirection::Inbound) &&
            !inboundStillAllowed &&
            options.PendingInbound == EventTransportPendingAction::Discard
        ) {
            const auto before = _inbound.size();
            _inbound.erase(
                std::remove_if(
                    _inbound.begin(),
                    _inbound.end(),
                    [&](const InboundWork& work) {
                        return
                            work.TypeID == typeID &&
                            work.Transport == transport;
                    }
                ),
                _inbound.end()
            );
            discardedInbound += before - _inbound.size();
        }
    }

    template<typename TEvent>
    static Registration CreateRegistration(
        EventTransportDirection defaultDirection
    ) {
        auto runtime = std::make_shared<RuntimeRegistration>();
        runtime->EventType = EventTypeKeyOf<TEvent>();
        runtime->TypeID = EventTransportTypeID<TEvent>();
        runtime->TypeName = EventTransportTypeTraits<TEvent>::Name;
        runtime->SchemaVersion = TEvent::GetSchemaVersion();
        runtime->Properties = Serializable::SchemaInspector<TEvent>::Properties();

        runtime->Serialize =
            [](IEvent* event, std::vector<uint8_t>& bytes) {
                if (
                    event == nullptr ||
                    event->__getTypeKey() != EventTypeKeyOf<TEvent>()
                ) {
                    return false;
                }

                auto* typed = static_cast<TEvent*>(event);
                const std::size_t prefix = bytes.size();
                if (Serializable::AppendDirectBinary(*typed, bytes)) {
                    return bytes.size() > prefix;
                }

                bytes.resize(prefix);
                Serializable::BinaryArchive archive;
                typed->Serialize(archive);
                const auto& payload = archive.GetData();
                if (payload.empty()) {
                    return false;
                }
                bytes.insert(bytes.end(), payload.begin(), payload.end());
                return true;
            };

        if constexpr (std::is_default_constructible_v<TEvent>) {
            runtime->Deserialize =
                [](const uint8_t* data, std::size_t size) -> IEvent* {
                    auto event = std::make_unique<TEvent>();
                    const auto direct =
                        Serializable::DeserializeDirectBinary(
                            data,
                            size,
                            *event
                        );
                    if (direct.Success()) {
                        return event.release();
                    }

                    Serializable::BinaryArchive archive;
                    if (!archive.Load(data, size)) {
                        return nullptr;
                    }
                    event = std::make_unique<TEvent>();
                    if (!event->Deserialize(archive)) {
                        return nullptr;
                    }
                    return event.release();
                };

            runtime->ConstructFromNode =
                [](const Serializable::SerializationNode& node,
                   const Serializable::DeserializationOptions& options)
                    -> SerializableEventConstructionResult {
                    SerializableEventConstructionResult result;
                    result.TypeRegistered = true;
                    result.Constructible = true;
                    auto event = std::make_unique<TEvent>();
                    Serializable::TreeArchive archive;
                    archive.GetNode() = node;
                    result.Deserialization =
                        event->DeserializeDetailed(archive, options);
                    if (result.Deserialization.Success()) {
                        result.Event = std::move(event);
                    }
                    return result;
                };
        }

        Registration proposed;
        proposed.Runtime = std::move(runtime);
        proposed.DefaultDirection = defaultDirection;
        return proposed;
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
            std::lock_guard<std::mutex> lock(_mutex);
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

        if (result == EventTransportRegistrationResult::Registered) {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnEventTransportTypeRegistered(typeID, after);
            });
        } else {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnEventTransportTypeRegistrationChanged(
                    typeID, before, after
                );
            });
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
        if (transport == nullptr) {
            return EventTransportRegistrationResult::InvalidTransport;
        }
        if (requireInboundFactory && !std::is_default_constructible_v<TEvent>) {
            return EventTransportRegistrationResult::TypeConflict;
        }

        constexpr uint64_t typeID = EventTransportTypeID<TEvent>();
        Registration proposed =
            CreateRegistration<TEvent>(EventTransportDirection::None);
        const EventTypeKey eventType = proposed.Runtime->EventType;
        EventTransportDirection before = EventTransportDirection::None;
        EventTransportDirection after = EventTransportDirection::None;
        bool createdOverride = false;

        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto found = _registrations.find(typeID);
            if (found == _registrations.end()) {
                proposed.TransportDirections[transport] = direction;
                _registrations.emplace(typeID, std::move(proposed));
                _runtimeTypes[eventType] = typeID;
                after = direction;
                createdOverride = true;
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
                    createdOverride = true;
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

        if (createdOverride) {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnEventTransportTypeRouteRegistered(
                    typeID, transport, after
                );
            });
            return EventTransportRegistrationResult::Registered;
        }
        _observable->Notify([&](IEventTransportManagerObserver* observer) {
            observer->OnEventTransportTypeRouteChanged(
                typeID, transport, before, after
            );
        });
        return EventTransportRegistrationResult::Updated;
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

public:
    ~EventTransportManager() override {
        Shutdown();
        Threads::Thread::Shutdown();
        _notificationTask.store(nullptr, std::memory_order_release);
    }

    EventTransportManager(const EventTransportManager&) = delete;
    EventTransportManager& operator=(const EventTransportManager&) = delete;

    static EventTransportManager& GetInstance() {
        static EventTransportManager instance;
        return instance;
    }

    Threads::ThreadInitializationStatus Initialize() override {
        if (_initialized) {
            return Threads::ThreadInitializationStatus::AlreadyInitialized;
        }
        _eventManagerObserverHandle =
            EventManager::GetInstance()->RegisterObserver(this);
        if (!_eventManagerObserverHandle) {
            return Threads::ThreadInitializationStatus::InitializationException;
        }
        const auto initializationStatus = Threads::Thread::Initialize();
        if (
            initializationStatus != Threads::ThreadInitializationStatus::Success &&
            initializationStatus != Threads::ThreadInitializationStatus::AlreadyInitialized
        ) {
            _eventManagerObserverHandle.reset();
            return initializationStatus;
        }
        if (
            GetThreadState() == Threads::ThreadState::Initialized ||
            GetThreadState() == Threads::ThreadState::Paused
        ) {
            const auto startStatus = Threads::Thread::Start();
            if (
                startStatus != Threads::ThreadInitializationStatus::Success &&
                startStatus != Threads::ThreadInitializationStatus::AlreadyInitialized
            ) {
                _eventManagerObserverHandle.reset();
                return startStatus;
            }
        }
        if (GetThreadState() != Threads::ThreadState::Running) {
            _eventManagerObserverHandle.reset();
            return Threads::ThreadInitializationStatus::InvalidState;
        }
        _initialized = true;
        return Threads::ThreadInitializationStatus::Success;
    }

    bool IsInitialized() const noexcept {
        return _initialized;
    }

    void Shutdown() {
        if (!_initialized) {
            return;
        }
        _eventManagerObserverHandle.reset();
        std::deque<OutboundWork> outbound;
        std::vector<IEventTransport*> transports;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            outbound.swap(_outbound);
            _inbound.clear();
            transports.swap(_transports);
            _initialized = false;
        }
        for (auto& work : outbound) {
            ReleaseOutbound(work);
        }
        for (auto* transport : transports) {
            if (transport != nullptr) {
                transport->SetReceiver(nullptr);
            }
        }
    }

    bool RegisterTransport(IEventTransport* transport) {
        if (transport == nullptr) {
            return false;
        }
        bool added = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!IsTransportRegisteredLocked(transport)) {
                _transports.push_back(transport);
                added = true;
            }
        }
        if (added) {
            transport->SetReceiver(this);
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnEventTransportRegistered(transport);
            });
        }
        return true;
    }

    void UnregisterTransport(IEventTransport* transport) {
        if (transport == nullptr) {
            return;
        }
        bool removed = false;
        std::vector<OutboundWork> discardedOutbound;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            const auto oldSize = _transports.size();
            _transports.erase(
                std::remove(_transports.begin(), _transports.end(), transport),
                _transports.end()
            );
            removed = _transports.size() != oldSize;
            if (removed) {
                auto current = _outbound.begin();
                while (current != _outbound.end()) {
                    if (current->Transport == transport) {
                        discardedOutbound.push_back(std::move(*current));
                        current = _outbound.erase(current);
                    } else {
                        ++current;
                    }
                }
                _inbound.erase(
                    std::remove_if(
                        _inbound.begin(), _inbound.end(),
                        [&](const InboundWork& work) {
                            return work.Transport == transport;
                        }
                    ),
                    _inbound.end()
                );
            }
        }
        for (auto& work : discardedOutbound) {
            ReleaseOutbound(work);
        }
        if (removed) {
            transport->SetReceiver(nullptr);
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnEventTransportUnregistered(transport);
            });
        }
    }

    std::vector<SerializableEventDescriptor>
    GetRegisteredSerializableEvents() const {
        std::vector<SerializableEventDescriptor> descriptors;
        std::lock_guard<std::mutex> lock(_mutex);
        descriptors.reserve(_registrations.size());
        for (const auto& entry : _registrations) {
            const auto& registration = entry.second;
            const RuntimeRegistrationPtr& runtime = registration.Runtime;
            if (!runtime) continue;
            SerializableEventDescriptor descriptor;
            descriptor.TypeID = runtime->TypeID;
            descriptor.TypeName = std::string(runtime->TypeName);
            descriptor.SchemaVersion = runtime->SchemaVersion;
            descriptor.DefaultDirection = registration.DefaultDirection;
            descriptor.Properties = runtime->Properties;
            descriptor.CanConstruct = static_cast<bool>(runtime->ConstructFromNode);
            descriptors.push_back(std::move(descriptor));
        }
        std::sort(
            descriptors.begin(), descriptors.end(),
            [](const auto& a, const auto& b) {
                return a.TypeName < b.TypeName;
            }
        );
        return descriptors;
    }

    bool FindRegisteredSerializableEvent(
        uint64_t typeID,
        SerializableEventDescriptor& descriptor
    ) const {
        std::lock_guard<std::mutex> lock(_mutex);
        const auto found = _registrations.find(typeID);
        if (
            found == _registrations.end() ||
            !found->second.Runtime
        ) {
            return false;
        }
        const auto& registration = found->second;
        const RuntimeRegistrationPtr& runtime = registration.Runtime;
        descriptor.TypeID = runtime->TypeID;
        descriptor.TypeName = std::string(runtime->TypeName);
        descriptor.SchemaVersion = runtime->SchemaVersion;
        descriptor.DefaultDirection = registration.DefaultDirection;
        descriptor.Properties = runtime->Properties;
        descriptor.CanConstruct = static_cast<bool>(runtime->ConstructFromNode);
        return true;
    }

    bool FindRegisteredSerializableEvent(
        std::string_view typeName,
        SerializableEventDescriptor& descriptor
    ) const {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto& entry : _registrations) {
            const auto& registration = entry.second;
            const RuntimeRegistrationPtr& runtime = registration.Runtime;
            if (!runtime || runtime->TypeName != typeName) {
                continue;
            }
            descriptor.TypeID = runtime->TypeID;
            descriptor.TypeName = std::string(runtime->TypeName);
            descriptor.SchemaVersion = runtime->SchemaVersion;
            descriptor.DefaultDirection = registration.DefaultDirection;
            descriptor.Properties = runtime->Properties;
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
            std::lock_guard<std::mutex> lock(_mutex);
            const auto found = _registrations.find(typeID);
            if (found == _registrations.end()) {
                return {};
            }
            runtime = found->second.Runtime;
        }

        if (!runtime) {
            return {};
        }
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
            std::lock_guard<std::mutex> lock(_mutex);
            for (const auto& entry : _registrations) {
                if (
                    entry.second.Runtime &&
                    entry.second.Runtime->TypeName == typeName
                ) {
                    runtime = entry.second.Runtime;
                    break;
                }
            }
        }

        if (!runtime) {
            return {};
        }
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
        if (!event) {
            return RuntimeEventDispatchResult::NullEvent;
        }
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
    EventTransportRegistrationResult RegisterEvent(
        EventTransportDirection direction
    ) {
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
    EventTransportBulkOperationResult RegisterEvents(
        EventTransportDirection direction
    ) {
        EventTransportBulkOperationResult result;
        result.Requested = sizeof...(TEvents);
        (AccumulateBulkResult(
            result,
            [&]() { return RegisterEvent<TEvents>(direction); }
        ), ...);
        return result;
    }

    template<typename... TEvents>
    EventTransportBulkOperationResult RegisterEvents(
        IEventTransport* transport,
        EventTransportDirection direction
    ) {
        EventTransportBulkOperationResult result;
        result.Requested = sizeof...(TEvents);
        (AccumulateBulkResult(
            result,
            [&]() { return RegisterEvent<TEvents>(transport, direction); }
        ), ...);
        return result;
    }

    template<typename TEvent>
    EventTransportRegistrationResult RegisterInboundEvent() {
        static_assert(std::is_default_constructible_v<TEvent>,
            "Inbound transported Events must be default constructible.");
        return RegisterGlobalEventImpl<TEvent>(EventTransportDirection::Inbound, true);
    }
    template<typename TEvent>
    EventTransportRegistrationResult RegisterOutboundEvent() {
        return RegisterGlobalEventImpl<TEvent>(EventTransportDirection::Outbound, false);
    }
    template<typename TEvent>
    EventTransportRegistrationResult RegisterBidirectionalEvent() {
        static_assert(std::is_default_constructible_v<TEvent>,
            "Bidirectionally transported Events must be default constructible.");
        return RegisterGlobalEventImpl<TEvent>(EventTransportDirection::Bidirectional, true);
    }
    template<typename TEvent>
    EventTransportRegistrationResult RegisterInboundEvent(IEventTransport* transport) {
        static_assert(std::is_default_constructible_v<TEvent>,
            "Inbound transported Events must be default constructible.");
        return RegisterTransportEventImpl<TEvent>(transport, EventTransportDirection::Inbound, true);
    }
    template<typename TEvent>
    EventTransportRegistrationResult RegisterOutboundEvent(IEventTransport* transport) {
        return RegisterTransportEventImpl<TEvent>(transport, EventTransportDirection::Outbound, false);
    }
    template<typename TEvent>
    EventTransportRegistrationResult RegisterBidirectionalEvent(IEventTransport* transport) {
        static_assert(std::is_default_constructible_v<TEvent>,
            "Bidirectionally transported Events must be default constructible.");
        return RegisterTransportEventImpl<TEvent>(transport, EventTransportDirection::Bidirectional, true);
    }

    template<typename... TEvents>
    EventTransportBulkOperationResult RegisterInboundEvents() {
        return RegisterEvents<TEvents...>(EventTransportDirection::Inbound);
    }
    template<typename... TEvents>
    EventTransportBulkOperationResult RegisterOutboundEvents() {
        return RegisterEvents<TEvents...>(EventTransportDirection::Outbound);
    }
    template<typename... TEvents>
    EventTransportBulkOperationResult RegisterBidirectionalEvents() {
        return RegisterEvents<TEvents...>(EventTransportDirection::Bidirectional);
    }
    template<typename... TEvents>
    EventTransportBulkOperationResult RegisterInboundEvents(IEventTransport* transport) {
        return RegisterEvents<TEvents...>(transport, EventTransportDirection::Inbound);
    }
    template<typename... TEvents>
    EventTransportBulkOperationResult RegisterOutboundEvents(IEventTransport* transport) {
        return RegisterEvents<TEvents...>(transport, EventTransportDirection::Outbound);
    }
    template<typename... TEvents>
    EventTransportBulkOperationResult RegisterBidirectionalEvents(IEventTransport* transport) {
        return RegisterEvents<TEvents...>(transport, EventTransportDirection::Bidirectional);
    }

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
        EventTransportUnregistrationResult result;
        std::vector<OutboundWork> discardedOutbound;
        std::size_t discardedInbound = 0;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto found = _registrations.find(typeID);
            if (found == _registrations.end()) {
                return EventTransportUnregistrationResult::NotRegistered;
            }
            before = found->second.DefaultDirection;
            after = RemoveDirection(before, direction);
            found->second.DefaultDirection = after;
            for (IEventTransport* transport : _transports) {
                const auto effectiveAfter = found->second.EffectiveDirection(transport);
                DropPendingForTransportLocked(
                    typeID,
                    transport,
                    direction,
                    options,
                    HasDirection(effectiveAfter, EventTransportDirection::Outbound),
                    HasDirection(effectiveAfter, EventTransportDirection::Inbound),
                    discardedOutbound,
                    discardedInbound
                );
            }
            result = found->second.HasAnyDirection()
                ? EventTransportUnregistrationResult::Updated
                : EventTransportUnregistrationResult::Removed;
            RemoveRegistrationIfUnusedLocked(typeID);
        }
        for (auto& work : discardedOutbound) {
            ReleaseOutbound(work);
        }
        (void)discardedInbound;
        _observable->Notify([&](IEventTransportManagerObserver* observer) {
            observer->OnEventTransportTypeUnregistered(typeID, before, after);
        });
        return result;
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
        if (transport == nullptr) {
            return EventTransportUnregistrationResult::InvalidTransport;
        }
        constexpr uint64_t typeID = EventTransportTypeID<TEvent>();
        EventTransportDirection before = EventTransportDirection::None;
        EventTransportDirection after = EventTransportDirection::None;
        EventTransportUnregistrationResult result =
            EventTransportUnregistrationResult::NotRegistered;
        std::vector<OutboundWork> discardedOutbound;
        std::size_t discardedInbound = 0;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto found = _registrations.find(typeID);
            if (found == _registrations.end()) {
                return EventTransportUnregistrationResult::NotRegistered;
            }
            before = found->second.EffectiveDirection(transport);
            if (
                !HasDirection(before, direction) &&
                direction != EventTransportDirection::Bidirectional
            ) {
                return EventTransportUnregistrationResult::NotRegistered;
            }
            after = RemoveDirection(before, direction);
            found->second.TransportDirections[transport] = after;
            DropPendingForTransportLocked(
                typeID, transport, direction, options,
                HasDirection(after, EventTransportDirection::Outbound),
                HasDirection(after, EventTransportDirection::Inbound),
                discardedOutbound, discardedInbound
            );
            if (
                after == EventTransportDirection::None &&
                found->second.DefaultDirection == EventTransportDirection::None
            ) {
                found->second.TransportDirections.erase(transport);
            }
            result = found->second.HasAnyDirection()
                ? (after == EventTransportDirection::None
                    ? EventTransportUnregistrationResult::Removed
                    : EventTransportUnregistrationResult::Updated)
                : EventTransportUnregistrationResult::Removed;
            RemoveRegistrationIfUnusedLocked(typeID);
        }
        for (auto& work : discardedOutbound) {
            ReleaseOutbound(work);
        }
        (void)discardedInbound;
        _observable->Notify([&](IEventTransportManagerObserver* observer) {
            observer->OnEventTransportTypeRouteUnregistered(
                typeID, transport, before, after
            );
        });
        return result;
    }

    template<typename... TEvents>
    EventTransportBulkOperationResult UnregisterEvents(
        EventTransportDirection direction,
        const EventTransportUnregistrationOptions& options = {}
    ) {
        EventTransportBulkOperationResult result;
        result.Requested = sizeof...(TEvents);
        (AccumulateBulkResult(
            result,
            [&]() { return UnregisterEvent<TEvents>(direction, options); }
        ), ...);
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
        (AccumulateBulkResult(
            result,
            [&]() { return UnregisterEvent<TEvents>(transport, direction, options); }
        ), ...);
        return result;
    }

    template<typename TEvent> auto UnregisterInboundEvent(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(EventTransportDirection::Inbound,o); }
    template<typename TEvent> auto UnregisterOutboundEvent(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(EventTransportDirection::Outbound,o); }
    template<typename TEvent> auto UnregisterBidirectionalEvent(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(EventTransportDirection::Bidirectional,o); }
    template<typename TEvent> auto UnregisterInboundEvent(IEventTransport* t,const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(t,EventTransportDirection::Inbound,o); }
    template<typename TEvent> auto UnregisterOutboundEvent(IEventTransport* t,const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(t,EventTransportDirection::Outbound,o); }
    template<typename TEvent> auto UnregisterBidirectionalEvent(IEventTransport* t,const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvent<TEvent>(t,EventTransportDirection::Bidirectional,o); }
    template<typename... TEvents> auto UnregisterInboundEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(EventTransportDirection::Inbound,o); }
    template<typename... TEvents> auto UnregisterOutboundEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(EventTransportDirection::Outbound,o); }
    template<typename... TEvents> auto UnregisterBidirectionalEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(EventTransportDirection::Bidirectional,o); }
    template<typename... TEvents> auto UnregisterInboundEvents(IEventTransport* t,const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(t,EventTransportDirection::Inbound,o); }
    template<typename... TEvents> auto UnregisterOutboundEvents(IEventTransport* t,const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(t,EventTransportDirection::Outbound,o); }
    template<typename... TEvents> auto UnregisterBidirectionalEvents(IEventTransport* t,const EventTransportUnregistrationOptions& o = {}) { return UnregisterEvents<TEvents...>(t,EventTransportDirection::Bidirectional,o); }

    EventTransportBulkOperationResult UnregisterAllEvents(
        EventTransportDirection direction,
        const EventTransportUnregistrationOptions& options = {}
    ) {
        EventTransportBulkOperationResult result;
        std::vector<uint64_t> typeIDs;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            typeIDs.reserve(_registrations.size());
            for (const auto& entry : _registrations) {
                typeIDs.push_back(entry.first);
            }
        }
        result.Requested = typeIDs.size();
        for (uint64_t typeID : typeIDs) {
            EventTransportDirection before = EventTransportDirection::None;
            EventTransportDirection after = EventTransportDirection::None;
            std::vector<OutboundWork> discardedOutbound;
            std::size_t discardedInbound = 0;
            bool changed = false;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                auto found = _registrations.find(typeID);
                if (found == _registrations.end()) {
                    ++result.Unchanged;
                    continue;
                }
                before = found->second.DefaultDirection;
                after = RemoveDirection(before, direction);
                if (before == after) {
                    ++result.Unchanged;
                    continue;
                }
                found->second.DefaultDirection = after;
                for (IEventTransport* transport : _transports) {
                    const auto effectiveAfter = found->second.EffectiveDirection(transport);
                    DropPendingForTransportLocked(
                        typeID, transport, direction, options,
                        HasDirection(effectiveAfter, EventTransportDirection::Outbound),
                        HasDirection(effectiveAfter, EventTransportDirection::Inbound),
                        discardedOutbound, discardedInbound
                    );
                }
                RemoveRegistrationIfUnusedLocked(typeID);
                changed = true;
            }
            for (auto& work : discardedOutbound) {
                ReleaseOutbound(work);
            }
            (void)discardedInbound;
            if (changed) {
                ++result.Changed;
                _observable->Notify([&](IEventTransportManagerObserver* observer) {
                    observer->OnEventTransportTypeUnregistered(typeID,before,after);
                });
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
        if (transport == nullptr) {
            result.Failed = 1;
            return result;
        }
        std::vector<uint64_t> typeIDs;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            typeIDs.reserve(_registrations.size());
            for (const auto& entry : _registrations) {
                typeIDs.push_back(entry.first);
            }
        }
        result.Requested = typeIDs.size();
        for (uint64_t typeID : typeIDs) {
            EventTransportDirection before = EventTransportDirection::None;
            EventTransportDirection after = EventTransportDirection::None;
            std::vector<OutboundWork> discardedOutbound;
            std::size_t discardedInbound = 0;
            bool changed = false;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                auto found = _registrations.find(typeID);
                if (found == _registrations.end()) {
                    ++result.Unchanged;
                    continue;
                }
                before = found->second.EffectiveDirection(transport);
                after = RemoveDirection(before, direction);
                if (before == after) {
                    ++result.Unchanged;
                    continue;
                }
                found->second.TransportDirections[transport] = after;
                DropPendingForTransportLocked(
                    typeID, transport, direction, options,
                    HasDirection(after, EventTransportDirection::Outbound),
                    HasDirection(after, EventTransportDirection::Inbound),
                    discardedOutbound, discardedInbound
                );
                if (
                    after == EventTransportDirection::None &&
                    found->second.DefaultDirection == EventTransportDirection::None
                ) {
                    found->second.TransportDirections.erase(transport);
                }
                RemoveRegistrationIfUnusedLocked(typeID);
                changed = true;
            }
            for (auto& work : discardedOutbound) {
                ReleaseOutbound(work);
            }
            (void)discardedInbound;
            if (changed) {
                ++result.Changed;
                _observable->Notify([&](IEventTransportManagerObserver* observer) {
                    observer->OnEventTransportTypeRouteUnregistered(
                        typeID,transport,before,after
                    );
                });
            }
        }
        return result;
    }

    EventTransportBulkOperationResult UnregisterAllInboundEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterAllEvents(EventTransportDirection::Inbound,o); }
    EventTransportBulkOperationResult UnregisterAllOutboundEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterAllEvents(EventTransportDirection::Outbound,o); }
    EventTransportBulkOperationResult UnregisterAllBidirectionalEvents(const EventTransportUnregistrationOptions& o = {}) { return UnregisterAllEvents(EventTransportDirection::Bidirectional,o); }
    EventTransportBulkOperationResult UnregisterAllInboundEvents(IEventTransport* t,const EventTransportUnregistrationOptions& o = {}) { return UnregisterAllEvents(t,EventTransportDirection::Inbound,o); }
    EventTransportBulkOperationResult UnregisterAllOutboundEvents(IEventTransport* t,const EventTransportUnregistrationOptions& o = {}) { return UnregisterAllEvents(t,EventTransportDirection::Outbound,o); }
    EventTransportBulkOperationResult UnregisterAllBidirectionalEvents(IEventTransport* t,const EventTransportUnregistrationOptions& o = {}) { return UnregisterAllEvents(t,EventTransportDirection::Bidirectional,o); }

    template<typename TEvent>
    EventTransportDirection GetEventTransportDirection() const {
        static_assert(
            EventTransportTypeTraits<TEvent>::Name.size() != 0,
            "Querying a transported Event requires its stable transport type trait."
        );
        constexpr uint64_t typeID = EventTransportTypeID<TEvent>();
        std::lock_guard<std::mutex> lock(_mutex);
        const auto found = _registrations.find(typeID);
        return found == _registrations.end()
            ? EventTransportDirection::None
            : found->second.DefaultDirection;
    }

    template<typename TEvent>
    EventTransportDirection GetEventTransportDirection(
        IEventTransport* transport
    ) const {
        static_assert(
            EventTransportTypeTraits<TEvent>::Name.size() != 0,
            "Querying a transported Event requires its stable transport type trait."
        );
        if (transport == nullptr) {
            return EventTransportDirection::None;
        }
        constexpr uint64_t typeID = EventTransportTypeID<TEvent>();
        std::lock_guard<std::mutex> lock(_mutex);
        const auto found = _registrations.find(typeID);
        return found == _registrations.end()
            ? EventTransportDirection::None
            : found->second.EffectiveDirection(transport);
    }

    Observable::ObserverHandlePtr RegisterObserver(
        IEventTransportManagerObserver* observer
    ) {
        return _observable->RegisterObserver(observer);
    }

    void UnregisterObserver(IEventTransportManagerObserver* observer) {
        _observable->UnregisterObserver(observer);
    }

    void OnEventDispatched(
        IEvent* event,
        EventDispatchMethod method,
        EventPriority priority,
        const EventDispatchContext& context
    ) override {
        if (
            !_initialized ||
            event == nullptr ||
            context.Origin != EventOrigin::Local
        ) {
            return;
        }

        const EventTypeKey eventType = event->__getTypeKey();
        if (eventType == nullptr) {
            return;
        }

        uint64_t typeID = 0;
        uint64_t messageID = 0;
        RuntimeRegistrationPtr runtime;
        std::vector<IEventTransport*> targetTransports;

        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto runtimeType = _runtimeTypes.find(eventType);
            if (runtimeType == _runtimeTypes.end()) {
                return;
            }
            auto registration = _registrations.find(runtimeType->second);
            if (
                registration == _registrations.end() ||
                !registration->second.Runtime
            ) {
                return;
            }

            targetTransports.reserve(_transports.size());
            for (IEventTransport* transport : _transports) {
                if (HasDirection(
                    registration->second.EffectiveDirection(transport),
                    EventTransportDirection::Outbound
                )) {
                    targetTransports.push_back(transport);
                }
            }
            if (targetTransports.empty()) {
                return;
            }

            typeID = runtimeType->second;
            runtime = registration->second.Runtime;
            messageID = _nextMessageID.fetch_add(1);
            for (IEventTransport* transport : targetTransports) {
                event->__ref();
                _outbound.push_back({
                    event,
                    transport,
                    typeID,
                    runtime,
                    method,
                    priority,
                    messageID
                });
            }
        }

        _observable->Notify([&](IEventTransportManagerObserver* observer) {
            observer->OnOutboundEventAccepted(typeID, messageID);
        });

        for (IEventTransport* transport : targetTransports) {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnOutboundEventAcceptedForTransport(
                    typeID, messageID, transport
                );
            });
            NotifyTransaction({
                EventTransportTransactionStage::OutboundAccepted,
                EventTransportDirection::Outbound,
                typeID,
                runtime->TypeName,
                runtime->SchemaVersion,
                messageID,
                transport,
                event,
                nullptr,
                0,
                method,
                priority,
                EventOrigin::Local,
                0,
                false
            });
        }
        Wake();
    }

    void ReceiveEventTransportPacket(
        IEventTransport* transport,
        const uint8_t* data,
        std::size_t size
    ) override {
        if (
            !_initialized ||
            transport == nullptr ||
            data == nullptr ||
            size < sizeof(EventTransportEnvelope)
        ) {
            return;
        }

        EventTransportEnvelope envelope;
        const uint8_t* payload = nullptr;
        if (!ParseEnvelope(data, size, envelope, payload)) {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnInboundPacketRejected(0,0,transport);
            });
            NotifyTransaction({
                EventTransportTransactionStage::InboundRejected,
                EventTransportDirection::Inbound,
                0,{},0,0,transport,nullptr,nullptr,0,
                EventDispatchMethod::Queue,
                EventPriority::Normal,
                EventOrigin::Remote,
                0,false
            });
            return;
        }

        bool accepted = false;
        RuntimeRegistrationPtr runtime;
        std::string_view typeName{};
        uint32_t schemaVersion = envelope.SchemaVersion;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (IsTransportRegisteredLocked(transport)) {
                auto found = _registrations.find(envelope.EventTypeID);
                if (
                    found != _registrations.end() &&
                    found->second.Runtime
                ) {
                    runtime = found->second.Runtime;
                    typeName = runtime->TypeName;
                    schemaVersion = runtime->SchemaVersion;
                    if (HasDirection(
                        found->second.EffectiveDirection(transport),
                        EventTransportDirection::Inbound
                    )) {
                        _inbound.push_back({
                            transport,
                            envelope.EventTypeID,
                            runtime,
                            std::vector<uint8_t>(data, data + size)
                        });
                        accepted = true;
                    }
                }
            }
        }

        if (accepted) {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnInboundPacketAccepted(
                    envelope.EventTypeID,
                    envelope.MessageID,
                    transport
                );
            });
            NotifyTransaction({
                EventTransportTransactionStage::InboundAccepted,
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
                true
            });
            Wake();
        } else {
            _observable->Notify([&](IEventTransportManagerObserver* observer) {
                observer->OnInboundPacketRejected(
                    envelope.EventTypeID,
                    envelope.MessageID,
                    transport
                );
            });
            NotifyTransaction({
                EventTransportTransactionStage::InboundRejected,
                EventTransportDirection::Inbound,
                envelope.EventTypeID,
                typeName,
                envelope.SchemaVersion,
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
        }
    }
};

}
