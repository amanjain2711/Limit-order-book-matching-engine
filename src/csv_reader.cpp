#include "nanomatch/csv_reader.hpp"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

#if defined(__linux__)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace nanomatch {

namespace {

Side parse_side(const std::string& s) {
    if (s == "BUY" || s == "buy" || s == "B") {
        return Side::Buy;
    }
    return Side::Sell;
}

OrderType parse_type(const std::string& s) {
    if (s == "MARKET" || s == "market" || s == "M") {
        return OrderType::Market;
    }
    if (s == "CANCEL" || s == "cancel" || s == "C") {
        return OrderType::Cancel;
    }
    return OrderType::Limit;
}

bool parse_event_line(std::string_view line, MarketEvent& ev) {
    std::string cols[6];
    int col = 0;
    std::size_t start = 0;
    while (start <= line.size() && col < 6) {
        const std::size_t comma = line.find(',', start);
        if (comma == std::string_view::npos) {
            cols[col++] = std::string(line.substr(start));
            break;
        }
        cols[col++] = std::string(line.substr(start, comma - start));
        start = comma + 1;
    }
    if (col < 6) {
        return false;
    }

    ev.type = parse_type(cols[0]);
    ev.id = static_cast<OrderId>(std::stoull(cols[1]));
    ev.side = parse_side(cols[2]);
    ev.price = static_cast<Price>(std::stoll(cols[3]));
    ev.qty = static_cast<Quantity>(std::stoul(cols[4]));
    ev.ts = static_cast<Timestamp>(std::stoull(cols[5]));
    return true;
}

#if defined(__linux__)
std::vector<MarketEvent> load_csv_mmap(const std::string& path) {
    const int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        throw std::runtime_error("Failed to open CSV: " + path + " error=" + std::strerror(errno));
    }

    struct stat st {};
    if (::fstat(fd, &st) != 0) {
        ::close(fd);
        throw std::runtime_error("fstat failed for CSV: " + path);
    }
    if (st.st_size <= 0) {
        ::close(fd);
        return {};
    }

    void* mapped = ::mmap(nullptr, static_cast<std::size_t>(st.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        ::close(fd);
        throw std::runtime_error("mmap failed for CSV: " + path + " error=" + std::strerror(errno));
    }

    const char* data = static_cast<const char*>(mapped);
    const std::size_t size = static_cast<std::size_t>(st.st_size);
    std::vector<MarketEvent> events;
    events.reserve(size / 28);  // rough heuristic for typical CSV row length

    bool header_skipped = false;
    std::size_t line_start = 0;
    for (std::size_t i = 0; i <= size; ++i) {
        const bool at_end = (i == size);
        if (!at_end && data[i] != '\n') {
            continue;
        }

        std::size_t line_end = i;
        if (line_end > line_start && data[line_end - 1] == '\r') {
            --line_end;
        }
        std::string_view line(data + line_start, line_end - line_start);
        line_start = i + 1;

        if (!header_skipped) {
            header_skipped = true;
            continue;
        }
        if (line.empty()) {
            continue;
        }

        MarketEvent ev;
        if (parse_event_line(line, ev)) {
            events.push_back(ev);
        }
    }

    ::munmap(mapped, size);
    ::close(fd);
    return events;
}
#endif

}  // namespace

std::vector<MarketEvent> load_csv(const std::string& path) {
#if defined(__linux__)
    // Linux fast-path: zero-copy file mapping for higher throughput replay.
    return load_csv_mmap(path);
#else
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Failed to open CSV: " + path);
    }

    std::vector<MarketEvent> events;
    std::string line;
    std::getline(in, line);  // header

    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        MarketEvent ev;
        if (parse_event_line(line, ev)) {
            events.push_back(ev);
        }
    }
    return events;
#endif
}

}  // namespace nanomatch
