# Serializable Events

Serialization is optional in ESPressio Event. Local Event dispatch does not require ESPressio Serializable.

## Declaring a Serializable Event

When an Event must be represented outside the local process, derive from `SerializableEvent<TDerived>` and declare the required Serializable schema using the Serializable library's normal facilities.

```cpp
class OperatorCommandEvent final :
    public ESPressio::Event::SerializableEvent<OperatorCommandEvent> {
public:
    // Serializable members/schema.
};
```

A Serializable Event automatically participates in Event's compiler-backed typed identity model.

## Runtime discovery

Event Transport can expose descriptors for registered Serializable Event types:

```cpp
auto& manager =
    ESPressio::Event::EventTransportManager::GetInstance();

for (const auto& descriptor :
     manager.GetRegisteredSerializableEvents()) {
    // descriptor.TypeID
    // descriptor.TypeName
    // descriptor.SchemaVersion
    // descriptor.Properties
    // descriptor.CanConstruct
}
```

Descriptors are snapshots. Callers do not receive mutable references into Event Transport's private registration state.

## Wire compatibility

The RTTI-free Event identity work does not redefine the Serializable Event wire contract. Event Transport continues to use its Event transport envelope and Serializable payload representation.

Treat wire type IDs, schema versions and payload representation as compatibility contracts. Compiler-backed `EventTypeKey` is the local runtime-routing identity and should not be confused with a transport wire identifier.

## Dependency discipline

Keep local-only Event types independent of Serializable unless serialization is genuinely required. This avoids pulling schema/format infrastructure into applications whose Events never cross a persistence or transport boundary.