#pragma once
#include <cstdint>

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
inline constexpr T Align(T Val, uint64_t Alignment) {
    return (T)(((uint64_t)Val + Alignment - 1) & ~(Alignment - 1));
}