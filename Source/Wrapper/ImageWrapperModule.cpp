#include <memory>
#include "Formats/BmpImageWrapper.h"
#include "Formats/ExrImageWrapper.h"
#include "Formats/IcoImageWrapper.h"
#include "Formats/JpegImageWrapper.h"
#include "Formats/PngImageWrapper.h"
#include "Decoder.h"

namespace {
static const uint8_t IMAGE_MAGIC_PNG[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
static const uint8_t IMAGE_MAGIC_JPEG[] = {0xFF, 0xD8, 0xFF};
static const uint8_t IMAGE_MAGIC_BMP[] = {0x42, 0x4D};
static const uint8_t IMAGE_MAGIC_ICO[] = {0x00, 0x00, 0x01, 0x00};
static const uint8_t IMAGE_MAGIC_EXR[] = {0x76, 0x2F, 0x31, 0x01};
static const uint8_t IMAGE_MAGIC_ICNS[] = {0x69, 0x63, 0x6E, 0x73};

/** Internal helper function to verify image signature. */
template <int magicCount>
bool StartsWith(const uint8_t* content, int64_t contentSize, const uint8_t (&Magic)[magicCount]) {
    if (contentSize < magicCount) {
        return false;
    }

    for (int i = 0; i < magicCount; ++i) {
        if (content[i] != Magic[i]) {
            return false;
        }
    }

    return true;
}
}  // namespace

namespace ImageDecoder {
/**
 * Image Wrapper module.
 */
class ImageWrapperModule : public IImageWrapperModule {
public:
    //~ IImageWrapperModule interface

    virtual IImageWrapper* CreateImageWrapper(const EImageFormat inFormat) override {
        IImageWrapper* imageWrapper = NULL;

        // Allocate a helper for the format type
        switch (inFormat) {
            case EImageFormat::PNG: imageWrapper = new FPngImageWrapper(); break;
            case EImageFormat::JPEG: imageWrapper = new FJpegImageWrapper(); break;
            case EImageFormat::GrayscaleJPEG: imageWrapper = new FJpegImageWrapper(1); break;
            case EImageFormat::BMP: imageWrapper = new FBmpImageWrapper(); break;
            case EImageFormat::ICO: imageWrapper = new FIcoImageWrapper(); break;
            case EImageFormat::EXR: imageWrapper = new FExrImageWrapper(); break;

            default: break;
        }
        return imageWrapper;
    }

    virtual EImageFormat DetectImageFormat(const void* compressedData, int64_t compressedSize) override {
        EImageFormat Format = EImageFormat::Invalid;
        if (StartsWith((uint8_t*)compressedData, compressedSize, IMAGE_MAGIC_PNG)) {
            Format = EImageFormat::PNG;
        } else if (StartsWith((uint8_t*)compressedData, compressedSize, IMAGE_MAGIC_JPEG)) {
            Format = EImageFormat::JPEG;  // @Todo: Should we detect grayscale vs non-grayscale?
        } else if (StartsWith((uint8_t*)compressedData, compressedSize, IMAGE_MAGIC_BMP)) {
            Format = EImageFormat::BMP;
        } else if (StartsWith((uint8_t*)compressedData, compressedSize, IMAGE_MAGIC_ICO)) {
            Format = EImageFormat::ICO;
        } else if (StartsWith((uint8_t*)compressedData, compressedSize, IMAGE_MAGIC_EXR)) {
            Format = EImageFormat::EXR;
        }

        return Format;
    }
};

ImageWrapperModule SharedImageWrapperModule;

IImageWrapperModule& GetImageWrapperModule() { return SharedImageWrapperModule; }
}  // namespace ImageDecoder
