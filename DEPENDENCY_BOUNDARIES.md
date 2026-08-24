# ESPressio Event Dependency Boundaries

## Responsibility

ESPressio Event owns the generic asynchronous Event mechanism: Event lifecycle,
dispatch, listeners, Event threads, Serializable Event support, and transport-
neutral Event Transport abstractions.

Event must not acquire a dependency solely to host concrete domain Events or an
Observer-to-Event bridge for another ESPressio library.

## Direct dependencies retained by Event 6.0.3

### ESPressio Threads — required

EventManager and EventThread use ESPressio Threads directly for asynchronous
execution. This is a genuine core dependency. Event 6.0.3 requires Threads
`>=3.1.7 <4.0.0`.

### ESPressio Timing — required

Event lifecycle timestamps use ESPressio SystemClock and TimeTraits directly.
This is a genuine core dependency. Timing/SystemClock Event bridges remain in
Event because Event is already the legitimate downstream consumer of Timing.
Event 6.0.3 requires Timing `>=2.2.8 <3.0.0`.

### ESPressio Observable — required

Event exposes synchronous lifecycle observation through Observable-backed
manager and transport-manager Observer contracts. This is a genuine direct
dependency. Event 6.0.3 retains Observable `>=3.0.2 <4.0.0`.

### ESPressio Serializable — optional

Serializable Event definitions and Event Transport payload serialization use
ESPressio Serializable only when those features are selected. Local ordinary
Events do not require Serializable. Event 6.0.3 CI validates this optional
surface against released Serializable 0.11.3.

## Domain integrations that must not be owned by Event

The following domains are not required by Event's generic mechanism and must
own their own optional Event integrations:

- ESPressio ESP-Now;
- ESPressio Sockets;
- ESPressio Command;
- ESPressio Security.

Their domain Event types and Observer-to-Event bridges belong in the respective
domain library, which may optionally consume Event.

## Dependency-direction rule

A concrete integration belongs at the lowest-order consumer that can own it
without creating a reciprocal dependency. Event may retain concrete bridges for
Timing and Threads because Event already consumes those libraries for its core
responsibility. Event must not depend on ESP-Now, Sockets, Command, or Security
merely to expose their lifecycle as Events.

## Release validation

For Event 6.0.3 and subsequent releases, the source tree must be audited to
confirm that no source, CI, package metadata, current-version documentation, or
examples introduce Event -> ESP-Now, Event -> Sockets, Event -> Command, or
Event -> Security dependencies.
