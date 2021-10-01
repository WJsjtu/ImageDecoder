// Copyright Epic Games, Inc. All Rights Reserved.

#include "JpegImageWrapper.h"
#include <assert.h>
#include <algorithm>
#include <mutex>

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
constexpr const T& clamp(const T& v, const T& lo, const T& hi, Compare comp) {
    assert(!comp(hi, lo));
    return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}

namespace {
int ConvertTJpegPixelFormat(ERGBFormat InFormat) {
    switch (InFormat) {
        case ERGBFormat::BGRA: return TJPF_BGRA;
        case ERGBFormat::Gray: return TJPF_GRAY;
        case ERGBFormat::RGBA: return TJPF_RGBA;
        default: return TJPF_RGBA;
    }
}
}  // namespace

// Only allow one thread to use JPEG decoder at a time (it's not thread safe)
std::mutex GJPEGSection;

/* FJpegImageWrapper structors
 *****************************************************************************/

FJpegImageWrapper::FJpegImageWrapper(int InNumComponents) : FImageWrapperBase(), NumComponents(InNumComponents), Compressor(tjInitCompress()), Decompressor(tjInitDecompress()) {}

FJpegImageWrapper::~FJpegImageWrapper() {
    std::lock_guard<std::mutex> JPEGLock(GJPEGSection);

    if (Compressor) {
        tjDestroy(Compressor);
    }
    if (Decompressor) {
        tjDestroy(Decompressor);
    }
}

/* FImageWrapperBase interface
 *****************************************************************************/

bool FJpegImageWrapper::SetCompressed(const void* InCompressedData, int64_t InCompressedSize) { return SetCompressedTurbo(InCompressedData, InCompressedSize); }

bool FJpegImageWrapper::SetRaw(const void* InRawData, int64_t InRawSize, const int InWidth, const int InHeight, const ERGBFormat InFormat, const int InBitDepth) {
    assert((InFormat == ERGBFormat::RGBA || InFormat == ERGBFormat::BGRA || InFormat == ERGBFormat::Gray) && InBitDepth == 8);

    bool bResult = FImageWrapperBase::SetRaw(InRawData, InRawSize, InWidth, InHeight, InFormat, InBitDepth);

    return bResult;
}

void FJpegImageWrapper::Compress(int Quality) { CompressTurbo(Quality); }

void FJpegImageWrapper::Uncompress(const ERGBFormat InFormat, int InBitDepth) { UncompressTurbo(InFormat, InBitDepth); }

bool FJpegImageWrapper::SetCompressedTurbo(const void* InCompressedData, int64_t InCompressedSize) {
    std::lock_guard<std::mutex> JPEGLock(GJPEGSection);

    assert(Decompressor);

    int ImageWidth;
    int ImageHeight;
    int SubSampling;
    int ColorSpace;
    if (tjDecompressHeader3(Decompressor, reinterpret_cast<const uint8_t*>(InCompressedData), InCompressedSize, &ImageWidth, &ImageHeight, &SubSampling, &ColorSpace) != 0) {
        return false;
    }

    const bool bResult = FImageWrapperBase::SetCompressed(InCompressedData, InCompressedSize);

    // set after call to base SetCompressed as it will reset members
    Width = ImageWidth;
    Height = ImageHeight;
    BitDepth = 8;  // We don't support 16 bit jpegs
    Format = SubSampling == TJSAMP_GRAY ? ERGBFormat::Gray : ERGBFormat::RGBA;

    return bResult;
}

void FJpegImageWrapper::CompressTurbo(int Quality) {
    if (CompressedData.size() == 0) {
        std::lock_guard<std::mutex> JPEGLock(GJPEGSection);

        assert(Compressor);

        if (Quality == 0) {
            Quality = 85;
        }
        assert(Quality >= 1 && Quality <= 100);
        Quality = std::min(std::max(Quality, 1), 100);

        assert(RawData.size());
        assert(Width > 0);
        assert(Height > 0);

        CompressedData.resize(RawData.size());

        const int PixelFormat = ConvertTJpegPixelFormat(RawFormat);
        unsigned char* OutBuffer = CompressedData.data();
        unsigned long OutBufferSize = static_cast<unsigned long>(CompressedData.size());
        const int Flags = TJFLAG_NOREALLOC | TJFLAG_FASTDCT;

        const bool bSuccess = tjCompress2(Compressor, RawData.data(), Width, 0, Height, PixelFormat, &OutBuffer, &OutBufferSize, TJSAMP_420, Quality, Flags) == 0;
        assert(bSuccess);

        CompressedData.resize((int64_t)OutBufferSize);
    }
}

void FJpegImageWrapper::UncompressTurbo(const ERGBFormat InFormat, int InBitDepth) {
    // Ensure we haven't already uncompressed the file.
    if (RawData.size() != 0) {
        return;
    }

    // Get the number of channels we need to extract
    int Channels = 0;
    if ((InFormat == ERGBFormat::RGBA || InFormat == ERGBFormat::BGRA) && InBitDepth == 8) {
        Channels = 4;
    } else if (InFormat == ERGBFormat::Gray && InBitDepth == 8) {
        Channels = 1;
    } else {
        assert(false);
    }

    std::lock_guard<std::mutex> JPEGLock(GJPEGSection);

    assert(Decompressor);
    assert(CompressedData.size());

    RawData.resize(Width * Height * Channels);
    const int PixelFormat = ConvertTJpegPixelFormat(InFormat);
    const int Flags = TJFLAG_NOREALLOC | TJFLAG_FASTDCT;

    if (tjDecompress2(Decompressor, CompressedData.data(), CompressedData.size(), RawData.data(), Width, 0, Height, PixelFormat, Flags) != 0) {
        return;
    }
}
