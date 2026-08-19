#include <cassert>
#include <cstdio>

#include "../src/protocol/crc16.h"
#include "../src/protocol/report.h"
#include "fixtures_usbmon3.h"

namespace {

void test_crc16_matches_all_fixtures() {
    for (const auto& fixture : kFixtures) {
        uint16_t computed = crc16_modbus(fixture.raw.data(), kReportSize - 2);
        uint16_t stored = static_cast<uint16_t>(fixture.raw[62] | (fixture.raw[63] << 8));
        if (computed != stored) {
            std::fprintf(stderr, "frame %d: crc mismatch computed=%04x stored=%04x\n",
                         fixture.frame_number, computed, stored);
        }
        assert(computed == stored);
    }
    std::printf("test_crc16_matches_all_fixtures: OK (%zu frames)\n", kFixtures.size());
}

void test_report_crc_valid_matches_all_fixtures() {
    for (const auto& fixture : kFixtures) {
        assert(report_crc_valid(fixture.raw));
    }
    std::printf("test_report_crc_valid_matches_all_fixtures: OK (%zu frames)\n", kFixtures.size());
}

void test_parse_then_build_round_trips() {
    for (const auto& fixture : kFixtures) {
        Interface2Report report = parse_report(fixture.raw);
        std::array<uint8_t, kReportSize> rebuilt = build_report(report);
        if (rebuilt != fixture.raw) {
            std::fprintf(stderr, "frame %d: round-trip mismatch\n", fixture.frame_number);
        }
        assert(rebuilt == fixture.raw);
    }
    std::printf("test_parse_then_build_round_trips: OK (%zu frames)\n", kFixtures.size());
}

void test_known_header_fields() {
    // frame 1453: the rainbow-gradient command decoded in PROTOCOL.md.
    const Fixture* frame_1453 = nullptr;
    for (const auto& fixture : kFixtures) {
        if (fixture.frame_number == 1453) {
            frame_1453 = &fixture;
            break;
        }
    }
    assert(frame_1453 != nullptr);

    Interface2Report report = parse_report(frame_1453->raw);
    assert(report.length == 0x29);
    assert(report.session == 0x0002);
    assert(report.counter == 0x3e);
    assert(report.marker == 0x10);
    assert(report.subcmd == 0x06);
    assert(report.flags == 0x00);
    // Rainbow stop 0 (position 0%, RGB ff/ff/00 = yellow) at raw offset 17-20,
    // i.e. payload offset 17-8=9 (payload starts right after the 8-byte header).
    assert(report.payload[9] == 0x00);   // position: 0%
    assert(report.payload[10] == 0xff);  // R
    assert(report.payload[11] == 0xff);  // G
    assert(report.payload[12] == 0x00);  // B
    std::printf("test_known_header_fields: OK\n");
}

}  // namespace

int main() {
    test_crc16_matches_all_fixtures();
    test_report_crc_valid_matches_all_fixtures();
    test_parse_then_build_round_trips();
    test_known_header_fields();
    std::printf("All tests passed.\n");
    return 0;
}
