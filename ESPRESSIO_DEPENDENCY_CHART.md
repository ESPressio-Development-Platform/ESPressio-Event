# ESPressio Dependency Chart — Serializable 0.11.2 Cascade

![ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.svg)

Arrows point from the consuming library to the library it consumes.

- **Required** — the dependency is part of the library's normal/core contract.
- **Opt-in** — the dependency is introduced only when the corresponding integration/header is selected.

## Current cascade generation

```text
Observable    3.0.2
Serializable  0.11.2
Units         0.2.6
Timing        2.2.7
Threads       3.1.6
Event         6.0.2   (this release)
Command       1.0.1   -> planned 1.0.2
Security      0.4.0   -> planned 0.4.1
Persistence   0.3.0   -> planned 0.3.1
Sockets       0.7.1   -> planned 0.7.2
ESP-Now       0.8.1   -> planned 0.8.2
WiFi          0.2.0   unreleased
Serial        0.8.0   -> planned 0.8.1
```

## Event dependencies

```text
Event 6.0.2
    -> Threads >= 3.1.6 < 4.0.0      required
    -> Timing >= 2.2.7 < 3.0.0       required
    -> Observable >= 3.0.2 < 4.0.0   required
    - - -> Serializable >= 0.11.2 < 1.0.0
            opt-in Serializable Events / Event Transport
```

Event remains a mechanism-only library. It does not consume Command, Security, Sockets or ESP-Now merely to host their domain-specific Event bridges.

## Active propagation order

```text
Serializable 0.11.2
    -> Units 0.2.6
    -> Timing 2.2.7
    -> Threads 3.1.6
    -> Event 6.0.2
    -> Command 1.0.2 / Security 0.4.1
    -> Persistence 0.3.1 / Sockets 0.7.2 / ESP-Now 0.8.2
    -> WiFi 0.2.0
    -> Serial 0.8.1
```

## Dependency-direction invariants

```text
Command  - - -> Event
Security - - -> Event
Sockets  - - -> Event
ESP-Now  - - -> Event

Event -> Command   NONE
Event -> Security  NONE
Event -> Sockets   NONE
Event -> ESP-Now   NONE
```

Timing and Threads Event bridges remain in Event because Event already requires Timing and Threads for its core responsibilities. Serializable support remains opt-in.

Serial remains terminal/downstream. Tree remains standalone.
