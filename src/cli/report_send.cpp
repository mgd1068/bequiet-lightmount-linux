// Sends a single known interface-2 report to a hidraw device path, following
// docs/first-write-test-plan.md. Without --confirm it only parses and prints
// the report (dry-run, never calls open() on the device) - see SECURITY.md
// rule 10. Only ever performs exactly one write; never loops or retries on
// reset/disconnect - see SECURITY.md rule 8.
//
// Usage: report_send <hidraw-path> <128-hex-chars> [--confirm]

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <array>
#include <cctype>
#include <cstdio>
#include <optional>
#include <string>

#include "../protocol/report.h"

namespace {

constexpr int kWriteTimeoutMs = 500;

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

void print_summary(const std::array<uint8_t, kReportSize>& raw) {
    Interface2Report report = parse_report(raw);
    std::printf("length=%u session=0x%04x seq=0x%04x subcmd=0x%02x flags=0x%02x crc_valid=%s\n",
                report.length, report.session, report.seq, report.subcmd, report.flags,
                report_crc_valid(raw) ? "yes" : "NO");
}

// Returns true on a confirmed, complete write. Never retries: a timeout or
// disconnect is reported as-is and treated as an expected possible outcome
// (see SECURITY.md rule 8), not something to loop against.
bool write_once(const std::string& path, const std::array<uint8_t, kReportSize>& raw) {
    int fd = open(path.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        std::perror("open");
        return false;
    }

    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLOUT;
    int poll_result = poll(&pfd, 1, kWriteTimeoutMs);

    if (poll_result <= 0) {
        std::fprintf(stderr, "write not ready within %dms (poll_result=%d) - "
                     "device may be busy, disconnected, or reset. Not retrying.\n",
                     kWriteTimeoutMs, poll_result);
        close(fd);
        return false;
    }
    if (pfd.revents & (POLLERR | POLLHUP)) {
        std::fprintf(stderr, "device reports error/hangup before write - "
                     "possible USB reset. Not retrying.\n");
        close(fd);
        return false;
    }

    ssize_t written = write(fd, raw.data(), raw.size());
    close(fd);

    if (written != static_cast<ssize_t>(raw.size())) {
        std::fprintf(stderr, "short/failed write: %zd of %zu bytes\n", written, raw.size());
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        std::fprintf(stderr, "usage: %s <hidraw-path> <128-hex-chars> [--confirm]\n", argv[0]);
        return 2;
    }
    std::string path = argv[1];
    std::string hex = argv[2];
    bool confirm = (argc == 4) && (std::string(argv[3]) == "--confirm");

    auto raw = parse_hex(hex);
    if (!raw) {
        std::fprintf(stderr, "error: expected exactly %zu hex characters (64 bytes), got %zu\n",
                     kReportSize * 2, hex.size());
        return 1;
    }

    print_summary(*raw);

    if (!confirm) {
        std::printf("dry-run only (no --confirm given) - device not opened.\n");
        return 0;
    }

    std::printf("sending to %s ...\n", path.c_str());
    bool ok = write_once(path, *raw);
    std::printf("result: %s\n", ok ? "write completed" : "write FAILED (see stderr)");
    return ok ? 0 : 1;
}
