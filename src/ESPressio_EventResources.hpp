#pragma once
#include <cstddef>
#include <cstdint>
namespace ESPressio::Event {
/// Component figures are contained in ResidentBytes, not additional allocations.
struct EventTypeResourceProfile final {
    std::size_t ResidentBytes=0,ResidentAlignment=1,PoolBytes=0,SlotBytes=0,PoolCapacity=0;
    std::size_t TargetNodeBytes=0;
    std::uint32_t LaneStackBytes=0;
    std::size_t ExecutionContexts=1,Signals=3;
    bool PlatformControlBytesKnown=false;
};
struct EventCapabilityResourceProfile final {
    std::size_t ResidentBytes=0,ResidentAlignment=1,PendingSlots=0,PendingPointerBytes=0;
    std::size_t PrivateCounterBytes=0,SharedCounterBytes=0,TargetNodeBytes=0,ListenerBindingBytes=0;
    std::size_t AdditionalExecutionContexts=0,AdditionalWorkSignals=0;
};
struct EventRuntimeResourceProfile final {
    std::size_t CoordinatorBytes=0,CoordinatorAlignment=1,TypeCount=0,TypeResidentBytes=0;
    std::size_t ReceiptCapacity=0,ReceiptBytes=0,ReceiptAlignment=1;
    std::size_t ExecutionContexts=0,Signals=0;
    std::uint32_t StackBytesPerType=0;
    bool PlatformControlBytesKnown=false;
};
}
