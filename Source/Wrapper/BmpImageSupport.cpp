#include "BmpImageSupport.h"
#include "Utils/Utils.h"

namespace ImageDecoder {
bool FBmiColorsMask::IsMaskRGB8() const { return CountLeadingZeros(RGBAMask[0]) + CountTrailingZeros(RGBAMask[0]) == 24 && CountLeadingZeros(RGBAMask[1]) + CountTrailingZeros(RGBAMask[1]) == 24 && CountLeadingZeros(RGBAMask[2]) + CountTrailingZeros(RGBAMask[2]) == 24; }
}  // namespace ImageDecoder
