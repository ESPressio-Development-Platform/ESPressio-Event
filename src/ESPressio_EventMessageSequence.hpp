#pragma once
#include <cstdint>
#include <limits>
#include "ESPressio_EventTypes.hpp"
namespace ESPressio::Event::Detail {
/// Owned by one Type runtime and accessed only under its admission gate. There
/// is no reset/reseed API; aborted constructions may burn an issued identifier.
template<std::uint64_t Maximum = std::numeric_limits<std::uint64_t>::max()>
class EventMessageSequence final {
    static_assert(Maximum > 0);
    std::uint64_t _highWater = 0;
public:
    EventMessageSequence() noexcept = default;
    EventMessageSequence(const EventMessageSequence&) = delete;
    EventMessageSequence& operator=(const EventMessageSequence&) = delete;
    bool TryIssue(ConceptualMessageId& id) noexcept {
        if (_highWater == Maximum) return false;
        id = ConceptualMessageId{++_highWater};
        return true;
    }
    bool IsExhausted() const noexcept { return _highWater == Maximum; }
    ConceptualMessageId HighWater() const noexcept { return ConceptualMessageId{_highWater}; }
};
}
