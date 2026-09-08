#include <Arduino.h>

#include <ESPressio_Event.hpp>
#include <ESPressio_EventThread.hpp>

#include <ESPressio_ThreadEventBridges.hpp>
#include <ESPressio_ThreadEvents.hpp>

using namespace ESPressio;

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: sizeof(EventThreadBase) + sizeof(EventListener) [0 bytes dynamic allocation]
 * Requires Stack/Heap Preallocation
 * Members:
 * - _registeredHandle (Event::EventListenerHandlePtr): sizeof(Event::EventListenerHandlePtr) [0 bytes dynamic allocation]
 * - _cleanupHandle (Event::EventListenerHandlePtr): sizeof(Event::EventListenerHandlePtr) [0 bytes dynamic allocation]
 * - _terminationHandle (Event::EventListenerHandlePtr): sizeof(Event::EventListenerHandlePtr) [0 bytes dynamic allocation]
 * Total Memory: sizeof(EventThreadBase) + sizeof(EventListener) + sizeof(Event::EventListenerHandlePtr) + sizeof(Event::EventListenerHandlePtr) + sizeof(Event::EventListenerHandlePtr) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class InfrastructureEventThread final :
    public Event::EventThread {

private:
    Event::EventListenerHandlePtr
        _registeredHandle;

    Event::EventListenerHandlePtr
        _cleanupHandle;

    Event::EventListenerHandlePtr
        _terminationHandle;

public:
    InfrastructureEventThread() :
        Event::EventThread(
            Threads::ThreadReleasePolicy::ExplicitRelease
        ) {

        _registeredHandle =
            RegisterListener<
                Event::ThreadRegisteredEvent
            >(
                [](
                    Event::ThreadRegisteredEvent* event,
                    Event::EventDispatchMethod,
                    Event::EventPriority,
                    const Event::EventDispatchContext&
                ) {
                    Serial.printf(
                        "registered id=%u core=%d\n",
                        event->Snapshot.ThreadID,
                        event->Snapshot.CoreID
                    );
                }
            );

        _cleanupHandle =
            RegisterListener<
                Event::ThreadCleanupCompletedEvent
            >(
                [](
                    Event::ThreadCleanupCompletedEvent* event,
                    Event::EventDispatchMethod,
                    Event::EventPriority,
                    const Event::EventDispatchContext&
                ) {
                    Serial.printf(
                        "cleanup deleted=%u\n",
                        static_cast<unsigned int>(
                            event->Result.ThreadsDeleted
                        )
                    );
                }
            );

        _terminationHandle =
            RegisterListener<
                Event::ThreadTerminationDispatchCompletedEvent
            >(
                [](
                    Event::ThreadTerminationDispatchCompletedEvent* event,
                    Event::EventDispatchMethod,
                    Event::EventPriority,
                    const Event::EventDispatchContext&
                ) {
                    Serial.printf(
                        "termination dispatched id=%u\n",
                        event->Snapshot.ThreadID
                    );
                }
            );
    }
};

InfrastructureEventThread infrastructureEvents;

void setup() {
    Serial.begin(115200);

    Event::ThreadManagerEventBridge::
        GetInstance().
        Initialize();

    Event::ThreadTerminationDispatcherEventBridge::
        GetInstance().
        Initialize();

    Threads::ThreadManager::
        GetInstance()->
        Initialize();
}

void loop() {
    delay(1000);
}
