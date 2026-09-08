#include <ESPressio_Event.hpp>
#include <ESPressio_SystemClockEventBridge.hpp>
#include <ESPressio_TimingEvents.hpp>

using namespace ESPressio;

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: sizeof(EventThreadBase) + sizeof(EventListener) [0 bytes dynamic allocation]
 * Requires Stack/Heap Preallocation
 * Members:
 * - _synchronizedListener (Event::EventListenerHandlePtr): sizeof(Event::EventListenerHandlePtr) [0 bytes dynamic allocation]
 * Total Memory: sizeof(EventThreadBase) + sizeof(EventListener) + sizeof(Event::EventListenerHandlePtr) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class TimingEventThread final : public Event::EventThread {
private:
    Event::EventListenerHandlePtr _synchronizedListener;

public:
    TimingEventThread() {
        _synchronizedListener =
            RegisterListener<Event::SystemClockSynchronizedEvent>(
                [](Event::SystemClockSynchronizedEvent* event,
                   Event::EventDispatchMethod,
                   Event::EventPriority,
                   const Event::EventDispatchContext&) {
                    Serial.printf(
                        "Clock synchronized: before=%llu after=%llu immediateDiff=%lld ns\n",
                        static_cast<unsigned long long>(event->ClockBeforeNanoseconds),
                        static_cast<unsigned long long>(event->ClockAfterNanoseconds),
                        static_cast<long long>(event->ImmediateDifferenceNanoseconds)
                    );
                }
            );
    }
};

TimingEventThread timingEvents;

void setup() {
    Serial.begin(115200);

    Event::SystemClockEventBridge::
        GetInstance().
        Initialize();

    Threads::ThreadManager::
        GetInstance()->
        Initialize();
}

void loop() {
    delay(1000);
}
