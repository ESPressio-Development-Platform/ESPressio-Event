# Getting Started

Define a routable Event with compiler-backed type identity:

```cpp
#include <ESPressio_Event.hpp>

class TemperatureChangedEvent final :
    public ESPressio::Event::TypedEvent<TemperatureChangedEvent> {
public:
    const float Previous;
    const float Current;

    TemperatureChangedEvent(float previous, float current) :
        Previous(previous), Current(current) {}
};
```

Dispatch it asynchronously:

```cpp
(new TemperatureChangedEvent(21.0f, 21.5f))->Queue();
```

Register a typed listener on an Event-aware Thread:

```cpp
auto handle = eventThread.RegisterListener<TemperatureChangedEvent>(
    [](TemperatureChangedEvent* event,
       Event::EventDispatchMethod,
       Event::EventPriority) {
        // consume event
    }
);
```

Keep the listener handle alive for as long as the listener should remain registered.

## Ownership

After dispatch, application code should treat the Event as immutable and should not retain ownership of the raw pointer. Event infrastructure owns its dispatched lifetime.

## Next steps

- [Typed Events and Identity](Typed-Events-and-Identity)
- [Event Listeners](Event-Listeners)
- [Bounded Queues and Diagnostics](Bounded-Queues-and-Diagnostics)
- [Event Transport](Event-Transport)