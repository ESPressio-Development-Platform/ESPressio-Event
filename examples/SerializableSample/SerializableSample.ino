#include <ESPressio_Event.hpp>
namespace E = ESPressio::Event;
using namespace ESPressio;
struct Sample final : E::SerializableEvent<Sample> {
    static constexpr E::EventTypeId TypeId{0x1002};
    static constexpr std::string_view CanonicalName="example.sample";
    static constexpr std::size_t MaximumLiveInstances=4, MaximumPendingInstances=1;
    std::int32_t Value=0;
    ESPRESSIO_SERIALIZABLE_TYPE(Sample)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("value",Value))
};
static_assert(Serializable::IsBoundedSerializable<Sample>);
void setup() { (void)Sample::GetPrimitiveTypeDescriptor(); }
void loop() {}
