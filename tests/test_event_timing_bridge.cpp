#include <ESPressio_SystemClockEventBridge.hpp>
#include <HostRuntime.hpp>
#include <cassert>
#include <cstdlib>
#include <new>
using namespace ESPressio;
static std::atomic<bool> denyHeap{false};
void* operator new(std::size_t n) {if(denyHeap) std::abort();if(auto* p=std::malloc(n?n:1)) return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
int main() {
    HostRuntime platform;
    Primitive::TypeDirectory<1> directory;
    assert(directory.Register<Event::SystemClockTimeChangedEvent>()==Primitive::TypeDirectoryRegistrationStatus::Success);
    assert(directory.Initialize()==Primitive::TypeDirectoryInitializationStatus::Success);
    Event::Runtime runtime;
    assert(runtime.Initialize(directory.View())==Event::EventRuntimeStatus::Success);
    assert(runtime.Start()==Event::EventRuntimeStatus::Success);
    Event::SystemClockEventBridge bridge;
    assert(bridge.Initialize());
    auto& clock=Timing::SystemClock<>::GetInstance();
    auto& type=Event::EventTypeRuntime<Event::SystemClockTimeChangedEvent>::Get();
    assert(type.MessageHighWater().Value()==0);
    denyHeap=true;
    for(unsigned i=0;i<1000;++i) { (void)clock.GetTime();(void)clock.GetSynchronizationStatus();(void)clock.CaptureQualifiedTime(); }
    assert(type.MessageHighWater().Value()==0 && bridge.UnavailableOccurrences()==0);
    const auto time=Timing::TimeTraits<Timing::DefaultClockTime>::FromNanoseconds<std::uint64_t>(9000000000ULL,1);
    assert(clock.TrySetTime(time)==Timing::ClockConfigurationStatus::Success);
    assert(type.MessageHighWater().Value()==1);
    clock.SealContinuity();
    assert(clock.TrySetTime(time)==Timing::ClockConfigurationStatus::ContinuitySealed);
    assert(type.MessageHighWater().Value()==1);
    bridge.Shutdown();
    assert(runtime.Shutdown()==Event::EventRuntimeStatus::Success);
    denyHeap=false;
}
