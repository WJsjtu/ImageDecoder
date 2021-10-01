#include "ImageWrapperBase.h"
#include <assert.h>

/* FImageWrapperBase structors
 *****************************************************************************/

FImageWrapperBase::FImageWrapperBase() : RawFormat(ERGBFormat::Invalid), RawBitDepth(0), Format(ERGBFormat::Invalid), BitDepth(0), Width(0), Height(0), NumFrames(1), Framerate(0) {}

/* FImageWrapperBase interface
 *****************************************************************************/

void FImageWrapperBase::Reset() {
    LastError.clear();

    RawFormat = ERGBFormat::Invalid;
    RawBitDepth = 0;
    Format = ERGBFormat::Invalid;
    BitDepth = 0;
    Width = 0;
    Height = 0;
    NumFrames = 1;
    Framerate = 0;
}

void FImageWrapperBase::SetError(const char* ErrorMessage) { LastError = ErrorMessage; }

/* IImageWrapper structors
 *****************************************************************************/

const std::vector<uint8_t>& FImageWrapperBase::GetCompressed(int Quality) {
    LastError.clear();
    Compress(Quality);

    return CompressedData;
}

bool FImageWrapperBase::GetRaw(const ERGBFormat InFormat, int InBitDepth, std::vector<uint8_t>& OutRawData) {
    LastError.clear();
    Uncompress(InFormat, InBitDepth);

    if (LastError.empty()) {
        OutRawData = std::move(RawData);
    }

    return LastError.empty();
}

bool FImageWrapperBase::SetCompressed(const void* InCompressedData, int64_t InCompressedSize) {
    if (InCompressedSize > 0 && InCompressedData != nullptr) {
        Reset();
        RawData.clear();  // Invalidates the raw data too

        CompressedData.resize(InCompressedSize);
        for (int i = 0; i < InCompressedSize; i++) {
            CompressedData[i] = static_cast<const uint8_t*>(InCompressedData)[i];
        }

        return true;
    }

    return false;
}

bool FImageWrapperBase::SetRaw(const void* InRawData, int64_t InRawSize, const int InWidth, const int InHeight, const ERGBFormat InFormat, const int InBitDepth) {
    assert(InRawData != NULL);
    assert(InRawSize > 0);
    assert(InWidth > 0);
    assert(InHeight > 0);

    Reset();
    CompressedData.clear();  // Invalidates the compressed data too

    RawData.resize(InRawSize);
    for (int i = 0; i < InRawSize; i++) {
        RawData[i] = static_cast<const uint8_t*>(InRawData)[i];
    }

    RawFormat = InFormat;
    RawBitDepth = InBitDepth;

    Width = InWidth;
    Height = InHeight;

    return true;
}

bool FImageWrapperBase::SetAnimationInfo(int InNumFrames, int InFramerate) {
    NumFrames = InNumFrames;
    Framerate = InFramerate;

    return true;
}
