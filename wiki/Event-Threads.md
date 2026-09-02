# Event Threads

Event-aware Threads combine ESPressio Threads lifecycle management with asynchronous Event reception.

## `EventThread`

Derive from `EventThread` when a long-lived worker should receive Events:

```cpp
class WorkerThread final : public ESPressio::Event::EventThread {
public:
    WorkerThread() :
        EventThread(
            ESPressio::Threads::ThreadReleasePolicy::ExplicitRelease
        ) {}
};
```

The 1.0.0 architecture uses the explicit Threads ownership policy. Historical boolean `freeOnTerminate` construction is not part of this contract.

Use `ThreadReleasePolicy::ReleaseOnTerminate` only for an object whose ownership/lifetime is intentionally delegated to the Thread lifecycle.

## `PrecisionEventThread`

`PrecisionEventThread` combines deterministic periodic work with Event reception.

```cpp
class ControlThread final :
    public ESPressio::Event::PrecisionEventThread<> {
protected:
    void OnIteration(
        IterationTime,
        IterationTime,
        ESPressio::Threads::SkippedIterationCount
    ) override {
        // Periodic control work.
    }

public:
    ControlThread() :
        PrecisionEventThread<>(
            ESPressio::Threads::ThreadReleasePolicy::ExplicitRelease
        ) {}
};
```

Applications can select how pending Events relate to iteration boundaries, including whether they are processed before or after periodic work.

## Choosing the abstraction

Use an `EventThread` when Event consumption is the worker's primary responsibility.

Use a `PrecisionEventThread` when deterministic periodic execution remains primary but asynchronous Event input must also be incorporated.

Use a normal ESPressio Thread when no Event inbox is required.

## Lifecycle ownership

Event does not redefine Thread lifecycle semantics. Initialization, termination, cleanup, release policy, affinity and stack/resource behaviour remain owned by ESPressio Threads and its Task/System provider stack.

Extensions to Event-aware Threads should preserve that dependency direction rather than duplicating native task ownership inside Event.