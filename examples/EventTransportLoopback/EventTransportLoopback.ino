#include <Arduino.h>

#include <ESPressio_Event.hpp>
#include <ESPressio_EventListener.hpp>
#include <ESPressio_EventTransport.hpp>
#include <ESPressio_Serializable.hpp>

using namespace ESPressio;

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members:
 * - Counter (int32_t): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr + 4 bytes known members [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class DistributedCounterEvent :
    public Event::Event<>,
    public Serializable::SerializableBase<DistributedCounterEvent> {
public:
    int32_t Counter = 0;

    ESPRESSIO_SERIALIZABLE_TYPE(DistributedCounterEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("counter", Counter)
    )
};

ESPRESSIO_EVENT_TRANSPORT_TYPE(
    DistributedCounterEvent,
    0xF10D000000000001ULL,
    "flowduino.example.distributed-counter.v1"
)

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members:
 * - _receiver (Event::IEventTransportReceiver*): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 8 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class LoopbackEventTransport final : public Event::IEventTransport {
private:
    Event::IEventTransportReceiver* _receiver = nullptr;

public:
    bool Send(Event::EventTransportPacket packet) override {
        if (_receiver == nullptr || !packet) return false;

        // A real transport may move this ownership-bearing packet into an
        // asynchronous worker. Loopback transfers the same immutable backing
        // directly into Event's inbound path without copying serialized bytes.
        _receiver->ReceiveEventTransportPacket(this, std::move(packet));
        return true;
    }

    void SetReceiver(Event::IEventTransportReceiver* receiver) override {
        _receiver = receiver;
    }
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes known bases + 5 bytes known members + sizeof(ListenerStorage) + sizeof(System::Synchronization::RecursiveMutex) [0 bytes dynamic allocation]
 * Members:
 * - _handle (Event::EventListenerHandlePtr): sizeof(Event::EventListenerHandlePtr) [0 bytes dynamic allocation]
 * Total Memory: 4 bytes known bases + 5 bytes known members + sizeof(ListenerStorage) + sizeof(System::Synchronization::RecursiveMutex) + sizeof(Event::EventListenerHandlePtr) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class DemoListener final : public Event::EventListener {
private:
    Event::EventListenerHandlePtr _handle;

public:
    DemoListener() {
        _handle = RegisterListener<DistributedCounterEvent>(
            [](
                DistributedCounterEvent* event,
                Event::EventDispatchMethod,
                Event::EventPriority,
                const Event::EventDispatchContext& context
            ) {
                // Origin belongs to this dispatch, not to the Event instance.
                Serial.printf(
                    "Counter=%ld origin=%s\n",
                    static_cast<long>(event->Counter),
                    context.Origin == Event::EventOrigin::Remote ? "remote" : "local"
                );
            }
        );
    }
};

LoopbackEventTransport loopback;
DemoListener listener;

void setup() {
    Serial.begin(115200);

    auto* eventManager = Event::EventManager::GetInstance();
    auto& transports = Event::EventTransportManager::GetInstance();

    transports.RegisterTransport(&loopback);
    transports.RegisterBidirectionalEvents<DistributedCounterEvent>();

    // Construction, resource initialization, and execution are deliberately
    // separate lifecycle phases for both managers.
    eventManager->Initialize();
    transports.Initialize();
    eventManager->Start();
    transports.Start();

    auto* event = new DistributedCounterEvent();
    event->Counter = 42;
    event->Queue();
}

void loop() {
    delay(1000);
}
