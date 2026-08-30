from pathlib import Path

path = Path('src/ESPressio_EventTransportManager.hpp')
text = path.read_text()


def replace_once(old: str, new: str, label: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise SystemExit(f'{label}: expected 1 exact match, found {count}')
    text = text.replace(old, new, 1)


replace_once(
    '#include <ESPressio_Synchronization.hpp>\n#include <ESPressio_Thread.hpp>',
    '#include <ESPressio_Synchronization.hpp>\n#include <ESPressio_TaskExecutor.hpp>\n#include <ESPressio_Thread.hpp>',
    'task include',
)

replace_once(
    '''#ifndef ESPRESSIO_EVENT_TRANSPORT_MAX_PENDING_OUTBOUND_EVENTS
    #define ESPRESSIO_EVENT_TRANSPORT_MAX_PENDING_OUTBOUND_EVENTS 32
#endif
''',
    '''#ifndef ESPRESSIO_EVENT_TRANSPORT_MAX_PENDING_OUTBOUND_EVENTS
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
''',
    'outbound task configuration macros',
)

replace_once(
    '''    struct InboundWork {
        IEventTransport* Transport = nullptr;
        uint64_t TypeID = 0;
        RuntimeRegistrationPtr Runtime;
        EventTransportPacket Packet;
    };
''',
    '''    struct InboundWork {
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
''',
    'outbound work type',
)

replace_once(
    '''    using TypeIdVector = System::Memory::Vector<
        uint64_t,
        ExternalPreferred
    >;

    mutable System::Synchronization::Mutex _mutex;
''',
    '''    using TypeIdVector = System::Memory::Vector<
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
''',
    'outbound task configuration',
)

replace_once(
    '''    std::atomic<uint64_t> _nextMessageID{1};
    std::atomic<uint64_t> _rejectedInboundCount{0};
    std::atomic<uint64_t> _processedInboundCount{0};
    std::atomic<uint64_t> _processedOutboundCount{0};
    std::atomic<std::size_t> _peakInboundCount{0};
    bool _initialized = false;

    EventTransportManager()
        : Threads::Thread(Threads::ThreadReleasePolicy::ExplicitRelease) {
        SetPriority(ESPRESSIO_EVENT_TRANSPORT_MANAGER_PRIORITY);
        SetCoreID(ESPRESSIO_EVENT_TRANSPORT_MANAGER_CORE_ID);
''',
    '''    std::atomic<uint64_t> _nextMessageID{1};
    std::atomic<uint64_t> _rejectedInboundCount{0};
    std::atomic<uint64_t> _processedInboundCount{0};
    std::atomic<uint64_t> _processedOutboundCount{0};
    std::atomic<uint64_t> _rejectedOutboundWorkCount{0};
    std::atomic<std::size_t> _peakInboundCount{0};
    Task::TaskExecutor<OutboundWork> _outboundExecutor;
    std::atomic<bool> _outboundExecutorInitialized{false};
    std::atomic<bool> _outboundExecutorReady{false};
    std::atomic<bool> _initialized{false};

    EventTransportManager()
        : Threads::Thread(Threads::ThreadReleasePolicy::ExplicitRelease),
          _outboundExecutor(CreateOutboundTaskConfiguration()) {
        SetPriority(ESPRESSIO_EVENT_TRANSPORT_MANAGER_PRIORITY);
        SetCoreID(ESPRESSIO_EVENT_TRANSPORT_MANAGER_CORE_ID);
        SetStartOnInitialize(false);
''',
    'members and constructor',
)

text = text.replace(
    'registerReceiver = _initialized;',
    'registerReceiver = _initialized.load(std::memory_order_acquire);',
)
text = text.replace(
    'unregisterReceiver = _initialized;',
    'unregisterReceiver = _initialized.load(std::memory_order_acquire);',
)

start = text.index('    void ProcessOutboundEvent(\n')
end = text.index('    bool PopInbound(InboundWork& work) {\n', start)
text = text[:start] + r'''    void ReleaseOutboundWork(const OutboundWork& work) noexcept {
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
        TransportVector targets;
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

''' + text[end:]

replace_once(
    '''                    [&](IEvent* event, EventDispatchMethod method, EventPriority priority) {
                        ProcessOutboundEvent(event, method, priority);
                    }
''',
    '''                    [&](IEvent* event, EventDispatchMethod method, EventPriority priority) {
                        SubmitOutboundEvent(event, method, priority);
                    }
''',
    'outbound mailbox handoff',
)

lifecycle_start = text.index('    Threads::ThreadInitializationStatus Initialize() override {\n')
lifecycle_end = text.index('    bool RegisterTransport(IEventTransport* transport) {\n', lifecycle_start)
text = text[:lifecycle_start] + r'''    Threads::ThreadInitializationStatus Initialize() override {
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

''' + text[lifecycle_end:]

text = text.replace(
    '        if (!_initialized || transport == nullptr || !packet) return;',
    '        if (!IsInitialized() || transport == nullptr || !packet) return;',
)

replace_once(
    '''    uint64_t GetProcessedOutboundEventCount() const noexcept {
        return _processedOutboundCount.load(std::memory_order_relaxed);
    }
''',
    '''    uint64_t GetProcessedOutboundEventCount() const noexcept {
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
''',
    'outbound diagnostics',
)

suspicious = [
    line for line in text.splitlines()
    if '_initialized' in line
    and '.load' not in line
    and '.store' not in line
    and '.exchange' not in line
    and 'std::atomic<bool>' not in line
]
if suspicious:
    raise SystemExit('unconverted _initialized uses: ' + repr(suspicious))

path.write_text(text)
