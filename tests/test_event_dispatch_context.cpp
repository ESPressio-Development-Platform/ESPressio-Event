#include <cassert>
#include <type_traits>

#include "../src/ESPressio_EventTypes.hpp"

int main() {
    using namespace ESPressio::Event;

    static_assert(
        std::is_same<
            decltype(EventDispatchContext{} == EventDispatchContext{}),
            bool
        >::value,
        "EventDispatchContext must be equality comparable."
    );

    constexpr EventDispatchContext localA{};
    constexpr EventDispatchContext localB{};
    static_assert(localA == localB);

    constexpr EventDispatchContext remoteA{EventOrigin::Remote};
    constexpr EventDispatchContext remoteB{EventOrigin::Remote};
    constexpr EventDispatchContext local{EventOrigin::Local};

    static_assert(remoteA == remoteB);
    static_assert(remoteA != local);

    static_assert(
        sizeof(EventDispatchContext) == sizeof(EventOrigin),
        "Event dispatch context must not grow transport-local route or hop metadata."
    );

    assert(remoteA == remoteB);
    assert(!(remoteA != remoteB));
    return 0;
}
