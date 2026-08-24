# ESPressio Dependency Chart — Serializable 0.11.3 Cascade

![ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.svg)

Arrows point from the consuming library to the library it consumes.

- **Required** — the dependency is part of the library's normal/core contract.
- **Opt-in** — the dependency is introduced only when the corresponding integration/header is selected.

## Current cascade generation

```text
Observable    3.0.2
Serializable  0.11.3
Units         0.2.7
Timing        2.2.8
Threads       3.1.7
Event         6.0.3   (this release)
Command       1.0.2   -> next patch required
Security      0.4.1   -> next patch required
Persistence   0.3.1   -> next patch required
Sockets       0.7.2   -> next patch required
ESP-Now       0.8.2   -> next patch required
WiFi          0.2.0   unreleased / requires released cascade repoint
Serial        0.8.0   -> downstream terminal cascade pending
```

## Event dependencies

```text
Event 6.0.3
    -> Threads >= 3.1.7 < 4.0.0      required
    -> Timing >= 2.2.8 < 3.0.0       required
    -> Observable >= 3.0.2 < 4.0.0   required
    - - -> Serializable >= 0.11.3 < 1.0.0
            opt-in Serializable Events / Event Transport
```

Event remains a mechanism-only library. It does not consume Command, Security, Sockets or ESP-Now merely to host their domain-specific Event bridges.

## Active propagation order

```text
Serializable 0.11.3
    -> Units 0.2.7
    -> Timing 2.2.8
    -> Threads 3.1.7
    -> Event 6.0.3
    -> Command / Security patch releases
    -> Persistence / Sockets / ESP-Now patch releases
    -> WiFi 0.2.0
    -> Serial
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
