#pragma once

#include "ESPressio_ThreadManagerEventBridge.hpp"

#if __has_include(<ESPressio_ThreadGarbageCollector.hpp>)
    #include "ESPressio_ThreadGarbageCollectorEventBridge.hpp"
#endif

#include "ESPressio_ThreadTerminationDispatcherEventBridge.hpp"
