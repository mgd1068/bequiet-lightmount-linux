// Dry-run tool: parses a 64-byte interface-2 report given as a 128-char hex
// string and prints its decoded fields. Never opens hidraw, never touches
// the device - see SECURITY.md. Useful for inspecting fixtures/captures and,
// later, for building reports offline before any real write test.
//
// Usage: report_dump <128-hex-chars>
//        echo <128-hex-chars> | report_dump

#include <array>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <optional>
#include <string>

#include "../protocol/report.h"

namespace {

std::optional<std::array<uint8_t, kReportSize>> parse_hex(const std::string& hex) {
    if (hex.size() != kReportSize * 2) {
        return std::nullopt;
    }
    std::array<uint8_t, kReportSize> raw{};
    for (size_t i = 0; i < kReportSize; ++i) {
        std::string byte_str = hex.substr(i * 2, 2);
        if (!std::isxdigit(static_cast<unsigned char>(byte_str[0])) ||
            !std::isxdigit(static_cast<unsigned char>(byte_str[1]))) {
            return std::nullopt;
        }
        raw[i] = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
    }
    return raw;
}

void print_report(const std::array<uint8_t, kReportSize>& raw) {
    Interface2Report report = parse_report(raw);
    bool crc_ok = report_crc_valid(raw);

    std::printf("length   = %u (0x%04x)\n", report.length, report.length);
    std::printf("session  = 0x%04x\n", report.session);
    std::printf("seq      = %u (0x%04x)\n", report.seq, report.seq);
    std::printf("subcmd   = 0x%02x\n", report.subcmd);
    std::printf("flags    = 0x%02x\n", report.flags);
    std::printf("crc_valid= %s\n", crc_ok ? "yes" : "NO - report is not a genuine/unmodified capture");
    std::printf("payload  = ");
    for (uint8_t b : report.payload) {
        std::printf("%02x ", b);
    }
    std::printf("\n");
}

}  // namespace

int main(int argc, char** argv) {
    std::string hex;
    if (argc == 2) {
        hex = argv[1];
    } else if (argc == 1) {
        std::getline(std::cin, hex);
    } else {
        std::fprintf(stderr, "usage: %s <128-hex-chars>  (or pipe via stdin)\n", argv[0]);
        return 2;
    }

    auto raw = parse_hex(hex);
    if (!raw) {
        std::fprintf(stderr, "error: expected exactly %zu hex characters (64 bytes), got %zu\n",
                     kReportSize * 2, hex.size());
        return 1;
    }

    print_report(*raw);
    return 0;
}
