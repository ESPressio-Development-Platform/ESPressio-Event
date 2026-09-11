#include <ESPressio_Event.hpp>
#include <ESPressio_EventInstancePool.hpp>
#include <ESPressio_EventMessageSequence.hpp>
#include <cassert>
#include <type_traits>
using namespace ESPressio;
struct Local final : Event::Event<Local> {
    static constexpr ESPressio::Event::EventTypeId TypeId{1};
    static constexpr std::size_t MaximumLiveInstances=1,MaximumPendingInstances=0;
    int Value;
    explicit Local(int value) : Value(value) {}
};
int main() {
    static_assert(Event::Detail::ValidateEventType<Local>());
    static_assert(!std::is_same_v<Event::EventTypeId,Primitive::CommandTypeId>);
    Event::EventInstancePool<Local,1> pool;
    auto slot=pool.TryReserve();
    auto occurrence=slot.Construct({Local::TypeId,Event::ConceptualMessageId{11},{22,Timing::TimeReliability::Acquiring}},3);
    assert(occurrence.Get<Local>().GetConceptualMessageId().Value()==11);
    assert(occurrence.Get<Local>().GetOriginDispatchTime().Nanoseconds==22);
    Local detached=occurrence.Get<Local>();
    assert(!detached.TryGetOccurrenceFacts());
    Event::Detail::EventMessageSequence<3> sequence;
    Event::ConceptualMessageId id;
    for (std::uint64_t n=1;n<=3;++n) { assert(sequence.TryIssue(id)); assert(id.Value()==n); }
    assert(sequence.IsExhausted() && !sequence.TryIssue(id) && id.Value()==3);
}
