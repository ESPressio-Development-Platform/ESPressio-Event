# ESPressio Dependency Chart — Current Released Generation

![ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.svg)

## Released generation

```text
Observable
Serializable
Units
Timing
Threads
Event
Command
Security
Persistence
Sockets
ESP-Now
WiFi
Serial
```

## Event dependencies

```text
Event
    -> Threads main
    -> Timing main
    -> Observable main
    - - -> Serializable main
            opt-in Serializable Events / Event Transport
```

Event remains a mechanism-only library. Domain-specific Event types and bridges are owned by their respective downstream libraries.

## Downstream integration direction

```text
Command  - - -> Event main
Security - - -> Event main
Sockets  - - -> Event main
ESP-Now  - - -> Event main
WiFi     - - -> Event main

Event -> Command   NONE
Event -> Security  NONE
Event -> Sockets   NONE
Event -> ESP-Now   NONE
Event -> WiFi      NONE
```

## Completed cascade

```text
Serializable
    -> Units
    -> Timing
    -> Threads
    -> Event
    -> Command / Security
    -> Persistence / Sockets / ESP-Now
    -> WiFi
    -> Serial
```

Timing and Threads Event bridges remain in Event because Event already requires Timing and Threads for its core responsibilities. Serializable support remains opt-in. Serial remains terminal/downstream; ESPressio Tree remains standalone.
