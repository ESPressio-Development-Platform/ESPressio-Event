from pathlib import Path

path = Path('src/ESPressio_EventTransportManager.hpp')
text = path.read_text()

# Capture Event identity and the resulting outbound requirement while the registration
# still exists. EventManager subscription mutation remains outside the ETM registry lock.
old = '''        EventTransportDirection before = EventTransportDirection::None;\n        EventTransportDirection after = EventTransportDirection::None;\n        bool remains = false;\n        {\n            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);\n            auto found = _registrations.find(typeID);'''
new = '''        EventTransportDirection before = EventTransportDirection::None;\n        EventTransportDirection after = EventTransportDirection::None;\n        EventTypeKey subscriptionType = nullptr;\n        bool outboundRequired = false;\n        bool remains = false;\n        {\n            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);\n            auto found = _registrations.find(typeID);'''
assert text.count(old) == 2
text = text.replace(old, new, 2)

old = '''            remains = found->second.HasAnyDirection();\n            RemoveRegistrationIfUnusedLocked(typeID);\n        }\n\n        RefreshOutboundSubscription(typeID);'''
new = '''            remains = found->second.HasAnyDirection();\n            if (found->second.Runtime) {\n                subscriptionType = found->second.Runtime->EventType;\n                outboundRequired = found->second.HasOutboundDirection();\n            }\n            RemoveRegistrationIfUnusedLocked(typeID);\n        }\n\n        if (subscriptionType != nullptr) {\n            SetOutboundSubscription(subscriptionType, outboundRequired);\n        }'''
assert text.count(old) == 2
text = text.replace(old, new, 2)

old = '''            EventTransportDirection before = EventTransportDirection::None;\n            EventTransportDirection after = EventTransportDirection::None;\n            bool changed = false;'''
new = '''            EventTransportDirection before = EventTransportDirection::None;\n            EventTransportDirection after = EventTransportDirection::None;\n            EventTypeKey subscriptionType = nullptr;\n            bool outboundRequired = false;\n            bool changed = false;'''
assert text.count(old) == 2
text = text.replace(old, new, 2)

old = '''                RemoveRegistrationIfUnusedLocked(typeID);\n                changed = true;\n            }\n            RefreshOutboundSubscription(typeID);'''
new = '''                if (found->second.Runtime) {\n                    subscriptionType = found->second.Runtime->EventType;\n                    outboundRequired = found->second.HasOutboundDirection();\n                }\n                RemoveRegistrationIfUnusedLocked(typeID);\n                changed = true;\n            }\n            if (subscriptionType != nullptr) {\n                SetOutboundSubscription(subscriptionType, outboundRequired);\n            }'''
assert text.count(old) == 2
text = text.replace(old, new, 2)

path.write_text(text)

Path('tools/check_event_transport_subscription_cleanup.py').write_text(r'''from pathlib import Path

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
''')
