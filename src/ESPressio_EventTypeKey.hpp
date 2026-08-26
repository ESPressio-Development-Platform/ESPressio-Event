#pragma once

#include <type_traits>

namespace ESPressio {
namespace Event {

using EventTypeKey = const void*;

template<typename T>
EventTypeKey EventTypeKeyOf() noexcept {
    using WithoutReference = typename std::remove_reference<T>::type;
    using Normalized = typename std::remove_cv<WithoutReference>::type;
    static const unsigned char token = 0;
    return static_cast<EventTypeKey>(&token);
}

} // namespace Event
} // namespace ESPressio
