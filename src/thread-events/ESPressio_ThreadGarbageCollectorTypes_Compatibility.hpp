#pragma once

#include <cstddef>

#include <ESPressio_ThreadManagerTypes.hpp>

namespace ESPressio {
namespace Threads {

    // Compatibility value types for Event schemas when the coordinated
    // Threads branch no longer exposes the legacy garbage-collector worker.
    // These definitions intentionally restore no runtime GC infrastructure.
    enum class ThreadGarbageCollectionExecutionMode {
        AsynchronousWorker,
        SynchronousFallback
    };

    struct ThreadGarbageCollectionResult {
        ThreadGarbageCollectionExecutionMode ExecutionMode =
            ThreadGarbageCollectionExecutionMode::AsynchronousWorker;

        bool InfrastructureAvailable = false;
        bool RequestQueued = false;
        bool Completed = false;
        bool Failed = false;

        ThreadManagerCleanupResult ManagerResult;
    };

}
}
