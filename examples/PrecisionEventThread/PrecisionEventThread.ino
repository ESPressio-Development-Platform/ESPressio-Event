#include <ESPressio_Event.hpp>
#include <ESPressio_PrecisionEventThread.hpp>
#include <ESPressio_ThreadManager.hpp>

using namespace ESPressio;

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members:
 * - _setpoint (int): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: sizeof(IEvent) + sizeof(std::atomic_flag) + 4 bytes vptr + 4 bytes known members [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class SetpointEvent final :
    public Event::TypedEvent<SetpointEvent> {

    private:
        const int _setpoint;

    public:
        explicit SetpointEvent(
            int setpoint
        ) :
            _setpoint(setpoint) {
        }

        int GetSetpoint() const {
            return _setpoint;
        }
};


/**
 * ESPressio Memory Audit
 * Inherited Memory Total: sizeof(Threads::PrecisionThread<TTime, TRepresentationTraits>) + sizeof(EventReceiver) + sizeof(IEventThreadBase) + sizeof(EventListener) + sizeof(IEventThread) + 2 bytes known members + sizeof(std::mutex) + sizeof(Observable::ObserverHandlePtr) + 4 bytes vptr [0 bytes dynamic allocation]
 * Requires Stack/Heap Preallocation
 * Members:
 * - _setpoint (int): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: sizeof(Threads::PrecisionThread<TTime, TRepresentationTraits>) + sizeof(EventReceiver) + sizeof(IEventThreadBase) + sizeof(EventListener) + sizeof(IEventThread) + 2 bytes known members + sizeof(std::mutex) + sizeof(Observable::ObserverHandlePtr) + 4 bytes vptr + 4 bytes known members [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class ControlThread final :
    public Event::PrecisionEventThread<> {

    private:
        int _setpoint = 0;

    protected:
        void OnIteration(
            IterationTime delta,
            IterationTime startTime,
            Threads::
                SkippedIterationCount
                    skippedIterations
        ) override {
            (void)delta;
            (void)startTime;

            Serial.printf(
                "control setpoint=%d skipped=%llu\n",
                _setpoint,
                static_cast<
                    unsigned long long
                >(
                    skippedIterations
                )
            );
        }

    public:
        ControlThread() :
            Event::PrecisionEventThread<>(
                Threads::ThreadReleasePolicy::ExplicitRelease
            ) {
        }

        void ApplySetpoint(
            SetpointEvent* event
        ) {
            _setpoint =
                event->GetSetpoint();
        }
};


ControlThread controlThread;

Event::EventListenerHandlePtr
    setpointListener;


void setup() {
    Serial.begin(115200);

    controlThread.
        SetIterationPeriod(
            Units::
                MilliSeconds<
                    uint64_t
                >(10)
        );

    controlThread.
        SetEventProcessOrder(
            Event::
                PrecisionEventProcessOrder::
                    EventsBeforeIteration
        );

    controlThread.
        SetEventArrivalPolicy(
            Event::
                PrecisionEventArrivalPolicy::
                    ProcessImmediately
        );

    setpointListener =
        controlThread.
            RegisterListener<
                SetpointEvent
            >(
                [](
                    SetpointEvent* event,
                    Event::
                        EventDispatchMethod,
                    Event::
                        EventPriority,
                    const Event::
                        EventDispatchContext&
                ) {
                    controlThread.
                        ApplySetpoint(
                            event
                        );
                }
            );

    Threads::ThreadManager::
        GetInstance()->
        Initialize();
}


void loop() {
    static int
        nextSetpoint = 1;

    (
        new SetpointEvent(
            nextSetpoint++
        )
    )->Queue();

    delay(1000);
}
