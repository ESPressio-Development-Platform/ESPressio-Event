#pragma once

#include <functional>
#include <mutex>

namespace ESPressio {
    namespace Threads {

/**
 * ESPressio Memory Audit
 * Members:
 * - _value (T): sizeof(T) [0 bytes dynamic allocation]
 * - _mutex (std::mutex): sizeof(std::mutex) [0 bytes dynamic allocation]
 * Total Memory: sizeof(T) + sizeof(std::mutex) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
template <class T>
        class ReadWriteMutex {
            private:
                T _value;
                mutable std::mutex _mutex;

            public:
                explicit ReadWriteMutex(T value) : _value(value) {}

                T Get() {
                    std::lock_guard<std::mutex> lock(_mutex);
                    return _value;
                }

                void Set(T value) {
                    std::lock_guard<std::mutex> lock(_mutex);
                    _value = value;
                }

                void WithWriteLock(std::function<void(T&)> callback) {
                    std::lock_guard<std::mutex> lock(_mutex);
                    callback(_value);
                }
        };

    }
}
