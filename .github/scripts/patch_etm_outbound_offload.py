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
    '''/// EventTransportManager behaves like an ordinary Event receiver: EventManager fans one Event reference into this
/// subscriber mailbox for each transport-enabled Serializable Event type. The Event is serialized once, then released
/// by EventReceiver after this subscriber callback returns. Physical transports receive shared immutable ownership of
/// the resulting ExternalPreferred packet and may move that handle into their own asynchronous execution contexts.
/// Inbound physical packets enter as owned buffers and are deserialized before being submitted to EventManager.
''',
    '''/// EventTransportManager behaves like an ordinary Event receiver: EventManager fans one Event reference into this
/// subscriber mailbox for each transport-enabled Serializable Event type. Its coordinator Thread transfers outbound
/// ownership into a bounded TaskExecutor and returns immediately; serialization, transport observation, and fanout execute
/// off the coordinator stack. The executor serializes once into ExternalPreferred storage, releases Event ownership after
/// Event-aware serialized-stage notifications, and then fans shared immutable packet ownership to physical transports.
/// Inbound physical packets enter as owned buffers and are deserialized before being submitted to EventManager.
''',
    'class lifecycle documentation',
)

replace_once(
    '''    Task::TaskExecutor<OutboundWork> _outboundExecutor;
    std::atomic<bool> _outboundExecutorInitialized{false};
''',
    '''    Task::TaskExecutor<OutboundWork> _outboundExecutor;
    // Single-worker scratch storage retains target-vector capacity in PSRAM
    // rather than allocating a fresh vector for every outbound Event.
    TransportVector _outboundTargets;
    std::atomic<bool> _outboundExecutorInitialized{false};
''',
    'reusable outbound target storage',
)

replace_once(
    '''        RuntimeRegistrationPtr runtime;
        TransportVector targets;
        {
''',
    '''        RuntimeRegistrationPtr runtime;
        TransportVector& targets = _outboundTargets;
        targets.clear();
        {
''',
    'reuse target storage',
)

replace_once(
    '''        _processedOutboundCount.fetch_add(1, std::memory_order_relaxed);
    }

    void SubmitOutboundEvent(
''',
    '''        targets.clear();
        _processedOutboundCount.fetch_add(1, std::memory_order_relaxed);
    }

    void SubmitOutboundEvent(
''',
    'clear reusable targets after fanout',
)

path.write_text(text)
