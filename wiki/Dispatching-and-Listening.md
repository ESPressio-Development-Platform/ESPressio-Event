# Dispatching and Listening

ESPressio Event provides asynchronous fan-out between producers and typed listeners.

## Dispatch methods

Dispatch FIFO-style with `Queue()`:

```cpp
(new TemperatureChangedEvent(21.0f, 21.5f))->Queue();
```

Dispatch LIFO-style with `Stack()`:

```cpp
(new TemperatureChangedEvent(21.0f, 21.5f))->Stack();
```

Both operations are asynchronous. Independent listeners do not have a meaningful globally guaranteed execution order.

If an operation requires synchronous, ordered completion, direct sequencing or ESPressio Observable is usually the more accurate abstraction.

## Registering a listener

Listener registration is typed:

```cpp
Event::EventListenerHandlePtr handle =
    eventThread.RegisterListener<TemperatureChangedEvent>(
        [](TemperatureChangedEvent* event,
           Event::EventDispatchMethod method,
           Event::EventPriority priority) {
            // Consume event.
        }
    );
```

Keep the returned handle alive for as long as the listener should remain registered. Destroying or unregistering the handle removes the registration according to the listener lifecycle contract.

## Re-entrant registration

Listener registration and unregistration are safe during dispatch. The current implementation uses stable listener records and deferred compaction rather than copying the complete listener collection for every dispatch.

The callback itself is invoked in place; it is not copied merely to release an internal lock before user code executes.

This is an important memory-efficiency property and should be preserved by extensions.

## Event priority

Dispatch carries `EventPriority` alongside the Event and dispatch method. Consumers can inspect the supplied priority where their processing policy requires it.

Priority does not turn asynchronous fan-out into a globally ordered synchronous pipeline.

## Bounded pending work

Receiver queues are bounded by default. A slow or blocked consumer therefore cannot cause pending Event storage to grow without limit.

The default maximum is configurable through:

```cpp
ESPRESSIO_EVENT_DEFAULT_MAX_PENDING_EVENT_COUNT
```

Queue diagnostics expose current and peak pending counts plus rejected/dropped Event counts. Treat those diagnostics as part of operational health monitoring when Event rates can approach receiver capacity.