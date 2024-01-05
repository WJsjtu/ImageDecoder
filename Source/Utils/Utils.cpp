#include "Utils.h"

namespace ImageDecoder {

unsigned CountTrailingZeros(uint32_t n) {
    unsigned bits = 0, x = n;

    if (x) {
        /* assuming `x` has 32 bits: lets count the low order 0 bits in batches */
        /* mask the 16 low order bits, add 16 and shift them out if they are all 0 */
        if (!(x & 0x0000FFFF)) {
            bits += 16;
            x >>= 16;
        }
        /* mask the 8 low order bits, add 8 and shift them out if they are all 0 */
        if (!(x & 0x000000FF)) {
            bits += 8;
            x >>= 8;
        }
        /* mask the 4 low order bits, add 4 and shift them out if they are all 0 */
        if (!(x & 0x0000000F)) {
            bits += 4;
            x >>= 4;
        }
        /* mask the 2 low order bits, add 2 and shift them out if they are all 0 */
        if (!(x & 0x00000003)) {
            bits += 2;
            x >>= 2;
        }
        /* mask the low order bit and add 1 if it is 0 */
        bits += (x & 1) ^ 1;
    }
    return bits;
}

unsigned CountLeadingZeros(uint32_t x) {
    static const char debruijn32[32] = {0, 31, 9, 30, 3, 8, 13, 29, 2, 5, 7, 21, 12, 24, 28, 19, 1, 10, 4, 14, 6, 22, 25, 20, 11, 15, 23, 26, 16, 27, 17, 18};
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return debruijn32[x * 0x076be629 >> 27];
}
}  // namespace ImageDecoder
