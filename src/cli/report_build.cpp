// Builds a 64-byte interface-2 report from individual fields and prints it
// as a 128-char hex string on stdout (for piping into report_send/report_dump).
// Never touches hidraw - pure offline report construction using the same
// build_report() the tests are verified against.
//
// Usage: report_build --subcmd <hex> --seq <hex> [--session <hex>] [--flags <hex>] [--length <hex>] [--payload <hex>]
//
// --length has no confirmed derivation rule from the payload (see PROTOCOL.md),
// so it must be given explicitly rather than guessed.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>

#include "../protocol/report.h"

namespace {

std::optional<unsigned long> parse_hex_arg(const std::string& s) {
    if (s.empty()) {
        return std::nullopt;
    }
    char* end = nullptr;
    unsigned long value = std::strtoul(s.c_str(), &end, 16);
    if (end == s.c_str() || *end != '\0') {
        return std::nullopt;
    }
    return value;
}

void print_usage(const char* prog) {
    std::fprintf(stderr,
        "usage: %s --subcmd <hex> --seq <hex> [--session <hex, default 0002>]"
        " [--flags <hex, default 00>] --length <hex> [--payload <hex bytes>]\n"
        "  Prints the resulting 128-char report hex on stdout.\n"
        "  --length has no confirmed derivation rule - must be given explicitly,\n"
        "  see PROTOCOL.md.\n",
        prog);
}

}  // namespace

int main(int argc, char** argv) {
    std::optional<unsigned long> subcmd, seq, length;
    unsigned long session = 0x0002;  // matches Interface2Report's default
    unsigned long flags = 0x00;
    std::string payload_hex;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "error: %s requires a value\n", arg.c_str());
                std::exit(2);
            }
            return argv[++i];
        };

        if (arg == "--subcmd") {
            subcmd = parse_hex_arg(next());
        } else if (arg == "--seq") {
            seq = parse_hex_arg(next());
        } else if (arg == "--session") {
            auto v = parse_hex_arg(next());
            if (!v) { std::fprintf(stderr, "error: invalid --session\n"); return 2; }
            session = *v;
        } else if (arg == "--flags") {
            auto v = parse_hex_arg(next());
            if (!v) { std::fprintf(stderr, "error: invalid --flags\n"); return 2; }
            flags = *v;
        } else if (arg == "--length") {
            length = parse_hex_arg(next());
        } else if (arg == "--payload") {
            payload_hex = next();
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::fprintf(stderr, "error: unknown argument %s\n", arg.c_str());
            print_usage(argv[0]);
            return 2;
        }
    }

    if (!subcmd || !seq || !length) {
        std::fprintf(stderr, "error: --subcmd, --seq, and --length are required\n");
        print_usage(argv[0]);
        return 2;
    }
    if (payload_hex.size() % 2 != 0 || payload_hex.size() / 2 > kPayloadSize) {
        std::fprintf(stderr, "error: --payload must be an even-length hex string of at most %zu bytes\n",
                     kPayloadSize);
        return 2;
    }

    Interface2Report report;
    report.length = static_cast<uint16_t>(*length);
    report.session = static_cast<uint16_t>(session);
    report.seq = static_cast<uint16_t>(*seq);
    report.subcmd = static_cast<uint8_t>(*subcmd);
    report.flags = static_cast<uint8_t>(flags);
    for (size_t i = 0; i < payload_hex.size() / 2; ++i) {
        report.payload[i] = static_cast<uint8_t>(std::stoul(payload_hex.substr(i * 2, 2), nullptr, 16));
    }

    std::array<uint8_t, kReportSize> raw = build_report(report);

    std::fprintf(stderr, "length=%u seq=0x%04x session=0x%04x flags=0x%02x subcmd=0x%02x\n",
                 report.length, report.seq, report.session, report.flags, report.subcmd);

    for (uint8_t b : raw) {
        std::printf("%02x", b);
    }
    std::printf("\n");
    return 0;
}
