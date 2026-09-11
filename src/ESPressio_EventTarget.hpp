#pragma once
#include <atomic>
#include "ESPressio_EventLease.hpp"
namespace ESPressio::Event {
enum class EventTargetClass : std::uint8_t { LocalThread, ExternalAdapter };
enum class EventTargetAdmission : std::uint8_t { Accepted, CapacityUnavailable, Quiesced };
/// Consumer-owned intrusive node. Only the Type lane changes Owed; linkage is
/// protected by the Type topology gate. Admission never invokes application code.
struct EventTargetNode final {
    void* Owner = nullptr;
    EventTargetAdmission (*TryAdmit)(void*, const EventLease&) noexcept = nullptr;
    bool (*Validate)(void*) noexcept = nullptr;
    void (*Quiesce)(void*) noexcept = nullptr;
    EventTargetClass Class = EventTargetClass::LocalThread;
    EventTargetNode* Next = nullptr;
    std::atomic<void*> Runtime{nullptr};
    void (*CapacityChanged)(void*) noexcept = nullptr;
    bool Owed = false;
    void NotifyCapacityChanged() const noexcept {
        if (auto* runtime=Runtime.load(std::memory_order_acquire)) CapacityChanged(runtime);
    }
};
}
