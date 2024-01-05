// Copyright Epic Games, Inc. All Rights Reserved.

#include "JpegImageWrapper.h"
#include "Utils/Utils.h"
#include <algorithm>
#include <mutex>

namespace ImageDecoder {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"

#if PLATFORM_UNIX || PLATFORM_MAC
#pragma clang diagnostic ignored "-Wshift-negative-value"  // clang 3.7.0
#endif
#endif

#pragma push_macro("DLLEXPORT")
#undef DLLEXPORT  // libjpeg-turbo defines DLLEXPORT as well
#include "turbojpeg.h"
#pragma pop_macro("DLLEXPORT")

#ifdef __clang__
#pragma clang diagnostic pop
#endif
template <class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return clamp(v, lo, hi, std::less<T>{});
}

template <class T, class Compare>
const T& clamp(const T& v, const T& lo, const T& hi, Compare comp) {
    Assert(!comp(hi, lo));
    return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}
int ConvertTJpegPixelFormat(ERGBFormat InFormat) {
    switch (InFormat) {
        case ERGBFormat::BGRA: return TJPF_BGRA;
        case ERGBFormat::Gray: return TJPF_GRAY;
        case ERGBFormat::RGBA: return TJPF_RGBA;
        default: return TJPF_RGBA;
    }
}

// Only allow one thread to use JPEG decoder at a time (it's not thread safe)
std::mutex GJPEGSection;

/* FJpegImageWrapper structors
 *****************************************************************************/

FJpegImageWrapper::FJpegImageWrapper(int inNumComponents) : FImageWrapperBase(), numComponents(inNumComponents), compressor(tjInitCompress()), decompressor(tjInitDecompress()) {}

FJpegImageWrapper::~FJpegImageWrapper() {
    std::lock_guard<std::mutex> JPEGLock(GJPEGSection);

    if (compressor) {
        tjDestroy(compressor);
    }
    if (decompressor) {
        tjDestroy(decompressor);
    }
}

/* FImageWrapperBase interface
 *****************************************************************************/

bool FJpegImageWrapper::SetCompressed(const void* inCompressedData, int64_t inCompressedSize) { return SetCompressedTurbo(inCompressedData, inCompressedSize); }

bool FJpegImageWrapper::SetRaw(const void* inRawData, int64_t inRawSize, const int inWidth, const int inHeight, const ERGBFormat inFormat, const int inBitDepth) {
    Assert((inFormat == ERGBFormat::RGBA || inFormat == ERGBFormat::BGRA || inFormat == ERGBFormat::Gray) && inBitDepth == 8);

    bool bResult = FImageWrapperBase::SetRaw(inRawData, inRawSize, inWidth, inHeight, inFormat, inBitDepth);

    return bResult;
}

void FJpegImageWrapper::Compress(int quality) { CompressTurbo(quality); }

void FJpegImageWrapper::Uncompress(const ERGBFormat inFormat, int inBitDepth) { UncompressTurbo(inFormat, inBitDepth); }

bool FJpegImageWrapper::SetCompressedTurbo(const void* inCompressedData, int64_t inCompressedSize) {
    std::lock_guard<std::mutex> JPEGLock(GJPEGSection);

    Assert(decompressor);

    int imageWidth;
    int imageHeight;
    int subSampling;
    int colorSpace;
    if (tjDecompressHeader3(decompressor, reinterpret_cast<const uint8_t*>(inCompressedData), static_cast<unsigned long>(inCompressedSize), &imageWidth, &imageHeight, &subSampling, &colorSpace) != 0) {
        return false;
    }

    const bool bResult = FImageWrapperBase::SetCompressed(inCompressedData, inCompressedSize);

    // set after call to base SetCompressed as it will reset members
    width = imageWidth;
    height = imageHeight;
    bitDepth = 8;  // We don't support 16 bit jpegs
    format = subSampling == TJSAMP_GRAY ? ERGBFormat::Gray : ERGBFormat::RGBA;

    return bResult;
}

void FJpegImageWrapper::CompressTurbo(int quality) {
    if (compressedData.size() == 0) {
        std::lock_guard<std::mutex> JPEGLock(GJPEGSection);

        Assert(compressor);

        if (quality == 0) {
            quality = 85;
        }
        Assert(quality >= 1 && quality <= 100);
        quality = std::min(std::max(quality, 1), 100);

        Assert(rawData.size());
        Assert(width > 0);
        Assert(height > 0);

        compressedData.resize(rawData.size());

        const int pixelFormat = ConvertTJpegPixelFormat(rawFormat);
        unsigned char* outBuffer = compressedData.data();
        unsigned long outBufferSize = static_cast<unsigned long>(compressedData.size());
        const int flags = TJFLAG_NOREALLOC | TJFLAG_FASTDCT;

        const bool bSuccess = tjCompress2(compressor, rawData.data(), width, 0, height, pixelFormat, &outBuffer, &outBufferSize, TJSAMP_420, quality, flags) == 0;
        Assert(bSuccess);

        compressedData.resize((int64_t)outBufferSize);
    }
}

void FJpegImageWrapper::UncompressTurbo(const ERGBFormat inFormat, int inBitDepth) {
    // Ensure we haven't already uncompressed the file.
    if (rawData.size() != 0) {
        return;
    }

    // Get the number of channels we need to extract
    int channels = 0;
    if ((inFormat == ERGBFormat::RGBA || inFormat == ERGBFormat::BGRA) && inBitDepth == 8) {
        channels = 4;
    } else if (inFormat == ERGBFormat::Gray && inBitDepth == 8) {
        channels = 1;
    } else {
        Assert(false);
    }

    std::lock_guard<std::mutex> jpegLock(GJPEGSection);

    Assert(decompressor);
    Assert(compressedData.size());

    rawData.resize(width * height * channels);
    const int pixelFormat = ConvertTJpegPixelFormat(inFormat);
    const int flags = TJFLAG_NOREALLOC | TJFLAG_FASTDCT;

    if (tjDecompress2(decompressor, compressedData.data(), static_cast<unsigned long>(compressedData.size()), rawData.data(), width, 0, height, pixelFormat, flags) != 0) {
        return;
    }
}
}  // namespace ImageDecoder
