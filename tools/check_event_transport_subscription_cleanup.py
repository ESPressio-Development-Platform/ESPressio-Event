from pathlib import Path

source = Path("src/ESPressio_EventTransportManager.hpp").read_text()

for marker in [
    "EventTransportUnregistrationResult UnregisterEvent(\n        EventTransportDirection direction",
    "EventTransportUnregistrationResult UnregisterEvent(\n        IEventTransport* transport",
    "EventTransportBulkOperationResult UnregisterAllEvents(\n        EventTransportDirection direction",
    "EventTransportBulkOperationResult UnregisterAllEvents(\n        IEventTransport* transport",
]:
    start = source.index(marker)
    end = source.find("\n    }", start)
    block = source[start:end]
    capture = block.index("subscriptionType = found->second.Runtime->EventType")
    removal = block.index("RemoveRegistrationIfUnusedLocked(typeID)")
    assert capture < removal
    assert "outboundRequired = found->second.HasOutboundDirection()" in block
    assert "SetOutboundSubscription(subscriptionType, outboundRequired)" in block
    assert "RefreshOutboundSubscription(typeID)" not in block

print("EventTransportManager removal paths preserve EventTypeKey through subscription cleanup")
