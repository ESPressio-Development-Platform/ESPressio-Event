from pathlib import Path

path = Path('src/ESPressio_EventTransportManager.hpp')
text = path.read_text()

# Single global unregistration: capture the subscription identity/state before registration erasure.
old = '''        EventTransportDirection before = EventTransportDirection::None;\n        EventTransportDirection after = EventTransportDirection::None;\n        bool remains = false;\n        {\n            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);\n            auto found = _registrations.find(typeID);'''
new = '''        EventTransportDirection before = EventTransportDirection::None;\n        EventTransportDirection after = EventTransportDirection::None;\n        EventTypeKey subscriptionType = nullptr;\n        bool outboundRequired = false;\n        bool remains = false;\n        {\n            std::lock_guard<System::Synchronization::Mutex> lock(_mutex);\n            auto found = _registrations.find(typeID);'''
assert text.count(old) == 2
text = text.replace(old, new, 2)

old = '''            remains = found->second.HasAnyDirection();\n            RemoveRegistrationIfUnusedLocked(typeID);\n        }\n\n        RefreshOutboundSubscription(typeID);'''
new = '''            remains = found->second.HasAnyDirection();\n            if (found->second.Runtime) {\n                subscriptionType = found->second.Runtime->EventType;\n                outboundRequired = found->second.HasOutboundDirection();\n            }\n            RemoveRegistrationIfUnusedLocked(typeID);\n        }\n\n        if (subscriptionType != nullptr) {\n            SetOutboundSubscription(subscriptionType, outboundRequired);\n        }'''
assert text.count(old) == 2
text = text.replace(old, new, 2)

# Bulk global / per-transport unregistration: same rule while iterating type IDs.
old = '''            EventTransportDirection before = EventTransportDirection::None;\n            EventTransportDirection after = EventTransportDirection::None;\n            bool changed = false;'''
new = '''            EventTransportDirection before = EventTransportDirection::None;\n            EventTransportDirection after = EventTransportDirection::None;\n            EventTypeKey subscriptionType = nullptr;\n            bool outboundRequired = false;\n            bool changed = false;'''
assert text.count(old) == 2
text = text.replace(old, new, 2)

old = '''                RemoveRegistrationIfUnusedLocked(typeID);\n                changed = true;\n            }\n            RefreshOutboundSubscription(typeID);'''
new = '''                if (found->second.Runtime) {\n                    subscriptionType = found->second.Runtime->EventType;\n                    outboundRequired = found->second.HasOutboundDirection();\n                }\n                RemoveRegistrationIfUnusedLocked(typeID);\n                changed = true;\n            }\n            if (subscriptionType != nullptr) {\n                SetOutboundSubscription(subscriptionType, outboundRequired);\n            }'''
assert text.count(old) == 2
text = text.replace(old, new, 2)

path.write_text(text)

checker = Path('tools/check_event_transport_subscription_cleanup.py')
checker.write_text(r'''from pathlib import Path

source = Path("src/ESPressio_EventTransportManager.hpp").read_text()

# Registration refresh remains valid on registration/addition paths and transport removal,
# but final event-registration removal must not depend on looking up a type after erasure.
for marker in [
    "EventTransportUnregistrationResult UnregisterEvent(\n        EventTransportDirection direction",
    "EventTransportUnregistrationResult UnregisterEvent(\n        IEventTransport* transport",
    "EventTransportBulkOperationResult UnregisterAllEvents(\n        EventTransportDirection direction",
    "EventTransportBulkOperationResult UnregisterAllEvents(\n        IEventTransport* transport",
]:
    start = source.index(marker)
    end = source.find("\n    }", start)
    block = source[start:end]
    assert "subscriptionType = found->second.Runtime->EventType" in block
    assert "outboundRequired = found->second.HasOutboundDirection()" in block
    assert block.index("subscriptionType = found->second.Runtime->EventType") < block.index("RemoveRegistrationIfUnusedLocked(typeID)")
    assert "SetOutboundSubscription(subscriptionType, outboundRequired)" in block
    assert "RefreshOutboundSubscription(typeID)" not in block

print("EventTransportManager removal paths preserve EventTypeKey through subscription cleanup")
''')

workflow = Path('.github/workflows/tests.yml')
w = workflow.read_text()
needle = '      - name: Verify Event dependency boundaries\n        run: python tools/check_dependency_boundaries.py\n'
replacement = needle + '      - name: Verify Event transport subscription cleanup\n        run: python tools/check_event_transport_subscription_cleanup.py\n'
assert needle in w and 'check_event_transport_subscription_cleanup.py' not in w
w = w.replace(needle, replacement, 1)

# Compile confirmation: instantiate removal surfaces on a real Serializable Event type.
needle = '''              (void)transportManager.GetOutboundExecutionStatistics();\n              (void)transportManager.GetRejectedOutboundWorkCount();\n'''
replacement = '''              (void)transportManager.GetOutboundExecutionStatistics();\n              (void)transportManager.GetRejectedOutboundWorkCount();\n              (void)transportManager.RegisterOutboundEvent<ESPressio::Event::ThreadRegisteredEventSerializable>();\n              (void)transportManager.UnregisterOutboundEvent<ESPressio::Event::ThreadRegisteredEventSerializable>();\n              (void)transportManager.UnregisterAllOutboundEvents();\n'''
# If the concrete alias differs, CI will tell us and we will use the actual exported type.
assert needle in w
w = w.replace(needle, replacement, 1)
workflow.write_text(w)
