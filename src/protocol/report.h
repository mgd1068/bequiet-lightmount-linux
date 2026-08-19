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
//   bytes[2:4]  "session", u16 LE - observed 0x0000/0x0001/0x0002 across different
//               phases of a single connection (handshake/setup-batch/runtime), NOT a
//               fixed protocol constant and NOT simply "which browser tab" - exact
//               meaning still unclear, see PROTOCOL.md ("Verbindungsaufbau-Capture").
//   byte[4]     counter - a single-byte, per-connection running counter that increases
//               by exactly 1 for EVERY command regardless of the "session"/subcmd
//               value. NOT freely choosable: the device tracks its own current value
//               and rejects a counter that doesn't continue from it (distinguishable
//               by a different response shape - full echo with byte[3]=0x0a instead of
//               a short ack with byte[3]=0x00). Once continued correctly from an
//               observed real value, subsequent +1 increments work reliably (confirmed
//               with two consecutive real hardware writes). How a fresh client learns
//               the current value without a live capture is UNSOLVED, see BACKLOG.md.
//   byte[5]     marker - NOT the counter's high byte (previously misdocumented as such
//               up to 2026-08-18). Varies per command/attribute during the connection
//               handshake; constant 0x10 for every static-color command observed so
//               far (both third-party and our own captures) and 0x01 for the periodic
//               keepalive (subcmd 0x03). Exact meaning otherwise unconfirmed.
//   byte[6]     subcmd
//   byte[7]     flags
//   bytes[8:62] payload, zero-padded
//   bytes[62:64] crc16_modbus(bytes[0:62]), u16 LE

constexpr size_t kReportSize = 64;
constexpr size_t kPayloadSize = kReportSize - 8 - 2; // 54 bytes

struct Interface2Report {
    uint16_t length = 0;
    uint16_t session = 0x0002;  // default matches the known usbmon3 fixtures
    uint8_t counter = 0;        // byte[4] - must continue from the device's real state
    uint8_t marker = 0x10;      // byte[5] - 0x10 for every known static-color command
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
