#include <ESPressio_EventWire.hpp>
#include <cassert>
#include <array>
using namespace ESPressio;
namespace E=ESPressio::Event;
int main() {
    E::EventWireHeader header;
    header.Key.TypeId=E::EventTypeId{0x0807060504030201ULL};
    header.Key.MessageId=E::ConceptualMessageId{0x1817161514131211ULL};
    System::DeviceIdentifier::Storage device{};
    for(unsigned i=0;i<16;++i) device[i]=static_cast<std::uint8_t>(0x20+i);
    header.Key.Origin={System::DeviceIdentifier{device},System::RuntimeIncarnationId{0x34333231}};
    header.OriginDispatchTime={0x4847464544434241ULL,Timing::TimeReliability::Holdover};
    header.PayloadLength=3;
    std::array<std::uint8_t,56> wire{};
    assert(E::EncodeEventWireHeader(header,wire.data(),wire.size()));
    const std::array<std::uint8_t,53> expected{
        3,0,1,0,1,2,3,4,5,6,7,8,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
        0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
        0x31,0x32,0x33,0x34,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,3,3,0,0,0};
    for(std::size_t i=0;i<expected.size();++i) assert(wire[i]==expected[i]);
    E::EventWireHeader decoded;
    assert(E::DecodeEventWireHeader(wire.data(),wire.size(),decoded));
    assert(decoded.Key==header.Key && decoded.OriginDispatchTime.Nanoseconds==header.OriginDispatchTime.Nanoseconds);
    for(std::size_t n=0;n<wire.size();++n) assert(!E::DecodeEventWireHeader(wire.data(),n,decoded));
    for(unsigned reliability=4;reliability<256;++reliability) {
        wire[48]=static_cast<std::uint8_t>(reliability);
        assert(!E::DecodeEventWireHeader(wire.data(),wire.size(),decoded));
        assert(decoded.OriginDispatchTime.Reliability==Timing::TimeReliability::Holdover);
    }
    wire[48]=3;wire[2]=2;
    assert(E::DecodeEventWireHeader(wire.data(),wire.size(),decoded).Status==E::EventWireStatus::UnsupportedProtocol);
    wire[2]=1;wire[49]=4;
    assert(E::DecodeEventWireHeader(wire.data(),wire.size(),decoded).Status==E::EventWireStatus::InvalidLength);
    wire[49]=3;
    for(unsigned i=20;i<36;++i) wire[i]=0;
    assert(!E::DecodeEventWireHeader(wire.data(),wire.size(),decoded));
}
