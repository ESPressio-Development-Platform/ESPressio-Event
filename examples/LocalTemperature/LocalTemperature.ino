#include <ESPressio_Event.hpp>
#include <ESPressio_ThreadWith.hpp>
#include <ESPressio_Precision.hpp>
namespace E = ESPressio::Event;
using namespace ESPressio;
struct Temperature final : E::Event<Temperature> {
    static constexpr E::EventTypeId TypeId{0x1001};
    static constexpr std::string_view CanonicalName="example.temperature";
    static constexpr std::size_t MaximumLiveInstances=4, MaximumPendingInstances=2;
    int MilliCelsius;
    explicit Temperature(int value) noexcept : MilliCelsius(value) {}
};
using Inbox=E::ThreadCapability<E::SharedPendingCapacity<0>,Temperature>;
class Display final : public Threads::ThreadWith<Inbox,Threads::Precision<8>> {
    int _last=0;
    void Receive(const Temperature& sample) { _last=sample.MilliCelsius; }
    void OnInitialization() override {
        if(!GetCapability<E::ThreadCapabilityTag>().Listen<Temperature>(*this,&Display::Receive))
            throw 1;
    }
public:
    ~Display() override { (void)Shutdown(); }
    int Last() const noexcept { return _last; } // Read after Shutdown, or in this Thread.
};
// Install platform execution/synchronization providers before setup.
Primitive::TypeDirectory<1> directory;
E::Runtime events;
Display display;
void setup() {
    if(directory.Register<Temperature>()!=Primitive::TypeDirectoryRegistrationStatus::Success) return;
    if(directory.Initialize()!=Primitive::TypeDirectoryInitializationStatus::Success) return;
    if(events.Initialize(directory.View())!=E::EventRuntimeStatus::Success) return;
    if(display.Initialize()!=Threads::ThreadStatus::Success) return;
    if(events.Start()!=E::EventRuntimeStatus::Success) return;
    if(display.Start()!=Threads::ThreadStatus::Success) return;
    (void)Temperature::TryDispatch(23125);
}
void loop() {}
