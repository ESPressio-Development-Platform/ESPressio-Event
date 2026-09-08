#include <Arduino.h>
#include <ESPressio_Event.hpp>
#include <ESPressio_EventTransport.hpp>
#include <ESPressio_Serializable.hpp>

using namespace ESPressio;

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - Value (int32_t): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 28 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class OperatorCommandEvent :
    public Event::Event<>,
    public Serializable::SerializableBase<OperatorCommandEvent> {
public:
    int32_t Value = 0;

    ESPRESSIO_SERIALIZABLE_TYPE(OperatorCommandEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("value", Value)
    )
};

ESPRESSIO_EVENT_TRANSPORT_TYPE(
    OperatorCommandEvent,
    0xF10D000000000003ULL,
    "flowduino.example.operator-command.v1"
)

void setup() {
    ::Serial.begin(115200);

    auto& manager = Event::EventTransportManager::GetInstance();
    manager.RegisterBidirectionalEvent<OperatorCommandEvent>();

    // Runtime discovery requires no compile-time knowledge of the Event type.
    for (const auto& descriptor : manager.GetRegisteredSerializableEvents()) {
        ::Serial.print("Registered: ");
        ::Serial.println(descriptor.TypeName.c_str());
        ::Serial.print("Schema: ");
        ::Serial.println(descriptor.SchemaVersion);
        for (const auto& property : descriptor.Properties) {
            ::Serial.print("  ");
            ::Serial.print(property.Name.c_str());
            ::Serial.print(" : ");
            ::Serial.println(property.Type.c_str());
        }
    }

    // A future Serial/REST/WebSocket console can obtain this node from JSON.
    // Event itself remains independent of ArduinoJson.
    Serializable::SerializationNode payload(
        Serializable::SerializationNodeType::Object
    );
    payload.Set("__schemaVersion", Serializable::Detail::ToNode(uint32_t{1}));
    payload.Set("value", Serializable::Detail::ToNode(int32_t{42}));

    auto result = manager.CreateSerializableEvent(
        "flowduino.example.operator-command.v1",
        payload
    );

    if (!result) {
        ::Serial.println("Runtime Event construction failed.");
        for (const auto& issue : result.Deserialization.Issues()) {
            ::Serial.print(issue.Path.c_str());
            ::Serial.print(": ");
            ::Serial.println(issue.Message.c_str());
        }
        return;
    }

    Event::EventTransportManager::DispatchSerializableEvent(
        std::move(result.Event),
        Event::EventDispatchMethod::Queue,
        Event::EventPriority::Normal
    );

    ::Serial.println("Runtime Event dispatched.");
}

void loop() {}
