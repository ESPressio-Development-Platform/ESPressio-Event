# Timing and Infrastructure Bridges

Event provides asynchronous Event representations for selected upstream lifecycle/observer APIs where the dependency direction permits it.

## Timing System Clock bridge

Timing is upstream of Event, so Event can adapt Timing System Clock observations into Events without introducing a reciprocal Timing dependency.

```cpp
#include <ESPressio_SystemClockEventBridge.hpp>
```

When those Events also need to cross Event Transport, use the Serializable bridge variant:

```cpp
#include <ESPressio_SystemClockEventBridge_Serializable.hpp>
```

## Threads infrastructure bridges

Threads is also upstream of Event. Event therefore supplies bridges for the Threads infrastructure that exists in the current Threads contract:

```cpp
#include <ESPressio_ThreadEventBridges.hpp>
```

Serializable variants are available where transport is required:

```cpp
#include <ESPressio_ThreadEventBridges_Serializable.hpp>
```

The current bridge set covers Thread lifecycle/manager cleanup and termination-dispatch observations.

The retired dedicated Thread Garbage Collector subsystem is not emulated. There are no compatibility `ThreadGarbageCollection*Event` types in the 1.0.0 model.

## Dependency direction

Concrete Event representations for downstream libraries belong with those libraries:

```text
Command  - - -> Event
Security - - -> Event
Sockets  - - -> Event
ESP-Now  - - -> Event
```

Event must not acquire reverse dependencies merely to host another library's domain Events.

## When to add a bridge

Add an Event bridge when:

1. the observed library is already upstream of Event;
2. an asynchronous Event representation has a clear use case;
3. the bridge can preserve the source library's semantics without making Event own that domain.

Otherwise, place the integration in the downstream/domain-owning library.