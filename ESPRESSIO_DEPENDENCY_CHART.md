# ESPressio Dependency Chart — Current Released Generation

![ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.svg)

## Released generation

```text
Observable    3.0.2
Serializable  0.11.3
Units         0.2.7
Timing        2.2.8
Threads       3.1.7
Event         6.0.3
Command       1.0.3
Security      0.4.2
Persistence   0.3.2
Sockets       0.7.3
ESP-Now       0.8.3
WiFi          0.2.0
Serial        0.8.1
```

## Event dependencies

```text
Event 6.0.3
    -> Threads >= 3.1.7 < 4.0.0
    -> Timing >= 2.2.8 < 3.0.0
    -> Observable >= 3.0.2 < 4.0.0
    - - -> Serializable >= 0.11.3 < 1.0.0
            opt-in Serializable Events / Event Transport
```

Event remains a mechanism-only library. Domain-specific Event types and bridges are owned by their respective downstream libraries.

## Downstream integration direction

```text
Command  - - -> Event >= 6.0.3 < 7.0.0
Security - - -> Event >= 6.0.3 < 7.0.0
Sockets  - - -> Event >= 6.0.3 < 7.0.0
ESP-Now  - - -> Event >= 6.0.3 < 7.0.0
WiFi     - - -> Event >= 6.0.3 < 7.0.0

Event -> Command   NONE
Event -> Security  NONE
Event -> Sockets   NONE
Event -> ESP-Now   NONE
Event -> WiFi      NONE
```

## Completed cascade

```text
Serializable 0.11.3
    -> Units 0.2.7
    -> Timing 2.2.8
    -> Threads 3.1.7
    -> Event 6.0.3
    -> Command 1.0.3 / Security 0.4.2
    -> Persistence 0.3.2 / Sockets 0.7.3 / ESP-Now 0.8.3
    -> WiFi 0.2.0
    -> Serial 0.8.1
```

Timing and Threads Event bridges remain in Event because Event already requires Timing and Threads for its core responsibilities. Serializable support remains opt-in. Serial remains terminal/downstream; ESPressio Tree remains standalone.
