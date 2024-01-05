#pragma once
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <string>

namespace ImageDecoder {

unsigned CountTrailingZeros(uint32_t n);

unsigned CountLeadingZeros(uint32_t x);

/**
 * Aligns a value to the nearest higher multiple of 'Alignment', which must be a power of two.
 *
 * @param  Val        The value to align.
 * @param  Alignment  The alignment value, must be a power of two.
 *
 * @return The value aligned up to the specified alignment.
 */
template <typename T>
inline constexpr T Align(T val, uint64_t alignment) {
    return (T)(((uint64_t)val + alignment - 1) & ~(alignment - 1));
}

#define Assert(expr)                                             \
    if (!(expr)) {                                               \
        std::string err = #expr;                                 \
        std::string file = __FILE__;                             \
        auto pos = file.find_last_of("SOURCE");                  \
        if (pos != std::string::npos) {                          \
            file = file.substr(pos + 6);                         \
        }                                                        \
        err = file + "-" + std::to_string(__LINE__) + ":" + err; \
        throw std::runtime_error(err);                           \
    }
}  // namespace ImageDecoder
