#pragma once

#include "ESPressio_ThreadManagerEventBridge_Serializable.hpp"

#if __has_include(<ESPressio_ThreadGarbageCollector.hpp>)
    #include "ESPressio_ThreadGarbageCollectorEventBridge_Serializable.hpp"
#endif

#include "ESPressio_ThreadTerminationDispatcherEventBridge_Serializable.hpp"
