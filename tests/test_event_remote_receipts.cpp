#include <ESPressio_EventRemoteReceipts.hpp>
#include <array>
#include <cassert>
using namespace ESPressio;
namespace E=ESPressio::Event;
int main() {
    std::array<E::RemoteAdmissionReceipt,2> storage;
    E::EventRemoteReceiptTable table;
    assert(table.Initialize(storage.data(),storage.size()));
    System::DeviceIdentifier::Storage device{};device[0]=1;
    E::EventOccurrenceKey key{E::EventTypeId{1},{System::DeviceIdentifier{device},System::RuntimeIncarnationId{1}},E::ConceptualMessageId{1}};
    auto key2=key; key2.MessageId=E::ConceptualMessageId{2};
    auto key3=key; key3.Origin.Incarnation=System::RuntimeIncarnationId{2};
    {
        auto first=table.TryReserve(key,10); assert(first);
        assert(table.TryReserve(key,1000).Status==E::EventRemoteAdmissionStatus::TemporarilyUnavailable);
        auto second=table.TryReserve(key2,10); assert(second);
        assert(!table.TryReserve(key3,10));
        first.Commit(20,100);
        assert(table.TryReserve(key,119).Status==E::EventRemoteAdmissionStatus::AlreadyAdmitted);
        assert(!table.TryReserve(key3,119));
        // The uncommitted second reservation is released at scope exit.
    }
    auto next=table.TryReserve(key3,119); assert(next); next.Commit(119,500);
    assert(table.TryReserve(key,119).Status==E::EventRemoteAdmissionStatus::AlreadyAdmitted);
    auto expired=table.TryReserve(key,120); assert(expired); expired.Commit(UINT64_MAX-5,10);
    // Unrepresentable expiry stays occupied, failing closed rather than evicting early.
    assert(table.TryReserve(key,UINT64_MAX).Status==E::EventRemoteAdmissionStatus::AlreadyAdmitted);
    E::EventOccurrenceKey invalid;
    assert(table.TryReserve(invalid,0).Status==E::EventRemoteAdmissionStatus::Invalid);
}
