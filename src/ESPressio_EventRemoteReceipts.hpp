#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <utility>
#include <ESPressio_Synchronization.hpp>
#include "ESPressio_EventTypes.hpp"
#include <ESPressio_PrimitiveAdmission.hpp>
namespace ESPressio::Event {
enum class EventRemoteAdmissionStatus : std::uint8_t {
    Admitted, AlreadyAdmitted, TemporarilyUnavailable, Invalid, UnknownType,
    UnsupportedProtocol, SchemaOrDecodeFailure, Rejected, ResourceUnavailable
};
struct EventRemoteAdmissionResult final {
    EventRemoteAdmissionStatus Status=EventRemoteAdmissionStatus::Invalid;
    bool EstablishesDestinationAdmission() const noexcept {
        return Status==EventRemoteAdmissionStatus::Admitted || Status==EventRemoteAdmissionStatus::AlreadyAdmitted;
    }
};
/// Fixed M1 translation. Link/route success cannot enter this family result path.
constexpr Primitive::PrimitiveAdmissionDisposition ToPrimitiveAdmissionDisposition(EventRemoteAdmissionStatus status) noexcept {
    using D=Primitive::PrimitiveAdmissionDisposition;
    switch(status) {
        case EventRemoteAdmissionStatus::Admitted:return D::Accepted;
        case EventRemoteAdmissionStatus::AlreadyAdmitted:return D::AlreadyAccepted;
        case EventRemoteAdmissionStatus::TemporarilyUnavailable:return D::TemporarilyUnavailable;
        case EventRemoteAdmissionStatus::ResourceUnavailable:return D::ResourceUnavailable;
        case EventRemoteAdmissionStatus::UnknownType:
        case EventRemoteAdmissionStatus::UnsupportedProtocol:return D::Unsupported;
        case EventRemoteAdmissionStatus::Rejected:return D::Rejected;
        case EventRemoteAdmissionStatus::Invalid:
        case EventRemoteAdmissionStatus::SchemaOrDecodeFailure:return D::Malformed;
    }
    return D::Malformed;
}
class EventRemoteReceiptTable;
/// Caller-provided fixed record storage. Its internals are exclusively owned by
/// the family Runtime while initialized. InProgress records are never expired.
class RemoteAdmissionReceipt final {
    friend class EventRemoteReceiptTable;
    enum class State : std::uint8_t { Free, InProgress, Admitted };
    State _state=State::Free;
    EventOccurrenceKey _key{};
    std::uint64_t _expires=0,_generation=0;
    bool _saturatedExpiry=false;
};
/// Fixed table with provisional ownership. An abandoned decode automatically
/// frees only its exact reservation; committed records survive duplicate retries.
class EventRemoteReceiptTable final {
    System::Synchronization::Mutex _mutex;
    RemoteAdmissionReceipt* _entries=nullptr;
    std::size_t _capacity=0;
    void Release(std::size_t index,std::uint64_t generation) noexcept {
        std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
        auto& entry=_entries[index];
        if(entry._state==RemoteAdmissionReceipt::State::InProgress && entry._generation==generation)
            entry._state=RemoteAdmissionReceipt::State::Free;
    }
    void Commit(std::size_t index,std::uint64_t generation,std::uint64_t now,std::uint64_t residence) noexcept {
        std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
        auto& entry=_entries[index];
        if(entry._state!=RemoteAdmissionReceipt::State::InProgress || entry._generation!=generation) std::terminate();
        entry._saturatedExpiry=residence>UINT64_MAX-now;
        entry._expires=entry._saturatedExpiry ? UINT64_MAX : now+residence;
        entry._state=RemoteAdmissionReceipt::State::Admitted;
    }
public:
    class Reservation final {
        friend class EventRemoteReceiptTable;
        EventRemoteReceiptTable* _table=nullptr;
        std::size_t _index=0;
        std::uint64_t _generation=0;
        Reservation(EventRemoteReceiptTable* table,std::size_t index,std::uint64_t generation) noexcept
            : _table(table),_index(index),_generation(generation) {}
    public:
        EventRemoteAdmissionStatus Status=EventRemoteAdmissionStatus::TemporarilyUnavailable;
        Reservation() noexcept = default;
        Reservation(const Reservation&)=delete;
        Reservation& operator=(const Reservation&)=delete;
        Reservation(Reservation&& b) noexcept : _table(std::exchange(b._table,nullptr)),_index(b._index),
            _generation(b._generation),Status(b.Status) {}
        Reservation& operator=(Reservation&&)=delete;
        ~Reservation() { if(_table) _table->Release(_index,_generation); }
        /// True denotes an uncommitted slot, not destination admission evidence.
        explicit operator bool() const noexcept { return _table!=nullptr; }
        void Commit(std::uint64_t now,std::uint64_t residence) noexcept {
            if(!_table) std::terminate();
            _table->Commit(_index,_generation,now,residence); _table=nullptr;
            Status=EventRemoteAdmissionStatus::Admitted;
        }
    };
    /// Bootstrap only. No record initialization/reset is performed over live history.
    bool Initialize(RemoteAdmissionReceipt* records,std::size_t capacity) noexcept {
        std::lock_guard<System::Synchronization::Mutex> lock(_mutex);
        if((_entries && (_entries!=records || _capacity!=capacity)) || (capacity && !records)) return false;
        for(std::size_t i=0;i<capacity;++i) if(records[i]._state!=RemoteAdmissionReceipt::State::Free) return false;
        _entries=records; _capacity=capacity; return true;
    }
    Reservation TryReserve(const EventOccurrenceKey& key,std::uint64_t now) noexcept {
        Reservation rejected;
        if(!key.IsValid()) { rejected.Status=EventRemoteAdmissionStatus::Invalid; return rejected; }
        std::unique_lock<System::Synchronization::Mutex> lock(_mutex,std::try_to_lock);
        if(!lock.owns_lock()) return rejected;
        std::size_t free=_capacity;
        for(std::size_t i=0;i<_capacity;++i) {
            auto& entry=_entries[i];
            if(entry._state==RemoteAdmissionReceipt::State::Admitted && !entry._saturatedExpiry && now>=entry._expires)
                entry._state=RemoteAdmissionReceipt::State::Free;
            if(entry._state!=RemoteAdmissionReceipt::State::Free && entry._key==key) {
                rejected.Status=entry._state==RemoteAdmissionReceipt::State::Admitted ?
                    EventRemoteAdmissionStatus::AlreadyAdmitted : EventRemoteAdmissionStatus::TemporarilyUnavailable;
                return rejected;
            }
            if(entry._state==RemoteAdmissionReceipt::State::Free && entry._generation!=UINT64_MAX && free==_capacity) free=i;
        }
        if(free==_capacity) return rejected; // Never evict a live/in-progress receipt.
        auto& entry=_entries[free];
        entry._key=key; ++entry._generation; entry._state=RemoteAdmissionReceipt::State::InProgress;
        return Reservation(this,free,entry._generation);
    }
    std::size_t Capacity() const noexcept { return _capacity; }
};
}
