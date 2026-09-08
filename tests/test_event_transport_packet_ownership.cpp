#include <cassert>
#include <cstdint>

#include "ESPressio_EventTransportTypes.hpp"

using namespace ESPressio::Event;

/**
 * ESPressio Memory Audit
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 0 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
struct ExplicitType {};
/**
 * ESPressio Memory Audit
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 0 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
struct UnregisteredType {};

ESPRESSIO_EVENT_TRANSPORT_TYPE(
    ExplicitType,
    0xF10D00000000F001ULL,
    "tests.explicit-type.v1"
)

int main() {
    static_assert(EventFamilyId == ESPressio::Primitive::FamilyIds::Event);
    static_assert(EventTransportTypeID<ExplicitType>() == 0xF10D00000000F001ULL);
    static_assert(EventTransportTypeID<UnregisteredType>() == 0);
    static_assert(EventTransportTypeTraits<ExplicitType>::Name == "tests.explicit-type.v1");

    EventTransportBuffer bytes;
    bytes.push_back(0x11);
    bytes.push_back(0x22);
    bytes.push_back(0x33);

    EventTransportPacket original(std::move(bytes), EventMessageId(42));
    assert(original);
    assert(original.Size() == 3);
    assert(original.MessageID() == EventMessageId(42));

    const auto originalBacking = original.Buffer();
    assert(originalBacking);
    const uint8_t* originalData = original.Data();

    EventTransportPacket firstCopy = original;
    EventTransportPacket secondCopy = firstCopy;

    assert(firstCopy.Buffer() == originalBacking);
    assert(secondCopy.Buffer() == originalBacking);
    assert(firstCopy.Data() == originalData);
    assert(secondCopy.Data() == originalData);
    assert(firstCopy.Size() == original.Size());
    assert(secondCopy.MessageID() == original.MessageID());

    original = {};
    assert(!original);
    assert(firstCopy);
    assert(secondCopy);
    assert(firstCopy.Data() == originalData);
    assert(firstCopy.Data()[0] == 0x11);
    assert(firstCopy.Data()[1] == 0x22);
    assert(firstCopy.Data()[2] == 0x33);

    firstCopy = {};
    assert(secondCopy);
    assert(secondCopy.Data() == originalData);

    secondCopy = {};
    assert(!secondCopy);
}
