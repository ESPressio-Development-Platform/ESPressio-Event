#pragma once

#include <type_traits>

namespace ESPressio {
namespace Event {

/// <summary>Process-local, RTTI-free identity token used to route concrete Event types.</summary>
using EventTypeKey = const void*;

namespace Detail {

template<typename T>
EventTypeKey EventTypeKeyStorage() noexcept {
    static const unsigned char token = 0;
    return static_cast<EventTypeKey>(&token);
}

} // namespace Detail

/// <summary>Returns the stable process-local routing key for an Event type.</summary>
/// <typeparam name="T">Event type whose identity is required; cv/ref qualifiers are ignored.</typeparam>
/// <returns>A stable token unique to the normalized type within the current process.</returns>
template<typename T>
EventTypeKey EventTypeKeyOf() noexcept {
    using WithoutReference = typename std::remove_reference<T>::type;
    using Normalized = typename std::remove_cv<WithoutReference>::type;
    return Detail::EventTypeKeyStorage<Normalized>();
}

} // namespace Event
} // namespace ESPressio
