#pragma once

#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#include <x86intrin.h>
#endif
#endif

namespace nanomatch {

inline std::uint64_t rdtsc() {
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    return static_cast<std::uint64_t>(__rdtsc());
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__) || defined(__i386__)
    unsigned int lo = 0, hi = 0;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<std::uint64_t>(hi) << 32) | lo;
#else
    return 0;
#endif
#else
    return 0;
#endif
}

}  // namespace nanomatch
