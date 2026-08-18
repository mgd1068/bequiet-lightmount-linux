#include "report.h"

#include "crc16.h"

Interface2Report parse_report(const std::array<uint8_t, kReportSize>& raw) {
    Interface2Report report;
    report.length = static_cast<uint16_t>(raw[0] | (raw[1] << 8));
    report.seq = static_cast<uint16_t>(raw[4] | (raw[5] << 8));
    report.subcmd = raw[6];
    report.flags = raw[7];
    for (size_t i = 0; i < kPayloadSize; ++i) {
        report.payload[i] = raw[8 + i];
    }
    return report;
}

std::array<uint8_t, kReportSize> build_report(const Interface2Report& report) {
    std::array<uint8_t, kReportSize> raw{};
    raw[0] = static_cast<uint8_t>(report.length & 0xFF);
    raw[1] = static_cast<uint8_t>(report.length >> 8);
    raw[2] = 0x02;  // session/connection id, LE: reproduces the known usbmon3 fixtures
                    // (their session used 0x0002) - NOT a universal protocol constant,
                    // see report.h and PROTOCOL.md.
    raw[3] = 0x00;
    raw[4] = static_cast<uint8_t>(report.seq & 0xFF);
    raw[5] = static_cast<uint8_t>(report.seq >> 8);
    raw[6] = report.subcmd;
    raw[7] = report.flags;
    for (size_t i = 0; i < kPayloadSize; ++i) {
        raw[8 + i] = report.payload[i];
    }
    uint16_t crc = crc16_modbus(raw.data(), kReportSize - 2);
    raw[62] = static_cast<uint8_t>(crc & 0xFF);
    raw[63] = static_cast<uint8_t>(crc >> 8);
    return raw;
}

bool report_crc_valid(const std::array<uint8_t, kReportSize>& raw) {
    uint16_t computed = crc16_modbus(raw.data(), kReportSize - 2);
    uint16_t stored = static_cast<uint16_t>(raw[62] | (raw[63] << 8));
    return computed == stored;
}
