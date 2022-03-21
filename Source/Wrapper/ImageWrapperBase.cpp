#include "ImageWrapperBase.h"
#include "Utils/Utils.h"

namespace ImageDecoder {

bool IImageWrapper::GetRaw(const ERGBFormat inFormat, int inBitDepth, Vector<uint8_t>& outRawData) {
    Vector<uint8_t> tmpRawData;
    if (GetRaw(inFormat, inBitDepth, tmpRawData)) {
        if (tmpRawData.size() != tmpRawData.size()) {
            std::cerr << "Tried to get " << GetWidth() << "x" << GetHeight() << " " << inBitDepth << "bpp image with format " << static_cast<int8_t>(inFormat) << " into 32-bit TArray (%" << tmpRawData.size() << " bytes)" << std::endl;
            return false;
        }
        // todo check
        std::swap(outRawData, tmpRawData);
        return true;
    } else {
        return false;
    }
}  // namespace ImageDecoder

/* FImageWrapperBase structors
 *****************************************************************************/

FImageWrapperBase::FImageWrapperBase() : rawFormat(ERGBFormat::Invalid), rawBitDepth(0), format(ERGBFormat::Invalid), bitDepth(0), width(0), height(0), numFrames(1), framerate(0) {}

/* FImageWrapperBase interface
 *****************************************************************************/

void FImageWrapperBase::Reset() {
    lastError.clear();

    rawFormat = ERGBFormat::Invalid;
    rawBitDepth = 0;
    format = ERGBFormat::Invalid;
    bitDepth = 0;
    width = 0;
    height = 0;
    numFrames = 1;
    framerate = 0;
}

void FImageWrapperBase::SetError(const char* ErrorMessage) { lastError = ErrorMessage; }

/* IImageWrapper structors
 *****************************************************************************/

const Vector<uint8_t>& FImageWrapperBase::GetCompressed(int quality) {
    lastError.clear();
    Compress(quality);

    return compressedData;
}

bool FImageWrapperBase::GetRaw(const ERGBFormat inFormat, int inBitDepth, Vector<uint8_t>& outRawData) {
    lastError.clear();
    Uncompress(inFormat, inBitDepth);

    if (lastError.empty()) {
        outRawData = std::move(rawData);
    }

    return lastError.empty();
}

bool FImageWrapperBase::SetCompressed(const void* inCompressedData, int64_t inCompressedSize) {
    if (inCompressedSize > 0 && inCompressedData != nullptr) {
        Reset();
        rawData.clear();  // Invalidates the raw data too

        compressedData.resize(inCompressedSize);
        for (int i = 0; i < inCompressedSize; i++) {
            compressedData[i] = static_cast<const uint8_t*>(inCompressedData)[i];
        }

        return true;
    }

    return false;
}

bool FImageWrapperBase::SetRaw(const void* inRawData, int64_t inRawSize, const int inWidth, const int inHeight, const ERGBFormat inFormat, const int inBitDepth) {
    Assert(inRawData != NULL);
    Assert(inRawSize > 0);
    Assert(inWidth > 0);
    Assert(inHeight > 0);

    Reset();
    compressedData.clear();  // Invalidates the compressed data too

    rawData.resize(inRawSize);
    for (int i = 0; i < inRawSize; i++) {
        rawData[i] = static_cast<const uint8_t*>(inRawData)[i];
    }

    rawFormat = inFormat;
    rawBitDepth = inBitDepth;

    width = inWidth;
    height = inHeight;

    return true;
}

bool FImageWrapperBase::SetAnimationInfo(int inNumFrames, int inFramerate) {
    numFrames = inNumFrames;
    framerate = inFramerate;

    return true;
}
}  // namespace ImageDecoder