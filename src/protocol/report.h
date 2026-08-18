#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// Interface-2 (vendor config channel, EP 0x83 IN / 0x04 OUT) 64-byte HID report.
// Layout confirmed by offline analysis of the public usbmon3 capture
// (OpenRGB issue #4950), see PROTOCOL.md and docs/evidence/. NOT yet confirmed
// against real hardware writes (SPEC.md Phase 2 criterion).
//
//   bytes[0:2]  length, u16 LE (exact semantics still a hypothesis, see PROTOCOL.md)
//   bytes[2:4]  constant 0x0002
//   bytes[4:6]  seq, u16 LE, monotonically increasing across observed commands
//   byte[6]     subcmd
//   byte[7]     flags
//   bytes[8:62] payload, zero-padded
//   bytes[62:64] crc16_modbus(bytes[0:62]), u16 LE

constexpr size_t kReportSize = 64;
constexpr size_t kPayloadSize = kReportSize - 8 - 2; // 54 bytes

struct Interface2Report {
    uint16_t length = 0;
    uint16_t seq = 0;
    uint8_t subcmd = 0;
    uint8_t flags = 0;
    std::array<uint8_t, kPayloadSize> payload{};
};

// Extracts header fields and payload from a raw 64-byte report. Does not
// validate the checksum - use report_crc_valid for that.
Interface2Report parse_report(const std::array<uint8_t, kReportSize>& raw);

// Serializes a report back to 64 raw bytes, recomputing the CRC16/MODBUS
// trailer. `length` is taken as given from the struct, not derived from
// payload size - that derivation rule is not yet confirmed (see PROTOCOL.md).
std::array<uint8_t, kReportSize> build_report(const Interface2Report& report);

// Recomputes CRC16/MODBUS over bytes[0:62] and compares against the stored
// little-endian trailer in bytes[62:64].
bool report_crc_valid(const std::array<uint8_t, kReportSize>& raw);
