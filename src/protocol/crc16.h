#pragma once

#include <cstddef>
#include <cstdint>

// CRC16/MODBUS (poly 0x8005, init 0xFFFF, reflected in/out, no xorout).
// Confirmed against 20/20 known Light Mount interface-2 reports, see
// docs/evidence/checksum_verification.py and PROTOCOL.md.
uint16_t crc16_modbus(const uint8_t* data, size_t len);
